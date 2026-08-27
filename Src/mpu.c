#include "mpu.h"
#include "stm32u5xx_hal.h"

typedef struct {
  uint32_t base;
  uint32_t limit;
  uint8_t attr_idx;
  uint8_t perm;
  uint8_t xn;
  uint8_t sh;
} mpu_region_cfg_t;

static const mpu_region_cfg_t mpu_regions[] = {
    {0x20000000UL, 0x2005FFFFUL, 0U, MPU_REGION_ALL_RW,
     MPU_INSTRUCTION_ACCESS_DISABLE, MPU_ACCESS_INNER_SHAREABLE},
    {0x201A0000UL, 0x201FFFFFUL, 0U, MPU_REGION_ALL_RW,
     MPU_INSTRUCTION_ACCESS_DISABLE, MPU_ACCESS_INNER_SHAREABLE},
    {0x20060000UL, 0x200BFFFFUL, 0U, MPU_REGION_ALL_RW,
     MPU_INSTRUCTION_ACCESS_DISABLE, MPU_ACCESS_INNER_SHAREABLE},
    {0x200D0000UL, 0x2010FFFFUL, 0U, MPU_REGION_ALL_RW,
     MPU_INSTRUCTION_ACCESS_DISABLE, MPU_ACCESS_INNER_SHAREABLE},
    {0x200C0000UL, 0x200C7FFFUL, 0U, MPU_REGION_ALL_RW,
     MPU_INSTRUCTION_ACCESS_DISABLE, MPU_ACCESS_INNER_SHAREABLE},
    {0x2019C000UL, 0x2019FFFFUL, 0U, MPU_REGION_PRIV_RW,
     MPU_INSTRUCTION_ACCESS_DISABLE, MPU_ACCESS_NOT_SHAREABLE},
};

void MX_MPU_Config(void) {
  MPU_Attributes_InitTypeDef attr = {0};
  MPU_Region_InitTypeDef region = {0};

  HAL_MPU_Disable();
  attr.Number = MPU_ATTRIBUTES_NUMBER0;
  attr.Attributes = INNER_OUTER(MPU_NOT_CACHEABLE);
  HAL_MPU_ConfigMemoryAttributes(&attr);

  for (uint32_t i = 0U; i < (sizeof(mpu_regions) / sizeof(mpu_regions[0]));
       i++) {
    region.Enable = MPU_REGION_ENABLE;
    region.Number = (uint8_t)i;
    region.BaseAddress = mpu_regions[i].base;
    region.LimitAddress = mpu_regions[i].limit;
    region.AttributesIndex = mpu_regions[i].attr_idx;
    region.AccessPermission = mpu_regions[i].perm;
    region.DisableExec = mpu_regions[i].xn;
    region.IsShareable = mpu_regions[i].sh;
    HAL_MPU_ConfigRegion(&region);
  }

  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
