#ifndef LV_DRAW_DMA2D_U5_H
#define LV_DRAW_DMA2D_U5_H

#include <stdint.h>

void lv_draw_dma2d_u5_init(void);
void u5_dma2d_selftest(void);

typedef struct
{
  volatile uint32_t evaluate_calls;
  volatile uint32_t accept_count;
  volatile uint32_t reject_type;
  volatile uint32_t reject_cf;
  volatile uint32_t reject_radius;
  volatile uint32_t reject_border;
  volatile uint32_t reject_shadow;
  volatile uint32_t reject_opa;
  volatile uint32_t reject_grad;
  volatile uint32_t sample_radius[4];
  volatile uint32_t sample_border[4];
  volatile uint32_t sample_opa[4];
  volatile uint32_t sample_grad_dir[4];
  volatile uint32_t sample_idx;
  volatile uint32_t dispatch_count;
  volatile uint32_t task_count;
  volatile uint32_t error_count;
  volatile uint32_t abort_count;
  volatile uint32_t last_timeout_ms;
  volatile uint32_t last_error_code;
} u5_dma2d_stats_t;

extern u5_dma2d_stats_t g_u5_dma2d_stats;

#endif /* LV_DRAW_DMA2D_U5_H */
