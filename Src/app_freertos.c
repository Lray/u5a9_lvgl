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
#include "main.h"
#include "FreeRTOS.h"
#include "lvgl.h"
#include "drivers/display/st_ltdc/lv_st_ltdc.h"
#include "perf_profiler.h"
#include "perf_uart.h"
#include "app_stats.h"
#include "freertos_runtime_stats.h"
#include "bench_runner.h"
#include "framebuffer.h"
#include "board_lcd.h"
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

uint8_t ucHeap[configTOTAL_HEAP_SIZE] __attribute__((section(".freertos_heap"), aligned(16)));

static uint32_t s_tick_snap;

m3_snapshot_t g_m3_stats;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void m3_snapshot(void);
/* USER CODE END FunctionPrototypes */

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void)
{
  __disable_irq();
  for (;;)
  {
  }
}
/* USER CODE END 5 */

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, char *pcTaskName)
{
  (void)xTask;
  (void)pcTaskName;
  __disable_irq();
  for (;;)
  {
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
  Perf_Uart_Start();
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
  lv_tick_set_cb(xTaskGetTickCount);
  lv_st_ltdc_create_direct(m_fb0_phys, m_fb1_phys, 0U);

  Bench_Scene_Setup();

  /* Infinite loop */
  while (1)
  {
    uint32_t ticks = xTaskGetTickCount();

    Bench_Scene_Step(ticks);

    lv_timer_handler();
    osDelay(2);

    if ((uint32_t)(xTaskGetTickCount() - s_tick_snap) >= 1000U)
    {
      s_tick_snap = xTaskGetTickCount();
      m3_snapshot();
    }
  }
  /* USER CODE END defaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static void m3_slot(const char * name, uint32_t * calls, uint32_t * cycles)
{
  int8_t i = lv_profiler_find(name);
  if (i >= 0)
  {
    *calls = g_prof_slots[i].calls;
    *cycles = g_prof_slots[i].cycles;
  }
}

static void m3_snapshot(void)
{
  m3_snapshot_t * s = &g_m3_stats;
  lv_mem_monitor_t mm;

  lv_mem_monitor(&mm);
  s->uptime_ms = xTaskGetTickCount();
  s->phase = g_prof_phase;
  s->refr_calls = 0U;
  s->refr_cycles = 0U;
  s->sync_calls = 0U;
  s->sync_cycles = 0U;
  s->wait_calls = 0U;
  s->wait_cycles = 0U;
  m3_slot("lv_display_refr_timer", &s->refr_calls, &s->refr_cycles);
  m3_slot("refr_sync_areas", &s->sync_calls, &s->sync_cycles);
  m3_slot("wait_for_flushing", &s->wait_calls, &s->wait_cycles);
  s->line_events = Board_LCD_GetLineEvents();
  s->idle_percent = perf_idle_percent();
  s->lv_used_max = (uint32_t)mm.max_used;
  s->rtos_heap_free_min = xPortGetMinimumEverFreeHeapSize();
  s->task_stack_hwm = uxTaskGetStackHighWaterMark(NULL);
  s->err_dsi = g_board_lcd_dsi_error_count;
  s->err_ltdc = g_board_lcd_ltdc_error_count;
}
/* USER CODE END Application */
