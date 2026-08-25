/**
 * @file perf_profiler.c
 *
 * Project LVGL profiler backend (lv_conf.h LV_PROFILER_INCLUDE). LIFO pair
 * accumulation keyed by the tag given at LV_PROFILER_BEGIN; sessions in
 * SRAM2 .perf_trace (docs/00_plan.md §5.2/§7 M3).
 */

#include "perf_profiler.h"
#include "lv_profiler_backend.h"
#include "stm32u5xx_hal.h"
#include <string.h>

#define PROF_STACK_DEPTH 16

typedef struct
{
  const char * tag;
  uint32_t start;
} prof_frame_t;

static prof_frame_t s_stack[PROF_STACK_DEPTH];
static uint8_t s_sp;

lv_prof_slot_t g_prof_slots[LV_PROF_SLOT_CNT] __attribute__((section(".perf_trace"), aligned(32)));
volatile uint8_t g_prof_phase;
volatile uint32_t g_prof_drops;

static lv_prof_slot_t * slot_for(const char * tag)
{
  uint32_t i;
  for (i = 0U; i < LV_PROF_SLOT_CNT; i++)
  {
    if (g_prof_slots[i].name[0] == '\0')
    {
      strncpy(g_prof_slots[i].name, tag, sizeof(g_prof_slots[i].name) - 1U);
      return &g_prof_slots[i];
    }
    if (strcmp(g_prof_slots[i].name, tag) == 0)
    {
      return &g_prof_slots[i];
    }
  }
  return NULL;
}

void lv_profiler_begin(const char * tag)
{
  if (s_sp >= PROF_STACK_DEPTH)
  {
    g_prof_drops++;
    return;
  }
  s_stack[s_sp].tag = tag;
  s_stack[s_sp].start = DWT->CYCCNT;
  s_sp++;
}

void lv_profiler_end(const char * tag)
{
  lv_prof_slot_t * s;
  uint32_t dt;
  uint32_t idx;

  (void)tag;
  if (s_sp == 0U)
  {
    return;
  }
  s_sp--;
  dt = DWT->CYCCNT - s_stack[s_sp].start;
  s = slot_for(s_stack[s_sp].tag);
  if (s == NULL)
  {
    g_prof_drops++;
    return;
  }
  s->calls++;
  s->cycles += dt;
  if (s->calls == 1U || dt < s->cyc_min)
  {
    s->cyc_min = dt;
  }
  if (dt > s->cyc_max)
  {
    s->cyc_max = dt;
  }
  idx = s->ring_idx & (LV_PROF_RING_LEN - 1U);
  s->ring[idx] = dt;
  s->ring_phase[idx] = g_prof_phase;
  s->ring_idx++;
}

void lv_profiler_set_phase(uint8_t phase)
{
  g_prof_phase = phase;
}

int8_t lv_profiler_find(const char * name)
{
  uint32_t i;
  for (i = 0U; i < LV_PROF_SLOT_CNT; i++)
  {
    if (g_prof_slots[i].name[0] != '\0' && strcmp(g_prof_slots[i].name, name) == 0)
    {
      return (int8_t)i;
    }
  }
  return -1;
}
