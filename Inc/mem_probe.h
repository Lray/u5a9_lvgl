#ifndef MEM_PROBE_H
#define MEM_PROBE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32u5xx_hal.h"

#define MEM_PROBE_MAGIC 0x4D454D31UL

typedef struct
{
  uint32_t magic;
  uint32_t dregion;
  uint32_t mpu_rb_ok;
  uint32_t mair0;
  uint32_t mair1;
  uint32_t arena_init;
  uint32_t arena_align_ok;
  uint32_t arena_data_ok;
  uint32_t arena_canary_ok;
  uint32_t arena_hwm;
  uint32_t diag_run;
  uint32_t diag_ok;
  uint32_t diag_fail_addr;
  uint32_t diag_ms;
  uint32_t done;
} mem_probe_t;

extern volatile mem_probe_t g_mem_probe;

#ifdef __cplusplus
}
#endif

#endif /* MEM_PROBE_H */
