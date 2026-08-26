#include "hspi_psram.h"
#include "mem_probe.h"

#define ARENA_ALIGN       32U
#define ARENA_HDR_SIZE    16U
#define ARENA_TAIL_SIZE   4U
#define ARENA_HDR_MAGIC   0x41524E31UL
#define ARENA_TAIL_CANARY 0xC3A5C33CUL

typedef struct
{
  uint32_t magic;
  uint32_t size;
  uint32_t head_canary;
  uint32_t rsvd;
} arena_hdr_t;

static uint8_t *s_cursor;

void *Hspi_Psram_Alloc(uint32_t size)
{
  if (size == 0U)
  {
    return 0L;
  }

  uint32_t used = (uint32_t)(s_cursor - (uint8_t *)(size_t)HSPI_PSRAM_ARENA_BASE);
  uint32_t pad = (0U - (used + ARENA_HDR_SIZE)) & (ARENA_ALIGN - 1U);
  uint32_t rounded = (size + (ARENA_ALIGN - 1U)) & ~(ARENA_ALIGN - 1U);
  uint32_t total = pad + ARENA_HDR_SIZE + rounded + ARENA_TAIL_SIZE;

  if ((used + total) > HSPI_PSRAM_ARENA_SIZE)
  {
    return 0L;
  }

  arena_hdr_t *hdr = (arena_hdr_t *)(void *)(s_cursor + pad);
  uint8_t *block = s_cursor + pad + ARENA_HDR_SIZE;

  hdr->magic = ARENA_HDR_MAGIC;
  hdr->size = rounded;
  hdr->head_canary = ARENA_HDR_MAGIC;
  hdr->rsvd = 0U;

  *(volatile uint32_t *)(void *)(block + rounded) = ARENA_TAIL_CANARY;
  __DSB();

  s_cursor += total;

  return (void *)(size_t)block;
}

void Hspi_Psram_ArenaInit(void)
{
  volatile uint32_t *base = (volatile uint32_t *)(size_t)HSPI_PSRAM_ARENA_BASE;

  s_cursor = (uint8_t *)(size_t)HSPI_PSRAM_ARENA_BASE;

  for (uint32_t i = 0U; i < 16U; i++)
  {
    base[i] = 0xA5965A3CUL ^ i;
  }
  __DSB();

  g_mem_probe.magic = MEM_PROBE_MAGIC;
  g_mem_probe.arena_init = 1U;

  g_mem_probe.arena_data_ok = 1U;
  for (uint32_t i = 0U; i < 16U; i++)
  {
    if (base[i] != (0xA5965A3CUL ^ i))
    {
      g_mem_probe.arena_data_ok = 0U;
      break;
    }
  }

  static const uint32_t sizes[6] = {16U, 48U, 100U, 4096U, 65536U, 1048576U};
  uint8_t *ptrs[6] = {0};
  uint32_t align_ok = 1U;

  for (uint32_t i = 0U; i < 6U; i++)
  {
    ptrs[i] = (uint8_t *)(size_t)Hspi_Psram_Alloc(sizes[i]);
    if (ptrs[i] == 0L)
    {
      align_ok = 0U;
      break;
    }
    if ((((uint32_t)(size_t)ptrs[i]) & (ARENA_ALIGN - 1U)) != 0U)
    {
      align_ok = 0U;
    }
    for (uint32_t b = 0U; b < sizes[i]; b++)
    {
      ptrs[i][b] = (uint8_t)(b + i);
    }
  }
  __DSB();

  g_mem_probe.arena_align_ok = align_ok;

  uint32_t canary_ok = 1U;
  for (uint32_t i = 0U; i < 6U; i++)
  {
    if (ptrs[i] == 0L)
    {
      continue;
    }
    const arena_hdr_t *hdr = (const arena_hdr_t *)(const void *)(ptrs[i] - ARENA_HDR_SIZE);
    const volatile uint32_t *tail =
        (const volatile uint32_t *)(const void *)(ptrs[i] + hdr->size);

    if ((hdr->magic != ARENA_HDR_MAGIC) || (hdr->head_canary != ARENA_HDR_MAGIC) ||
        (*tail != ARENA_TAIL_CANARY))
    {
      canary_ok = 0U;
      continue;
    }

    for (uint32_t b = 0U; b < sizes[i]; b++)
    {
      if (ptrs[i][b] != (uint8_t)(b + i))
      {
        canary_ok = 0U;
        break;
      }
    }
  }

  g_mem_probe.arena_canary_ok = canary_ok;
  g_mem_probe.arena_hwm = (uint32_t)(s_cursor - (uint8_t *)(size_t)HSPI_PSRAM_ARENA_BASE);
  g_mem_probe.done = 1U;
}
