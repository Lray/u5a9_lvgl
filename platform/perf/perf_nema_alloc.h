#ifndef PERF_NEMA_ALLOC_H
#define PERF_NEMA_ALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct
{
  volatile uint32_t bytes_cum;
  volatile uint32_t outst;
  volatile uint32_t outst_hwm;
  volatile uint32_t allocs;
  volatile uint32_t frees;
  volatile uint32_t fails;
} nema_alloc_stats_t;

extern nema_alloc_stats_t g_nema_alloc_stats;

#ifdef __cplusplus
}
#endif

#endif /* PERF_NEMA_ALLOC_H */
