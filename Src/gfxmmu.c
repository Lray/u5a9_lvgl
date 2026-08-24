/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gfxmmu.c
  * @brief   This file provides code for the configuration
  *          of the GFXMMU instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "gfxmmu.h"

#include "gfxmmu_lut.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

GFXMMU_HandleTypeDef hgfxmmu;

/* GFXMMU init function */
void MX_GFXMMU_Init(void)
{

  /* USER CODE BEGIN GFXMMU_Init 0 */

  /* USER CODE END GFXMMU_Init 0 */

  /* USER CODE BEGIN GFXMMU_Init 1 */

  /* USER CODE END GFXMMU_Init 1 */
  hgfxmmu.Instance = GFXMMU;
  hgfxmmu.Init.BlocksPerLine = GFXMMU_192BLOCKS;
  hgfxmmu.Init.DefaultValue = 0xFF000000;
  hgfxmmu.Init.Buffers.Buf0Address = 0x20000000;
  hgfxmmu.Init.Buffers.Buf1Address = 0x201A0000;
  hgfxmmu.Init.Buffers.Buf2Address = 0;
  hgfxmmu.Init.Buffers.Buf3Address = 0;
  hgfxmmu.Init.CachePrefetch.Activation = DISABLE;
  hgfxmmu.Init.Interrupts.Activation = ENABLE;
  hgfxmmu.Init.Interrupts.UsedInterrupts = GFXMMU_BUFFER0_OVERFLOW_IT|GFXMMU_AHB_MASTER_ERROR_IT;
  if (HAL_GFXMMU_Init(&hgfxmmu) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_GFXMMU_ConfigLut(&hgfxmmu, GFXMMU_LUT_FIRST, GFXMMU_LUT_SIZE, (uint32_t)gfxmmu_lut_config) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN GFXMMU_Init 2 */

  /* USER CODE END GFXMMU_Init 2 */

}

void HAL_GFXMMU_MspInit(GFXMMU_HandleTypeDef* gfxmmuHandle)
{

  if(gfxmmuHandle->Instance==GFXMMU)
  {
  /* USER CODE BEGIN GFXMMU_MspInit 0 */

  /* USER CODE END GFXMMU_MspInit 0 */
    /* GFXMMU clock enable */
    __HAL_RCC_GFXMMU_CLK_ENABLE();
  /* USER CODE BEGIN GFXMMU_MspInit 1 */
    HAL_NVIC_SetPriority(GFXMMU_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(GFXMMU_IRQn);
  /* USER CODE END GFXMMU_MspInit 1 */
  }
}

void HAL_GFXMMU_MspDeInit(GFXMMU_HandleTypeDef* gfxmmuHandle)
{

  if(gfxmmuHandle->Instance==GFXMMU)
  {
  /* USER CODE BEGIN GFXMMU_MspDeInit 0 */

  /* USER CODE END GFXMMU_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_GFXMMU_CLK_DISABLE();
  /* USER CODE BEGIN GFXMMU_MspDeInit 1 */

  /* USER CODE END GFXMMU_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
