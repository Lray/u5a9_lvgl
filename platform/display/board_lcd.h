#ifndef BOARD_LCD_H
#define BOARD_LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32u5xx_hal.h"

HAL_StatusTypeDef Board_LCD_BringUp(void);
uint32_t Board_LCD_GetLineEvents(void);
uint32_t Board_LCD_GetLastFramePeriod(void);

typedef struct
{
  uint32_t dsi_cr;
  uint32_t dsi_ccr;
  uint32_t dsi_lvcidr;
  uint32_t dsi_lcolcr;
  uint32_t dsi_vmcr;
  uint32_t dsi_vnpcr;
  uint32_t dsi_lpmcr;
  uint32_t dsi_pcr;
  uint32_t dsi_tccr;
  uint32_t dsi_clcr;
  uint32_t dsi_wrpcr;
  uint32_t dsi_ier0;
  uint32_t dsi_ier1;
  uint32_t ltdc_gcr;
  uint32_t ltdc_ier;
  uint32_t ltdc_srcr;
  uint32_t ltdc_bccr;
  uint32_t ltdc_l0cr;
  uint32_t ltdc_cfbar;
  uint32_t ltdc_cfblr;
  uint32_t ltdc_cfblnr;
} Board_LCD_RegSnapshot_t;

extern volatile Board_LCD_RegSnapshot_t g_board_lcd_regs;
extern volatile uint32_t g_board_lcd_dsi_error_count;
extern volatile uint32_t g_board_lcd_dsi_last_error;
extern volatile uint32_t g_board_lcd_ltdc_error_count;
extern volatile uint32_t g_board_lcd_ltdc_last_error;

#ifdef __cplusplus
}
#endif

#endif /* BOARD_LCD_H */
