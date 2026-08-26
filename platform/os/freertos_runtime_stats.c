/**
 * @file freertos_runtime_stats.c
 *
 * FreeRTOS run-time stats counter (docs/00_plan.md §5.2/§6.2): DWT CYCCNT
 * extended to 64-bit with unsigned modular deltas.
 */

#include "freertos_runtime_stats.h"
#include "stm32u5xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

static volatile uint32_t s_last;
static volatile uint64_t s_total;

void perf_runtime_stats_init(void)
{
  s_last = DWT->CYCCNT;
  s_total = 0U;
}

uint64_t perf_runtime_counter64(void)
{
  uint32_t raw = DWT->CYCCNT;

  s_total += (uint32_t)(raw - s_last);
  s_last = raw;
  return s_total;
}

uint32_t perf_idle_percent(void)
{
  uint32_t idle = ulTaskGetIdleRunTimeCounter();
  uint64_t total = perf_runtime_counter64();

  if (total == 0U)
  {
    return 0U;
  }

  return (uint32_t)(((uint64_t)idle * 100U) / total);
}
