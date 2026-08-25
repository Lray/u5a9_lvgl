#ifndef LV_DRAW_DMA2D_U5_H
#define LV_DRAW_DMA2D_U5_H

#include <stdint.h>

void lv_draw_dma2d_u5_init(void);
void u5_dma2d_selftest(void);

typedef struct
{
  volatile uint32_t evaluate_calls;
  volatile uint32_t accept_count;
  volatile uint32_t shield_count;
  volatile uint32_t steer_count;
  volatile uint32_t reject_type;
  volatile uint32_t reject_cf;
  volatile uint32_t reject_simple;
  volatile uint32_t dispatch_count;
  volatile uint32_t task_count;
  volatile uint32_t error_count;
  volatile uint32_t abort_count;
  volatile uint32_t last_timeout_ms;
  volatile uint32_t last_error_code;
} u5_dma2d_stats_t;

extern u5_dma2d_stats_t g_u5_dma2d_stats;

#endif /* LV_DRAW_DMA2D_U5_H */
