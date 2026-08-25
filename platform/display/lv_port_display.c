/**
 * @file lv_port_display.c
 *
 * M3 project display port: LVGL v9.3.0 direct-render double buffer bound to
 * the two GFXMMU virtual buffers (docs/00_plan.md §2.2/§7 M3).
 *
 * flush_cb: submit VBlank swap via FB_Submit() only (no lv_display_flush_ready).
 * flush_wait_cb: wait until the swap is done (g_fb_swap_pending == 0), same
 * contract as the stock v9.3 ST LTDC driver (ThirdParty/LVGL/src/drivers/
 * display/st_ltdc/lv_st_ltdc.c).
 */

#include "lv_port_display.h"
#include "framebuffer.h"
#include "perf_profiler.h"

_Static_assert(LV_PORT_HOR_RES == 480, "logical width must be 480");
_Static_assert(LV_PORT_VER_RES == 480, "logical height must be 480");
_Static_assert(LV_PORT_STRIDE_BYTES == 3072UL, "stride must be 3072 bytes");
_Static_assert(LV_PORT_BUF_SIZE == 1474560UL, "buf_size must be 3072*480 = 1,474,560");
_Static_assert(FB_VIRT_BUFFER0 == 0x24000000UL, "virtual buffer 0 base drifted");
_Static_assert(FB_VIRT_BUFFER1 == 0x24400000UL, "virtual buffer 1 base drifted");

lv_port_stats_t g_lv_port_stats;

static lv_area_t m_prev_area;
static uint8_t m_prev_valid;

static uint32_t area_px(const lv_area_t * a)
{
  return (uint32_t)(a->x2 - a->x1 + 1) * (uint32_t)(a->y2 - a->y1 + 1);
}

static uint32_t est_diff_px(const lv_area_t * prev, const lv_area_t * cur)
{
  uint32_t px_count = area_px(prev);
  int32_t x1 = prev->x1 > cur->x1 ? prev->x1 : cur->x1;
  int32_t y1 = prev->y1 > cur->y1 ? prev->y1 : cur->y1;
  int32_t x2 = prev->x2 < cur->x2 ? prev->x2 : cur->x2;
  int32_t y2 = prev->y2 < cur->y2 ? prev->y2 : cur->y2;

  if (x1 <= x2 && y1 <= y2)
  {
    px_count -= (uint32_t)(x2 - x1 + 1) * (uint32_t)(y2 - y1 + 1);
  }
  return px_count;
}

static void flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
  lv_port_stats_t * s = &g_lv_port_stats;
  uint32_t est = 0U;

  (void)disp;
  (void)px_map;

  /* est: previous frame's area not re-drawn this frame -> CPU sync copy into
   * the other buffer (refr_sync_areas). Byte count is a geometry estimate. */
  if (m_prev_valid)
  {
    est = est_diff_px(&m_prev_area, area) * 2U;
  }
  m_prev_area = *area;
  m_prev_valid = 1U;

  s->est_copy_bytes += est;
  s->est_ring[s->est_ring_idx & 63U] = est;
  s->est_ring_phase[s->est_ring_idx & 63U] = g_prof_phase;
  s->est_ring_idx++;

  /* Swap only on the frame's last flush (stock v9.3 ST LTDC driver pattern):
   * mid-frame flushes would expose partially rendered areas at VBlank. */
  if (lv_display_flush_is_last(disp))
  {
    if (FB_Submit() != HAL_OK)
    {
      s->submit_err++;
    }
  }
  s->frame_seq++;
}

static void flush_wait_cb(lv_display_t * disp)
{
  (void)disp;
  while (g_fb_swap_pending != 0U)
  {
  }
}

lv_display_t * lv_port_display_create(void)
{
  lv_display_t * disp = lv_display_create(LV_PORT_HOR_RES, LV_PORT_VER_RES);

  lv_display_set_flush_cb(disp, flush_cb);
  lv_display_set_flush_wait_cb(disp, flush_wait_cb);
  lv_display_set_buffers_with_stride(disp, (void *)FB_VIRT_BUFFER0, (void *)FB_VIRT_BUFFER1,
                                     LV_PORT_BUF_SIZE, LV_PORT_STRIDE_BYTES,
                                     LV_DISPLAY_RENDER_MODE_DIRECT);
  return disp;
}
