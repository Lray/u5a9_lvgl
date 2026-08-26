#ifndef FREERTOS_RUNTIME_STATS_H
#define FREERTOS_RUNTIME_STATS_H

#include <stdint.h>

void perf_runtime_stats_init(void);
uint64_t perf_runtime_counter64(void);
uint32_t perf_idle_percent(void);

#endif /* FREERTOS_RUNTIME_STATS_H */
