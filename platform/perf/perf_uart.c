#include "perf_uart.h"
#include "usart.h"
#include "app_stats.h"
#include "perf_nema_alloc.h"
#include "board_lcd.h"
#include <stdint.h>
#include <stdio.h>
#include <cmsis_os2.h>

perf_uart_stats_t g_perf_uart_stats;

static void perf_uart_task(void *arg)
{
  static char line[128];
  uint32_t seq = 0U;

  (void)arg;

  for (;;)
  {
    osDelay(1000U);

    int n = snprintf(line, sizeof(line),
                     "%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
                     (unsigned long)(seq++),
                     (unsigned long)g_m3_stats.uptime_ms,
                     (unsigned long)Board_LCD_GetLineEvents(),
                     (unsigned long)g_board_lcd_dsi_error_count,
                     (unsigned long)g_board_lcd_ltdc_error_count,
                     (unsigned long)g_nema_alloc_stats.allocs,
                     (unsigned long)g_nema_alloc_stats.frees,
                     (unsigned long)g_nema_alloc_stats.outst_hwm,
                     (unsigned long)g_nema_alloc_stats.fails,
                     (unsigned long)g_m3_stats.idle_percent,
                     (unsigned long)g_m3_stats.lv_used_max);

    if ((n <= 0) || ((uint32_t)n >= sizeof(line)))
    {
      g_perf_uart_stats.drops++;
      continue;
    }

    if (HAL_UART_Transmit(&huart1, (const uint8_t *)line, (uint16_t)n, 100U) != HAL_OK)
    {
      g_perf_uart_stats.drops++;
    }
    else
    {
      g_perf_uart_stats.lines++;
    }
  }
}

void Perf_Uart_Start(void)
{
  static const osThreadAttr_t attrs = {
      .name = "perfUart",
      .priority = (osPriority_t)osPriorityBelowNormal,
      .stack_size = 2048U,
  };

  (void)osThreadNew(perf_uart_task, NULL, &attrs);
}
