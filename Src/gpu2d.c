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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
GPU2D_HandleTypeDef hgpu2d;

volatile uint32_t g_gpu2d_sramcached_readback;
volatile uint32_t g_gpu2d_error_count;

void MX_GPU2D_Init(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();

  /* DCACHE2 must stay off: clear SRAMCACHED and read it back (plan §6.3) */
  CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_SRAMCACHED);
  g_gpu2d_sramcached_readback = READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_SRAMCACHED);

  hgpu2d.Instance = GPU2D;
  if (HAL_GPU2D_Init(&hgpu2d) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_GPU2D_MspInit(GPU2D_HandleTypeDef *gpu2dHandle)
{
  if (gpu2dHandle->Instance == GPU2D)
  {
    /* USER CODE BEGIN GPU2D_MspInit 0 */

    /* USER CODE END GPU2D_MspInit 0 */
    __HAL_RCC_GPU2D_CLK_ENABLE();

    HAL_NVIC_SetPriority(GPU2D_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(GPU2D_IRQn);
    HAL_NVIC_SetPriority(GPU2D_ER_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(GPU2D_ER_IRQn);
    /* USER CODE BEGIN GPU2D_MspInit 1 */

    /* USER CODE END GPU2D_MspInit 1 */
  }
}

void HAL_GPU2D_MspDeInit(GPU2D_HandleTypeDef *gpu2dHandle)
{
  if (gpu2dHandle->Instance == GPU2D)
  {
    /* USER CODE BEGIN GPU2D_MspDeInit 0 */

    /* USER CODE END GPU2D_MspDeInit 0 */
    __HAL_RCC_GPU2D_CLK_DISABLE();

    HAL_NVIC_DisableIRQ(GPU2D_IRQn);
    HAL_NVIC_DisableIRQ(GPU2D_ER_IRQn);
    /* USER CODE BEGIN GPU2D_MspDeInit 1 */

    /* USER CODE END GPU2D_MspDeInit 1 */
  }
}
