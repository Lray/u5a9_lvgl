/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : app_freertos.c
 * Description        : FreeRTOS applicative file
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
#include "app_freertos.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "drivers/display/st_ltdc/lv_st_ltdc.h"
#include "lv_demos.h"
#include "lvgl.h"
#if LV_USE_PROFILER && LV_USE_PROFILER_BUILTIN
#include "misc/lv_profiler_builtin_private.h"
#endif
#include "main.h"
#include "stm32u5x9j_discovery_ts.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

uint8_t ucHeap[configTOTAL_HEAP_SIZE]
    __attribute__((section(".freertos_heap"), aligned(16)));

/* LVGL direct-mode double framebuffers (LVGL STM32 LTDC driver doc) */
static uint8_t s_fb0[LCD_WIDTH * LCD_HEIGHT * LV_COLOR_DEPTH / 8]
    __attribute__((section(".fb0"), aligned(32)));
static uint8_t s_fb1[LCD_WIDTH * LCD_HEIGHT * LV_COLOR_DEPTH / 8]
    __attribute__((section(".fb1"), aligned(32)));

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 1024 * 16
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data);
/* USER CODE END FunctionPrototypes */

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void) {
  __disable_irq();
  for (;;) {
  }
}
/* USER CODE END 5 */

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, char *pcTaskName) {
  (void)xTask;
  (void)pcTaskName;
  __disable_irq();
  for (;;) {
  }
}
/* USER CODE END 4 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief Function implementing the defaultTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN defaultTask */
  lv_init();
  lv_tick_set_cb(HAL_GetTick);
#if LV_USE_PROFILER && LV_USE_PROFILER_BUILTIN
  lv_profiler_builtin_config_t profiler_config;
  lv_profiler_builtin_config_init(&profiler_config);
  profiler_config.flush_cb = NULL;
  lv_profiler_builtin_uninit();
  lv_profiler_builtin_init(&profiler_config);
#endif
  lv_display_t *display = lv_st_ltdc_create_direct(s_fb0, s_fb1, 0U);
  TS_Init_t touch_init = {
    .Width = LCD_WIDTH,
    .Height = LCD_HEIGHT,
    .Orientation = TS_ORIENTATION_PORTRAIT,
    .Accuracy = 0U
  };
  if (BSP_TS_Init(0U, &touch_init) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  lv_indev_t *touch = lv_indev_create();
  if (touch == NULL)
  {
    Error_Handler();
  }
  lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
  lv_indev_set_display(touch, display);
  lv_indev_set_read_cb(touch, touch_read_cb);
  lv_timer_set_period(lv_indev_get_read_timer(touch), 10U);
  lv_demo_benchmark();
  printf("Lvgl\n");
  /* Infinite loop */
  for (;;)
  {
    
    uint32_t delay_ms = lv_timer_handler();
    if (delay_ms == LV_NO_TIMER_READY) {
      delay_ms = LV_DEF_REFR_PERIOD;
    }
    osDelay(delay_ms == 0U ? 1U : delay_ms);
  }
  /* USER CODE END defaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
  TS_State_t state;
  (void)indev;

  /* LVGL supplies the last valid point on release; ignore BSP startup coordinates. */
  data->state = LV_INDEV_STATE_RELEASED;
  if (BSP_TS_GetState(0U, &state) == BSP_ERROR_NONE && state.TouchDetected != 0U &&
      state.TouchX < LCD_WIDTH && state.TouchY < LCD_HEIGHT)
  {
    data->point.x = (int32_t)state.TouchX;
    data->point.y = (int32_t)state.TouchY;
    data->state = LV_INDEV_STATE_PRESSED;
  }
}
/* USER CODE END Application */

