#include "perf_nema_alloc.h"

nema_alloc_stats_t g_nema_alloc_stats;

void *__real_tsi_malloc_pool(int pool, int size);
void __real_tsi_free(void *ptr);

void *__wrap_tsi_malloc_pool(int pool, int size)
{
  void *p = __real_tsi_malloc_pool(pool, size);

  if (p == 0L)
  {
    g_nema_alloc_stats.fails++;
    return p;
  }

  g_nema_alloc_stats.allocs++;
  if (size > 0)
  {
    g_nema_alloc_stats.bytes_cum += (uint32_t)size;
  }
  g_nema_alloc_stats.outst++;
  if (g_nema_alloc_stats.outst > g_nema_alloc_stats.outst_hwm)
  {
    g_nema_alloc_stats.outst_hwm = g_nema_alloc_stats.outst;
  }

  return p;
}

void __wrap_tsi_free(void *ptr)
{
  if (ptr != 0L)
  {
    g_nema_alloc_stats.frees++;
    if (g_nema_alloc_stats.outst != 0U)
    {
      g_nema_alloc_stats.outst--;
    }
  }
  __real_tsi_free(ptr);
}
