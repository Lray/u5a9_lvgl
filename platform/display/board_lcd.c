#include "board_lcd.h"
#include <string.h>
#include "dsihost.h"
#include "ltdc.h"
#include "gfxmmu.h"
#include "framebuffer.h"
#include "stm32u5x9j_discovery_gfxmmu_lut.h"

#define VIRTUAL_BUFFER0_BASE 0x24000000UL
#define FB_LOGICAL_WIDTH  480UL
#define FB_LOGICAL_HEIGHT 480UL

/* m_fb0_phys / m_fb1_phys owned by framebuffer.c (section .fb0_phys/.fb1_phys) */

volatile uint32_t g_board_lcd_line_events;
volatile uint32_t g_board_lcd_last_frame_period;
volatile Board_LCD_RegSnapshot_t g_board_lcd_regs;
volatile uint32_t g_board_lcd_dsi_error_count;
volatile uint32_t g_board_lcd_dsi_last_error;
volatile uint32_t g_board_lcd_ltdc_error_count;
volatile uint32_t g_board_lcd_ltdc_last_error;
volatile uint32_t g_board_lcd_map_check[6];

static void snapshot_regs(void)
{
  g_board_lcd_regs.dsi_cr = DSI->CR;
  g_board_lcd_regs.dsi_ccr = DSI->CCR;
  g_board_lcd_regs.dsi_lvcidr = DSI->LVCIDR;
  g_board_lcd_regs.dsi_lcolcr = DSI->LCOLCR;
  g_board_lcd_regs.dsi_vmcr = DSI->VMCR;
  g_board_lcd_regs.dsi_vnpcr = DSI->VNPCR;
  g_board_lcd_regs.dsi_lpmcr = DSI->LPMCR;
  g_board_lcd_regs.dsi_pcr = DSI->PCR;
  g_board_lcd_regs.dsi_tccr = DSI->TCCR[0];
  g_board_lcd_regs.dsi_clcr = DSI->CLCR;
  g_board_lcd_regs.dsi_wrpcr = DSI->WRPCR;
  g_board_lcd_regs.dsi_ier0 = DSI->IER[0];
  g_board_lcd_regs.dsi_ier1 = DSI->IER[1];
  g_board_lcd_regs.ltdc_gcr = LTDC->GCR;
  g_board_lcd_regs.ltdc_ier = LTDC->IER;
  g_board_lcd_regs.ltdc_srcr = LTDC->SRCR;
  g_board_lcd_regs.ltdc_bccr = LTDC->BCCR;
  g_board_lcd_regs.ltdc_l0cr = LTDC_Layer1->CR;
  g_board_lcd_regs.ltdc_cfbar = LTDC_Layer1->CFBAR;
  g_board_lcd_regs.ltdc_cfblr = LTDC_Layer1->CFBLR;
  g_board_lcd_regs.ltdc_cfblnr = LTDC_Layer1->CFBLNR;
}

static void fill_rect_at(uint32_t base, uint32_t color, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
  uint32_t *row = (uint32_t *)(base + (y * FB_STRIDE_BYTES) + (x * 4U));
  for (uint32_t i = 0U; i < h; i++)
  {
    for (uint32_t j = 0U; j < w; j++)
    {
      row[j] = color;
    }
    row += FB_STRIDE_BYTES / 4U;
  }
}

static void fill_rect(uint32_t color, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
  fill_rect_at(g_fb_back_virtual, color, x, y, w, h);
}

static uint32_t virt_to_phys(uint32_t x, uint32_t y)
{
  uint32_t lo = gfxmmu_lut_config_argb8888[(y * 2U) + 1U] & 0x003FFFF0U;
  uint32_t fvb = (gfxmmu_lut_config_argb8888[y * 2U] >> 8U) & 0xFFU;
  uint32_t lvb = (gfxmmu_lut_config_argb8888[y * 2U] >> 16U) & 0xFFU;
  uint32_t block = fvb + (x / 4U);

  if (block > lvb)
  {
    return 0U;
  }
  return (uint32_t)m_fb0_phys + ((lo + (fvb * 16U)) & 0x003FFFFFU) + ((block - fvb) * 16U) +
         ((x % 4U) * 4U);
}

