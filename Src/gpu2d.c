/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : gpu2d.c
  * @brief          : GPU2D (NeoChrom) init: M4 per docs/00_plan.md §7 M4.
  *                   DCACHE2 stays disabled; SRAMCACHED cleared and read back
  *                   before the GPU2D is enabled.
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "gpu2d.h"

/* USER CODE BEGIN 0 */
volatile uint32_t g_gpu2d_sramcached_readback;
/* USER CODE END 0 */

GPU2D_HandleTypeDef hgpu2d;

/* GPU2D init function */
void MX_GPU2D_Init(void)
{

  /* USER CODE BEGIN GPU2D_Init 0 */
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_SRAMCACHED);
  g_gpu2d_sramcached_readback = READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_SRAMCACHED);
  /* USER CODE END GPU2D_Init 0 */

  /* USER CODE BEGIN GPU2D_Init 1 */

  /* USER CODE END GPU2D_Init 1 */
  hgpu2d.Instance = GPU2D;
  if (HAL_GPU2D_Init(&hgpu2d) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN GPU2D_Init 2 */

  /* USER CODE END GPU2D_Init 2 */

}

void HAL_GPU2D_MspInit(GPU2D_HandleTypeDef* gpu2dHandle)
{

  if(gpu2dHandle->Instance==GPU2D)
  {
  /* USER CODE BEGIN GPU2D_MspInit 0 */

  /* USER CODE END GPU2D_MspInit 0 */
    /* GPU2D clock enable */
    __HAL_RCC_GPU2D_CLK_ENABLE();
    __HAL_RCC_DCACHE2_CLK_ENABLE();

    /* GPU2D interrupt Init */
    HAL_NVIC_SetPriority(GPU2D_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(GPU2D_IRQn);
    HAL_NVIC_SetPriority(GPU2D_ER_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(GPU2D_ER_IRQn);
  /* USER CODE BEGIN GPU2D_MspInit 1 */

  /* USER CODE END GPU2D_MspInit 1 */
  }
}

void HAL_GPU2D_MspDeInit(GPU2D_HandleTypeDef* gpu2dHandle)
{

  if(gpu2dHandle->Instance==GPU2D)
  {
  /* USER CODE BEGIN GPU2D_MspDeInit 0 */

  /* USER CODE END GPU2D_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_GPU2D_CLK_DISABLE();
    __HAL_RCC_DCACHE2_CLK_DISABLE();

    /* GPU2D interrupt Deinit */
    HAL_NVIC_DisableIRQ(GPU2D_IRQn);
    HAL_NVIC_DisableIRQ(GPU2D_ER_IRQn);
  /* USER CODE BEGIN GPU2D_MspDeInit 1 */

  /* USER CODE END GPU2D_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
