#ifndef XSPI_PROBE_H
#define XSPI_PROBE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32u5xx_hal.h"

#define XSPI_PROBE_MAGIC 0x58535031UL

typedef struct
{
  uint32_t magic;
  uint32_t hspi_init;
  uint32_t hspi_mmp;
  uint32_t psram_cmp;
  uint32_t psram_misoff;
  uint32_t nor_init;
  uint32_t nor_mmp;
  uint32_t nor_word0;
  uint32_t nor_word1;
  uint32_t done;
} xspi_probe_t;

extern volatile xspi_probe_t g_xspi_probe;

#ifdef __cplusplus
}
#endif

#endif /* XSPI_PROBE_H */
