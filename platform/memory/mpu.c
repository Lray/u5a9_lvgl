#include "mpu.h"
#include "mem_probe.h"
#include "main.h"
#include "stm32u5xx_hal.h"

typedef struct
{
  uint32_t base;
  uint32_t limit;
  uint8_t  attr_idx;
  uint8_t  perm;
  uint8_t  xn;
  uint8_t  sh;
} mpu_region_cfg_t;

#define MPU_RB_CNT 8U

volatile mem_probe_t g_mem_probe;

static const mpu_region_cfg_t k_mpu_regions[MPU_RB_CNT] = {
    {0x20000000UL, 0x2005FFFFUL, 0U, MPU_REGION_ALL_RW, MPU_INSTRUCTION_ACCESS_DISABLE, MPU_ACCESS_INNER_SHAREABLE},
    {0x201A0000UL, 0x201FFFFFUL, 0U, MPU_REGION_ALL_RW, MPU_INSTRUCTION_ACCESS_DISABLE, MPU_ACCESS_INNER_SHAREABLE},
    {0x20060000UL, 0x200BFFFFUL, 0U, MPU_REGION_ALL_RW, MPU_INSTRUCTION_ACCESS_DISABLE, MPU_ACCESS_INNER_SHAREABLE},
    {0x200D0000UL, 0x2010FFFFUL, 0U, MPU_REGION_ALL_RW, MPU_INSTRUCTION_ACCESS_DISABLE, MPU_ACCESS_INNER_SHAREABLE},
    {0x200C0000UL, 0x200C7FFFUL, 0U, MPU_REGION_ALL_RW, MPU_INSTRUCTION_ACCESS_DISABLE, MPU_ACCESS_INNER_SHAREABLE},
    {0x90000000UL, 0x93FFFFFFUL, 1U, MPU_REGION_ALL_RO, MPU_INSTRUCTION_ACCESS_DISABLE, MPU_ACCESS_INNER_SHAREABLE},
    {0xA0000000UL, 0xA3FFFFFFUL, 0U, MPU_REGION_ALL_RW, MPU_INSTRUCTION_ACCESS_DISABLE, MPU_ACCESS_INNER_SHAREABLE},
    {0x2019C000UL, 0x2019FFFFUL, 0U, MPU_REGION_PRIV_RW, MPU_INSTRUCTION_ACCESS_DISABLE, MPU_ACCESS_NOT_SHAREABLE},
};

void MX_MPU_Config(void)
{
  MPU_Attributes_InitTypeDef attr = {0};
  MPU_Region_InitTypeDef region = {0};
  uint32_t rb_ok = 0U;

  g_mem_probe.magic = MEM_PROBE_MAGIC;
  g_mem_probe.dregion = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;

  if (g_mem_probe.dregion != MPU_RB_CNT)
  {
    Error_Handler();
  }

  HAL_MPU_Disable();

  attr.Number = MPU_ATTRIBUTES_NUMBER0;
  attr.Attributes = INNER_OUTER(MPU_NOT_CACHEABLE);
  HAL_MPU_ConfigMemoryAttributes(&attr);

  attr.Number = MPU_ATTRIBUTES_NUMBER1;
  attr.Attributes = MPU_NOT_CACHEABLE | OUTER(MPU_WRITE_BACK | MPU_NON_TRANSIENT | MPU_R_ALLOCATE);
  HAL_MPU_ConfigMemoryAttributes(&attr);

  for (uint32_t i = 0U; i < MPU_RB_CNT; i++)
  {
    region.Enable = MPU_REGION_ENABLE;
    region.Number = (uint8_t)i;
    region.BaseAddress = k_mpu_regions[i].base;
    region.LimitAddress = k_mpu_regions[i].limit;
    region.AttributesIndex = k_mpu_regions[i].attr_idx;
    region.AccessPermission = k_mpu_regions[i].perm;
    region.DisableExec = k_mpu_regions[i].xn;
    region.IsShareable = k_mpu_regions[i].sh;
    HAL_MPU_ConfigRegion(&region);
  }

  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

  for (uint32_t i = 0U; i < MPU_RB_CNT; i++)
  {
    uint32_t exp_rbar = (k_mpu_regions[i].base & 0xFFFFFFE0UL) |
                        ((uint32_t)k_mpu_regions[i].sh << MPU_RBAR_SH_Pos) |
                        ((uint32_t)k_mpu_regions[i].perm << MPU_RBAR_AP_Pos) |
                        ((uint32_t)k_mpu_regions[i].xn << MPU_RBAR_XN_Pos);
    uint32_t exp_rlar = (k_mpu_regions[i].limit & 0xFFFFFFE0UL) |
                        ((uint32_t)k_mpu_regions[i].attr_idx << MPU_RLAR_AttrIndx_Pos) |
                        MPU_RLAR_EN_Msk;

    MPU->RNR = i;
    if ((MPU->RBAR == exp_rbar) && (MPU->RLAR == exp_rlar))
    {
      rb_ok |= (1UL << i);
    }
  }

  g_mem_probe.mpu_rb_ok = rb_ok;
  g_mem_probe.mair0 = MPU->MAIR0;
  g_mem_probe.mair1 = MPU->MAIR1;
}