HAL_StatusTypeDef Board_LCD_BringUp(void)
{
  RCC_PeriphCLKInitTypeDef clk = {0};

  if (HAL_GFXMMU_ConfigLut(&hgfxmmu, 0, GFXMMU_LUT_SIZE,
                           (uint32_t)gfxmmu_lut_config_argb8888) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_GFXMMU_DisableLutLines(&hgfxmmu, GFXMMU_LUT_SIZE, 544U) != HAL_OK)
  {
    return HAL_ERROR;
  }

  clk.PeriphClockSelection = RCC_PERIPHCLK_DSI;
  clk.DsiClockSelection = RCC_DSICLKSOURCE_DSIPHY;
  if (HAL_RCCEx_PeriphCLKConfig(&clk) != HAL_OK)
  {
    return HAL_ERROR;
  }

  HAL_Delay(11);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_SET);
  HAL_Delay(150);

  if (HAL_DSI_Start(&hdsi) != HAL_OK)
  {
    return HAL_ERROR;
  }

  static const uint8_t unlock[3] = {0xFF, 0x83, 0x79};
  static const uint8_t pwr[16] = {0x44, 0x1C, 0x1C, 0x37, 0x57, 0x90, 0xD0, 0xE2,
                                  0x58, 0x80, 0x38, 0x38, 0xF8, 0x33, 0x34, 0x42};
  static const uint8_t disp[9] = {0x80, 0x14, 0x0C, 0x30, 0x20, 0x50, 0x11, 0x42, 0x1D};
  static const uint8_t cyc[10] = {0x01, 0xAA, 0x01, 0xAF, 0x01, 0xAF, 0x10, 0xEA, 0x1C, 0xEA};
  static const uint8_t vcom[4] = {0x00, 0x00, 0x00, 0xC0};
  static const uint8_t pan[37] = {0x00, 0x07, 0x00, 0x00, 0x00, 0x08, 0x08, 0x32, 0x10, 0x01, 0x00,
                                  0x01, 0x03, 0x72, 0x03, 0x72, 0x00, 0x08, 0x00, 0x08, 0x33, 0x33,
                                  0x05, 0x05, 0x37, 0x05, 0x05, 0x37, 0x0A, 0x00, 0x00, 0x00, 0x0A,
                                  0x00, 0x01, 0x00, 0x0E};
  static const uint8_t g1[34] = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x19, 0x19, 0x18,
                                 0x18, 0x18, 0x18, 0x19, 0x19, 0x01, 0x00, 0x03, 0x02, 0x05, 0x04,
                                 0x07, 0x06, 0x23, 0x22, 0x21, 0x20, 0x18, 0x18, 0x18, 0x18, 0x00,
                                 0x00};
  static const uint8_t g2[35] = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x19, 0x19, 0x18,
                                 0x18, 0x19, 0x19, 0x18, 0x18, 0x06, 0x07, 0x04, 0x05, 0x02, 0x03,
                                 0x00, 0x01, 0x20, 0x21, 0x22, 0x23, 0x18, 0x18, 0x18, 0x18};
  static const uint8_t gam[42] = {0x00, 0x16, 0x1B, 0x30, 0x36, 0x3F, 0x24, 0x40, 0x09, 0x0D, 0x0F,
                                  0x18, 0x0E, 0x11, 0x12, 0x11, 0x14, 0x07, 0x12, 0x13, 0x18, 0x00,
                                  0x17, 0x1C, 0x30, 0x36, 0x3F, 0x24, 0x40, 0x09, 0x0C, 0x0F, 0x18,
                                  0x0E, 0x11, 0x14, 0x11, 0x12, 0x07, 0x12, 0x14, 0x18};
  static const uint8_t b6[3] = {0x2C, 0x2C, 0x00};
  static const uint8_t gm0[] = {0x01, 0x00, 0x07, 0x0F, 0x16, 0x1F, 0x27, 0x30, 0x38, 0x40, 0x47,
                                0x4E, 0x56, 0x5D, 0x65, 0x6D, 0x74, 0x7D, 0x84, 0x8A, 0x90, 0x99,
                                0xA1, 0xA9, 0xB0, 0xB6, 0xBD, 0xC4, 0xCD, 0xD4, 0xDD, 0xE5, 0xEC,
                                0xF3, 0x36, 0x07, 0x1C, 0xC0, 0x1B, 0x01, 0xF1, 0x34, 0x00};
  static const uint8_t gm1[] = {0x00, 0x08, 0x0F, 0x16, 0x1F, 0x28, 0x31, 0x39, 0x41, 0x48, 0x51,
                                0x59, 0x60, 0x68, 0x70, 0x78, 0x7F, 0x87, 0x8D, 0x94, 0x9C, 0xA3,
                                0xAB, 0xB3, 0xB9, 0xC1, 0xC8, 0xD0, 0xD8, 0xE0, 0xE8, 0xEE, 0xF5,
                                0x3B, 0x1A, 0xB6, 0xA0, 0x07, 0x45, 0xC5, 0x37, 0x00};
  static const uint8_t gm2[42] = {0x00, 0x09, 0x0F, 0x18, 0x21, 0x2A, 0x34, 0x3C, 0x45, 0x4C, 0x56,
                                  0x5E, 0x66, 0x6E, 0x76, 0x7E, 0x87, 0x8E, 0x95, 0x9D, 0xA6, 0xAF,
                                  0xB7, 0xBD, 0xC5, 0xCE, 0xD5, 0xDF, 0xE7, 0xEE, 0xF4, 0xFA, 0xFF,
                                  0x0C, 0x31, 0x83, 0x3C, 0x5B, 0x56, 0x1E, 0x5A, 0xFF};

  if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 3, 0xB9, (uint8_t *)unlock) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 16, 0xB1, (uint8_t *)pwr) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 9, 0xB2, (uint8_t *)disp) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 10, 0xB4, (uint8_t *)cyc) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 4, 0xC7, (uint8_t *)vcom) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xCC, 0x02) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xD2, 0x77) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 37, 0xD3, (uint8_t *)pan) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 34, 0xD5, (uint8_t *)g1) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 35, 0xD6, (uint8_t *)g2) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 42, 0xE0, (uint8_t *)gam) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 3, 0xB6, (uint8_t *)b6) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xBD, 0x00) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 42, 0xC1, (uint8_t *)gm0) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xBD, 0x01) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 42, 0xC1, (uint8_t *)gm1) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xBD, 0x02) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, sizeof(gm2), 0xC1, (uint8_t *)gm2) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xBD, 0x00) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P0, 0x11, 0x00) != HAL_OK)
  {
    return HAL_ERROR;
  }
  HAL_Delay(120);

  fill_rect_at(VIRTUAL_BUFFER0_BASE, 0xFF000000UL, 0U, 0U, FB_LOGICAL_WIDTH, FB_LOGICAL_HEIGHT);

  if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P0, 0x29, 0x00) != HAL_OK)
  {
    return HAL_ERROR;
  }
  HAL_Delay(120);

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0U;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  HAL_NVIC_SetPriority(LTDC_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(LTDC_IRQn);

  snapshot_regs();

  return HAL_LTDC_ProgramLineEvent(&hltdc, 0);
}

