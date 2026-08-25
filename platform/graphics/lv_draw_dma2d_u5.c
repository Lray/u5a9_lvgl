/**
 * @file lv_draw_dma2d_u5.c
 *
 * M4 project-owned synchronous U5 DMA2D draw unit (docs/00_plan.md §5.1/§7 M4).
 *
 * Supported: LV_DRAW_TASK_TYPE_FILL with radius 0 / border 0 / bg_opa COVER /
 * no shadow / target RGB565. Filled synchronously via DMA2D R2M with the
 * output offset register supporting the 3072-B GFXMMU stride.
 * Timeout/error: HAL_DMA2D_Abort + re-init, counters set, task left WAITING
 * (fallback units re-dispatch it; the failed buffer is not swapped by this unit).
 *
 * Version-coupled to the locked v9.3 private draw API (lv_draw_private.h).
 */

#include "lv_draw_dma2d_u5.h"
#include "draw/lv_draw_private.h"
#include "draw/lv_draw_rect.h"
#include "draw/lv_draw_buf.h"
#include "dma2d.h"
#include "framebuffer.h"
#include <string.h>

#define PROJECT_DMA2D_UNIT_ID 5
#define PROJECT_DMA2D_SCORE 20
#define PROJECT_DMA2D_SHIELD_SCORE 80
#define DMA2D_POLL_TIMEOUT_MS 50U

#define GFXMMU_VIRT_BASE 0x24000000UL
#define GFXMMU_VIRT_END  0x25000000UL

typedef struct
{
  lv_draw_unit_t base_unit;
} u5_dma2d_unit_t;

volatile uint32_t g_u5_dma2d_selftest;
volatile uint32_t g_u5_dma2d_selftest_dump;

u5_dma2d_stats_t g_u5_dma2d_stats;

static uint32_t is_gfxmmu_virt(uint32_t addr)
{
  return (addr >= GFXMMU_VIRT_BASE && addr < GFXMMU_VIRT_END);
}

/* Dedicated route: this project unit is head-most (created last).
 * 1) Any root-layer task is claimed here with score 80 — which blocks the
 *    vendor Nema unit ('score > 80' guard) — and steered back to SW at
 *    dispatch time. Root buffers are GFXMMU 3072-B stride; neither DMA2D
 *    (dead-write evidence) nor Nema (960-B pitch vendor limit) can target it.
 * 2) Clean fills on contiguous off-screen layer buffers are executed here via
 *    synchronous DMA2D R2M. */
static int32_t evaluate_cb(lv_draw_unit_t * draw_unit, lv_draw_task_t * task)
{
  (void)draw_unit;
  const lv_draw_rect_dsc_t * dsc = (const lv_draw_rect_dsc_t *)task->draw_dsc;

  g_u5_dma2d_stats.evaluate_calls++;
  if (task->target_layer == NULL || task->target_layer->draw_buf == NULL ||
      task->target_layer->draw_buf->data == NULL) {
    return 0;
  }

  if (is_gfxmmu_virt((uint32_t)(uintptr_t)task->target_layer->draw_buf->data)) {
    if (task->preference_score > PROJECT_DMA2D_SHIELD_SCORE) {
      task->preference_score = PROJECT_DMA2D_SHIELD_SCORE;
      task->preferred_draw_unit_id = PROJECT_DMA2D_UNIT_ID;
    }
    g_u5_dma2d_stats.shield_count++;
    return 1;
  }

  if (task->type != LV_DRAW_TASK_TYPE_FILL) {
    g_u5_dma2d_stats.reject_type++;
    return 0;
  }
  if (task->target_layer->color_format != LV_COLOR_FORMAT_RGB565) {
    g_u5_dma2d_stats.reject_cf++;
    return 0;
  }
  if (dsc->radius != 0 || dsc->border_width != 0 || dsc->shadow_width != 0 ||
      dsc->bg_opa != LV_OPA_COVER || dsc->bg_grad.dir != LV_GRAD_DIR_NONE) {
    g_u5_dma2d_stats.reject_simple++;
    return 0;
  }

  g_u5_dma2d_stats.accept_count++;
  task->preferred_draw_unit_id = PROJECT_DMA2D_UNIT_ID;
  task->preference_score = PROJECT_DMA2D_SCORE;
  return 1;
}

