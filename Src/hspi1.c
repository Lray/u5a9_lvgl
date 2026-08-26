#include "hspi1.h"
#include "main.h"
#include "stm32u5x9j_discovery_hspi.h"

volatile xspi_probe_t g_xspi_probe;

static uint32_t hspi1_psram_pattern_check(void)
{
  volatile uint32_t *base = (volatile uint32_t *)(size_t)HSPI1_BASE;
  static const uint32_t offsets[2] = {0x00000000UL, 0x00800000UL};

  for (uint32_t w = 0U; w < 2U; w++)
  {
    volatile uint32_t *p = base + (offsets[w] / sizeof(uint32_t));
    for (uint32_t i = 0U; i < 64U; i++)
    {
      p[i] = 0xA5965A3CUL ^ i;
    }
  }

  __DSB();

  for (uint32_t w = 0U; w < 2U; w++)
  {
    volatile uint32_t *p = base + (offsets[w] / sizeof(uint32_t));
    for (uint32_t i = 0U; i < 64U; i++)
    {
      if (p[i] != (0xA5965A3CUL ^ i))
      {
        return offsets[w] + (i * sizeof(uint32_t));
      }
    }
  }

  return 0xFFFFFFFFUL;
}

void MX_HSPI1_Init(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  BSP_HSPI_RAM_Cfg_t cfg = {0};

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_HSPI;
  PeriphClkInit.HspiClockSelection = RCC_HSPICLKSOURCE_SYSCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

  cfg.LatencyType      = BSP_HSPI_RAM_FIXED_LATENCY;
  cfg.ReadLatencyCode  = BSP_HSPI_RAM_READ_LATENCY_7;
  cfg.WriteLatencyCode = BSP_HSPI_RAM_WRITE_LATENCY_7;
  cfg.BurstType        = BSP_HSPI_RAM_LINEAR_BURST;
  cfg.BurstLength      = BSP_HSPI_RAM_BURST_64_BYTES;
  cfg.IOMode           = BSP_HSPI_RAM_IO_X16_MODE;

  g_xspi_probe.magic     = XSPI_PROBE_MAGIC;
  g_xspi_probe.hspi_init = (uint32_t)BSP_HSPI_RAM_Init(0U, &cfg);
  g_xspi_probe.hspi_mmp  = (uint32_t)BSP_HSPI_RAM_EnableMemoryMappedMode(0U);

  if ((g_xspi_probe.hspi_init == BSP_ERROR_NONE) && (g_xspi_probe.hspi_mmp == BSP_ERROR_NONE))
  {
    g_xspi_probe.psram_misoff = hspi1_psram_pattern_check();
    g_xspi_probe.psram_cmp = (g_xspi_probe.psram_misoff == 0xFFFFFFFFUL) ? 0U : 1U;
  }
}
