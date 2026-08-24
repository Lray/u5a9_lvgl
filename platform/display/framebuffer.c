#include "framebuffer.h"
#include <string.h>
#include "ltdc.h"

uint16_t m_fb0_phys[196608] __attribute__((section(".fb0_phys"), aligned(16)));
uint16_t m_fb1_phys[196608] __attribute__((section(".fb1_phys"), aligned(16)));

volatile uint32_t g_fb_swap_submit_seq;
volatile uint32_t g_fb_reload_done_seq;
volatile uint32_t g_fb_swap_pending;
volatile uint32_t g_fb_swap_errors;
volatile uint32_t g_fb_front_virtual;
volatile uint32_t g_fb_back_virtual;
volatile uint32_t g_fb_submit_ts[8];
volatile uint32_t g_fb_done_ts[8];
static uint32_t m_ts_idx;

#define FB_RGB565_BYTES 393216UL

void FB_Init(void)
{
  memset(m_fb0_phys, 0, FB_RGB565_BYTES);
  memset(m_fb1_phys, 0, FB_RGB565_BYTES);

  g_fb_front_virtual = FB_VIRT_BUFFER1;
  g_fb_back_virtual = FB_VIRT_BUFFER0;
  g_fb_swap_submit_seq = 0U;
  g_fb_reload_done_seq = 0U;
  g_fb_swap_pending = 0U;
  g_fb_swap_errors = 0U;
  m_ts_idx = 0U;

  if (HAL_LTDC_SetAddress(&hltdc, g_fb_front_virtual, 0) != HAL_OK)
  {
    g_fb_swap_errors++;
  }
  if (HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_IMMEDIATE) != HAL_OK)
  {
    g_fb_swap_errors++;
  }
}

uint32_t FB_GetBackVirtual(void)
{
  return g_fb_back_virtual;
}

HAL_StatusTypeDef FB_Submit(void)
{
  if (g_fb_swap_pending != 0U)
  {
    g_fb_swap_errors++;
    return HAL_ERROR;
  }
  if (HAL_LTDC_SetAddress_NoReload(&hltdc, g_fb_back_virtual, 0) != HAL_OK)
  {
    g_fb_swap_errors++;
    return HAL_ERROR;
  }
  if (HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING) != HAL_OK)
  {
    g_fb_swap_errors++;
    return HAL_ERROR;
  }
  g_fb_swap_submit_seq++;
  g_fb_swap_pending = 1U;
  g_fb_submit_ts[m_ts_idx & 7U] = DWT->CYCCNT;
  return HAL_OK;
}

void HAL_LTDC_ReloadEventCallback(LTDC_HandleTypeDef *hltdc)
{
  uint32_t tmp = g_fb_front_virtual;

  g_fb_front_virtual = g_fb_back_virtual;
  g_fb_back_virtual = tmp;
  g_fb_reload_done_seq++;
  g_fb_swap_pending = 0U;
  g_fb_done_ts[m_ts_idx & 7U] = DWT->CYCCNT;
  m_ts_idx++;
}