void Board_LCD_VerifyMapping(void)
{
  uint32_t i;
  uint32_t pass = 0U;

  for (i = 0U; i < 6U; i++)
  {
    g_board_lcd_map_check[i] = 0U;
  }

  g_board_lcd_map_check[0] = 0xFF00FF00UL;
  fill_rect(g_board_lcd_map_check[0], 240U, 240U, 1U, 1U);
  g_board_lcd_map_check[1] = *(volatile uint32_t *)virt_to_phys(240U, 240U);
  if (g_board_lcd_map_check[1] == g_board_lcd_map_check[0])
  {
    pass++;
  }

  g_board_lcd_map_check[2] = 0xFFFF0000UL;
  *(volatile uint32_t *)(VIRTUAL_BUFFER0_BASE + (0U * FB_STRIDE_BYTES) + (200U * 4U)) =
      g_board_lcd_map_check[2];
  if (virt_to_phys(200U, 0U) == 0U)
  {
    pass++;
  }
  g_board_lcd_map_check[3] =
      *(volatile uint32_t *)(VIRTUAL_BUFFER0_BASE + (0U * FB_STRIDE_BYTES) + (200U * 4U));
  if (g_board_lcd_map_check[3] == 0xFFFFFFFFUL)
  {
    pass++;
  }

  g_board_lcd_map_check[4] = pass;
  g_board_lcd_map_check[5] = virt_to_phys(240U, 240U);
}

