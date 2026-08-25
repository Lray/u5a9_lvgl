#ifndef PERF_PROFILER_H
#define PERF_PROFILER_H

#include <stdint.h>

#define LV_PROF_SLOT_CNT 24
#define LV_PROF_RING_LEN 64

typedef struct
{
  char name[32];
  volatile uint32_t calls;
  volatile uint32_t cycles;
  volatile uint32_t cyc_min;
  volatile uint32_t cyc_max;
  volatile uint32_t ring[LV_PROF_RING_LEN];
  volatile uint8_t  ring_phase[LV_PROF_RING_LEN];
  volatile uint8_t  ring_idx;
} lv_prof_slot_t;

extern lv_prof_slot_t g_prof_slots[LV_PROF_SLOT_CNT];
extern volatile uint8_t g_prof_phase;
extern volatile uint32_t g_prof_drops;

void lv_profiler_set_phase(uint8_t phase);
int8_t lv_profiler_find(const char * name);

#endif /* PERF_PROFILER_H */
