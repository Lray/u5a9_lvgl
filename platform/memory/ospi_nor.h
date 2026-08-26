#ifndef OSPI_NOR_H
#define OSPI_NOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define OSPI_NOR_BASE 0x90000000UL

uint32_t Ospi_Nor_Crc32(const void *data, uint32_t len);
void Ospi_Nor_SelfCheck(void);

#ifdef __cplusplus
}
#endif

#endif /* OSPI_NOR_H */