void Board_LCD_DiagnosticPatterns(void)
{
  static const uint32_t colors[] = {0xFFFF0000UL, 0xFF00FF00UL, 0xFF0000FFUL, 0xFFFFFFFFUL,
                                    0xFF000000UL};
  const uint32_t black = 0xFF000000UL;
  const uint32_t white = 0xFFFFFFFFUL;

  for (uint32_t c = 0U; c < (sizeof(colors) / sizeof(colors[0])); c++)
  {
    fill_rect(colors[c], 0U, 0U, FB_LOGICAL_WIDTH, FB_LOGICAL_HEIGHT);
    HAL_Delay(3000);
  }

  for (uint32_t y = 0U; y < FB_LOGICAL_HEIGHT; y += 8U)
  {
    for (uint32_t x = 0U; x < FB_LOGICAL_WIDTH; x += 8U)
    {
      uint32_t w = ((FB_LOGICAL_WIDTH - x) < 8U) ? (FB_LOGICAL_WIDTH - x) : 8U;
      uint32_t h = ((FB_LOGICAL_HEIGHT - y) < 8U) ? (FB_LOGICAL_HEIGHT - y) : 8U;
      fill_rect((((y / 8U) + (x / 8U)) & 1U) ? white : black, x, y, w, h);
    }
  }
  HAL_Delay(3000);
}

void HAL_LTDC_LineEventCallback(LTDC_HandleTypeDef *hltdc)
{
  static uint32_t prev;
  uint32_t now = DWT->CYCCNT;

  if (g_board_lcd_line_events != 0U)
  {
    g_board_lcd_last_frame_period = now - prev;
  }
  prev = now;
  g_board_lcd_line_events++;

  HAL_LTDC_ProgramLineEvent(hltdc, 0);
}

void HAL_DSI_ErrorCallback(DSI_HandleTypeDef *hdsi)
{
  g_board_lcd_dsi_error_count++;
  g_board_lcd_dsi_last_error = HAL_DSI_GetError(hdsi);
}

void HAL_LTDC_ErrorCallback(LTDC_HandleTypeDef *hltdc)
{
  g_board_lcd_ltdc_error_count++;
  g_board_lcd_ltdc_last_error = hltdc->ErrorCode;
}

void Board_LCD_FillBack(uint32_t color)
{
  fill_rect(color, 0U, 0U, FB_LOGICAL_WIDTH, FB_LOGICAL_HEIGHT);
}

void Board_LCD_SoakLoop(void)
{
  static const uint32_t bars[7] = {0xFFFFFFFFUL, 0xFFFFFF00UL, 0xFF00FFFFUL, 0xFF00FF00UL,
                                   0xFFFF00FFUL, 0xFFFF0000UL, 0xFF0000FFUL};
  const uint32_t bar_w = 68U;

  for (;;)
  {
    for (uint32_t b = 0U; b < 7U; b++)
    {
      fill_rect(bars[b], b * bar_w, 0U, bar_w, FB_LOGICAL_HEIGHT);
    }
    fill_rect(0xFF000000UL, 7U * bar_w, 0U, FB_LOGICAL_WIDTH - (7U * bar_w), FB_LOGICAL_HEIGHT);
    FB_Submit();
    while (g_fb_swap_pending != 0U) { }
    HAL_Delay(2000U);

    for (uint32_t y = 0U; y < FB_LOGICAL_HEIGHT; y++)
    {
      uint32_t v = (y * 255U) / (FB_LOGICAL_HEIGHT - 1U);
      fill_rect(0xFF000000UL | (v << 16U) | (v << 8U) | v, 0U, y, FB_LOGICAL_WIDTH, 1U);
    }
    FB_Submit();
    while (g_fb_swap_pending != 0U) { }
    HAL_Delay(2000U);
  }
}

uint32_t Board_LCD_GetLineEvents(void)
{
  return g_board_lcd_line_events;
}

uint32_t Board_LCD_GetLastFramePeriod(void)
{
  return g_board_lcd_last_frame_period;
}