#ifndef LV_PORT_DISPLAY_H
#define LV_PORT_DISPLAY_H

#include "lvgl.h"
#include <stdint.h>

#define LV_PORT_HOR_RES        480
#define LV_PORT_VER_RES        480
#define LV_PORT_STRIDE_BYTES   3072UL
#define LV_PORT_BUF_SIZE       (LV_PORT_STRIDE_BYTES * 480UL)

typedef struct
{
  volatile uint32_t frame_seq;
  volatile uint32_t submit_err;
  volatile uint32_t est_copy_bytes;
  volatile uint32_t est_ring[64];
  volatile uint8_t  est_ring_phase[64];
  volatile uint8_t  est_ring_idx;
} lv_port_stats_t;

extern lv_port_stats_t g_lv_port_stats;

lv_display_t * lv_port_display_create(void);

#endif /* LV_PORT_DISPLAY_H */
