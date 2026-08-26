#ifndef HSPI_PSRAM_H
#define HSPI_PSRAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define HSPI_PSRAM_ARENA_BASE 0xA0000000UL
#define HSPI_PSRAM_ARENA_SIZE 0x02000000UL

void Hspi_Psram_ArenaInit(void);
void *Hspi_Psram_Alloc(uint32_t size);

#if defined(PSRAM_FULL_DIAG)
void Hspi_Psram_FullDiag(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* HSPI_PSRAM_H */
