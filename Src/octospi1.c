#include "octospi1.h"
#include "main.h"
#include "stm32u5x9j_discovery_ospi.h"

void MX_OCTOSPI1_Init(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  BSP_OSPI_NOR_Init_t init = {0};

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_OSPI;
  PeriphClkInit.OspiClockSelection = RCC_OSPICLKSOURCE_SYSCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

  init.InterfaceMode = BSP_OSPI_NOR_OPI_MODE;
  init.TransferRate  = BSP_OSPI_NOR_DTR_TRANSFER;

  g_xspi_probe.magic    = XSPI_PROBE_MAGIC;
  g_xspi_probe.nor_init = (uint32_t)BSP_OSPI_NOR_Init(0U, &init);
  g_xspi_probe.nor_mmp  = (uint32_t)BSP_OSPI_NOR_EnableMemoryMappedMode(0U);

  if ((g_xspi_probe.nor_init == BSP_ERROR_NONE) && (g_xspi_probe.nor_mmp == BSP_ERROR_NONE))
  {
    const volatile uint32_t *nor = (const volatile uint32_t *)(size_t)0x90000000UL;
    __DSB();
    g_xspi_probe.nor_word0 = nor[0];
    g_xspi_probe.nor_word1 = nor[1];
  }

  g_xspi_probe.done = 1U;
}