static int32_t dispatch_cb(lv_draw_unit_t * draw_unit, lv_layer_t * layer)
{
  (void)draw_unit;
  lv_draw_task_t * task = lv_draw_get_available_task(layer, NULL, PROJECT_DMA2D_UNIT_ID);
  if (task == NULL) return 0;

  lv_draw_buf_t * draw_buf = task->target_layer->draw_buf;
  if (draw_buf == NULL || draw_buf->data == NULL) {
    task->state = LV_DRAW_TASK_STATE_READY;
    g_u5_dma2d_stats.error_count++;
    return 1;
  }

  g_u5_dma2d_stats.dispatch_count++;

  /* Root-layer shield: hand the task back to SW (reset preference). */
  if (is_gfxmmu_virt((uint32_t)(uintptr_t)draw_buf->data)) {
    task->preferred_draw_unit_id = LV_DRAW_UNIT_NONE;
    task->preference_score = 100;
    g_u5_dma2d_stats.steer_count++;
    return 1;
  }

  const lv_draw_rect_dsc_t * dsc = (const lv_draw_rect_dsc_t *)task->draw_dsc;
  g_u5_dma2d_stats.task_count++;

  task->state = LV_DRAW_TASK_STATE_IN_PROGRESS;
  {
    DMA2D_HandleTypeDef * h = &hdma2d;
    DMA2D_TypeDef * dm = h->Instance;
    /* R2M (RM0456 §27.5): solid OCOLR -> output; OOR = line offset in pixels */
    uint32_t stride_px = draw_buf->header.stride / 2;
    uint32_t offset_x = task->area.x1;
    uint32_t offset_y = task->area.y1;
    uint32_t width = (uint32_t)lv_area_get_width(&task->area);
    uint32_t height = (uint32_t)lv_area_get_height(&task->area);
    uint32_t dst_addr = (uint32_t)(uintptr_t)draw_buf->data +
                        (offset_y * draw_buf->header.stride) + (offset_x * 2);

    dm->CR &= ~(DMA2D_CR_MODE_Msk | DMA2D_CR_START_Msk);
    dm->CR |= DMA2D_R2M;
    dm->OPFCCR = DMA2D_OUTPUT_RGB565;
    dm->OCOLR = (uint32_t)lv_color_to_u16(dsc->bg_color) & 0xFFFFU;
    dm->OOR = stride_px - width;
    dm->OMAR = dst_addr;
    dm->NLR = (width << DMA2D_NLR_PL_Pos) | height;
    dm->FGPFCCR = 0U;
    dm->BGPFCCR = 0U;
    dm->CR |= DMA2D_CR_START_Msk;

    HAL_StatusTypeDef status = HAL_DMA2D_PollForTransfer(h, DMA2D_POLL_TIMEOUT_MS);
    if (status != HAL_OK) {
      g_u5_dma2d_stats.error_count++;
      g_u5_dma2d_stats.last_timeout_ms = DMA2D_POLL_TIMEOUT_MS;
      g_u5_dma2d_stats.last_error_code = (uint32_t)status;
      (void)HAL_DMA2D_Abort(h);
      g_u5_dma2d_stats.abort_count++;
      (void)HAL_DMA2D_Init(h);
      /* fallback units re-dispatch this task (it stays WAITING) */
      task->state = LV_DRAW_TASK_STATE_WAITING;
      return 1;
    }
  }

  task->state = LV_DRAW_TASK_STATE_READY;
  return 1;
}

void lv_draw_dma2d_u5_init(void)
{
  u5_dma2d_unit_t * unit = (u5_dma2d_unit_t *)lv_draw_create_unit(sizeof(u5_dma2d_unit_t));
  unit->base_unit.dispatch_cb = dispatch_cb;
  unit->base_unit.evaluate_cb = evaluate_cb;
  unit->base_unit.name = "U5_DMA2D";
  unit->base_unit.idx = PROJECT_DMA2D_UNIT_ID;
  memset((void *)&g_u5_dma2d_stats, 0, sizeof(g_u5_dma2d_stats));
}

/* 64x64 green (0x07E0) block at (0,0) of the current GFXMMU back virtual buffer;
 * validates R2M + OOR(3072 stride) + poll synchronously. */
void u5_dma2d_selftest(void)
{
  extern uint32_t gfxmmu_lut_config[960];
  uint32_t dst = FB_GetBackVirtual();
  DMA2D_TypeDef * dm = hdma2d.Instance;

  g_u5_dma2d_selftest_dump = 0xDEADBEEFU;

  /* 1. sanity: 4x4 R2M fill into plain SRAM */
  dm->CR &= ~(DMA2D_CR_MODE_Msk | DMA2D_CR_START_Msk);
  dm->CR |= DMA2D_R2M;
  dm->OPFCCR = DMA2D_OUTPUT_RGB565;
  dm->OCOLR = 0xAB34U;
  dm->OOR = 0U;
  dm->OMAR = (uint32_t)(uintptr_t)&g_u5_dma2d_selftest_dump;
  dm->NLR = (1U << DMA2D_NLR_PL_Pos) | 1U;
  dm->CR |= DMA2D_CR_START_Msk;
  g_u5_dma2d_selftest = (HAL_DMA2D_PollForTransfer(&hdma2d, 50U) == HAL_OK) ? 1U : 0U;
  g_u5_dma2d_selftest |= ((g_u5_dma2d_selftest_dump & 0xFFFFU) == 0xAB34U) ? 2U : 0U;

  /* 2. 64x64 green (0x07E0) into GFXMMU back virtual buffer */
  dm->CR &= ~(DMA2D_CR_MODE_Msk | DMA2D_CR_START_Msk);
  dm->CR |= DMA2D_R2M;
  dm->OPFCCR = DMA2D_OUTPUT_RGB565;
  dm->OCOLR = 0x07E0U;
  dm->OOR = (3072U / 2U) - 64U;
  dm->OMAR = dst;
  dm->NLR = (64U << DMA2D_NLR_PL_Pos) | 64U;
  dm->CR |= DMA2D_CR_START_Msk;
  if (HAL_DMA2D_PollForTransfer(&hdma2d, 50U) == HAL_OK) {
    g_u5_dma2d_selftest |= 4U;  /* poll ok */
  }
  if (g_u5_dma2d_selftest & 4U) {
    g_u5_dma2d_selftest |= ((*(volatile uint16_t *)dst) == 0x07E0U) ? 8U : 0U;
    uint32_t lo = gfxmmu_lut_config[1] & 0x003FFFF0U;
    uint32_t fvb = (gfxmmu_lut_config[0] >> 8U) & 0xFFU;
    uint32_t phys = (uint32_t)&m_fb0_phys[0] + ((lo + (fvb * 16U)) & 0x003FFFFFU);
    g_u5_dma2d_selftest |= ((*(volatile uint16_t *)phys) == 0x07E0U) ? 16U : 0U;
  }
}
