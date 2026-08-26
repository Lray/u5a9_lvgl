#include "ospi_nor.h"
#include "mem_probe.h"

#define NOR_SELFCHECK_LEN 0x10000UL

static const uint32_t k_crc_table[16] = {
    0x00000000U, 0x1DB71064U, 0x3B6E20C8U, 0x26D930ACU,
    0x76DC4190U, 0x6B6B51F4U, 0x4DB26158U, 0x5005713CU,
    0xEDB88320U, 0xF00F9344U, 0xD6D6A3E8U, 0xCB61B38CU,
    0x9B64C2B0U, 0x86D3D2D4U, 0xA00AE278U, 0xBDBDF21CU};

uint32_t Ospi_Nor_Crc32(const void *data, uint32_t len)
{
  const uint8_t *p = (const uint8_t *)data;
  uint32_t crc = 0xFFFFFFFFUL;

  while (len--)
  {
    crc ^= (uint32_t)*p++;
    crc = (crc >> 4U) ^ k_crc_table[crc & 0x0FU];
    crc = (crc >> 4U) ^ k_crc_table[crc & 0x0FU];
  }

  return crc ^ 0xFFFFFFFFUL;
}

void Ospi_Nor_SelfCheck(void)
{
  const volatile uint8_t *nor = (const volatile uint8_t *)(size_t)OSPI_NOR_BASE;
  uint32_t nonzero = 0U;

  __DSB();
  g_mem_probe.nor_crc32 = Ospi_Nor_Crc32((const void *)nor, NOR_SELFCHECK_LEN);

  for (uint32_t i = 0U; i < (NOR_SELFCHECK_LEN / sizeof(uint32_t)); i++)
  {
    if (((const volatile uint32_t *)(size_t)OSPI_NOR_BASE)[i] != 0U)
    {
      nonzero++;
    }
  }
  g_mem_probe.nor_nonzero_words = nonzero;
}