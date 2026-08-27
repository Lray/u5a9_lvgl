#ifndef APP_STATS_H
#define APP_STATS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct
{
  uint32_t uptime_ms;
  uint8_t  phase;
  uint32_t refr_calls;
  uint32_t refr_cycles;
  uint32_t sync_calls;
  uint32_t sync_cycles;
  uint32_t wait_calls;
  uint32_t wait_cycles;
  uint32_t line_events;
  uint32_t idle_percent;
  uint32_t lv_used_max;
  uint32_t rtos_heap_free_min;
  uint32_t task_stack_hwm;
  uint32_t err_dsi;
  uint32_t err_ltdc;
} m3_snapshot_t;

extern m3_snapshot_t g_m3_stats;

#ifdef __cplusplus
}
#endif

#endif /* APP_STATS_H */
