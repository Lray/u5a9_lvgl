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
#include "lv_port_display.h"
#include "perf_profiler.h"
#include "framebuffer.h"
#include "board_lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define M3_PHASE_SECS 25U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

uint8_t ucHeap[configTOTAL_HEAP_SIZE] __attribute__((section(".freertos_heap"), aligned(16)));

static lv_obj_t *s_rect;
static lv_obj_t *s_alpha;
static lv_obj_t *s_label;
static uint32_t s_phase;
static uint32_t s_frame;
static uint32_t s_tick_snap;

typedef struct
{
  uint32_t uptime_ms;
  uint8_t  phase;
  uint32_t lvgl_frames;
  uint32_t swap_submit;
  uint32_t swap_done;
  uint32_t refr_calls;
  uint32_t refr_cycles;
  uint32_t sync_calls;
  uint32_t sync_cycles;
  uint32_t wait_calls;
  uint32_t wait_cycles;
  uint32_t est_copy_bytes;
  uint32_t line_events;
  uint32_t idle_percent;
  uint32_t lv_used_max;
  uint32_t rtos_heap_free_min;
  uint32_t task_stack_hwm;
  uint32_t err_fb;
  uint32_t err_dsi;
  uint32_t err_ltdc;
  uint32_t err_gfxmmu;
  uint32_t front_virtual;
} m3_snapshot_t;

m3_snapshot_t g_m3_stats;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 16 * 1024
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void m3_snapshot(void);
static void m3_anim_x_cb(void * var, int32_t value);
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
  lv_tick_set_cb(xTaskGetTickCount);
  lv_port_display_create();

  {
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101010), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    s_rect = lv_obj_create(scr);
    lv_obj_set_size(s_rect, 140, 140);
    lv_obj_set_style_bg_color(s_rect, lv_color_hex(0xC00000), 0);
    lv_obj_set_style_bg_opa(s_rect, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_rect, 0, 0);

    s_alpha = lv_obj_create(scr);
    lv_obj_set_size(s_alpha, 120, 120);
    lv_obj_set_style_bg_color(s_alpha, lv_color_hex(0x0070E0), 0);
    lv_obj_set_style_bg_opa(s_alpha, LV_OPA_50, 0);
    lv_obj_set_style_border_width(s_alpha, 0, 0);

    s_label = lv_label_create(scr);
    lv_label_set_text(s_label, "LVGL 9.3.0 direct 480x480 stride3072 M3");
    lv_obj_set_y(s_label, 12);

    /* time-based motion via the official animation engine (uniform speed,
     * independent of the rendered-frame jitter); vertical slots stay fixed */
    lv_obj_set_y(s_rect, 200);
    lv_obj_set_y(s_alpha, 60);
    {
      lv_anim_t a;
      lv_anim_init(&a);
      lv_anim_set_exec_cb(&a, m3_anim_x_cb);
      lv_anim_set_path_cb(&a, lv_anim_path_linear);
      lv_anim_set_duration(&a, 2800);
      lv_anim_set_playback_duration(&a, 2800);
      lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
      lv_anim_set_var(&a, s_rect);
      lv_anim_set_values(&a, 20, 330);
      lv_anim_start(&a);
      lv_anim_set_var(&a, s_alpha);
      lv_anim_set_values(&a, 10, 350);
      lv_anim_start(&a);
      lv_anim_set_playback_duration(&a, 0);
      lv_anim_set_var(&a, s_label);
      lv_anim_set_values(&a, 480, -270);
      lv_anim_start(&a);
    }
  }

  /* Infinite loop */
  while (1)
  {
    uint32_t ticks = xTaskGetTickCount();
    uint32_t phase = (ticks / (M3_PHASE_SECS * 1000U)) & 1U;

    if (phase != s_phase)
    {
      s_phase = phase;
      lv_profiler_set_phase((uint8_t)phase);
    }

    s_frame++;
    if (s_phase == 0U)
    {
      /* 100% dirty: repaint whole screen every frame; color steps at 1 Hz to
       * avoid a per-frame full-screen strobe */
      lv_obj_set_style_bg_color(lv_screen_active(),
                                ((ticks / 1000U) & 1U) ? lv_color_hex(0x203080) : lv_color_hex(0x802020),
                                0);
      lv_obj_invalidate(lv_screen_active());
    }

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
static void m3_anim_x_cb(void * var, int32_t value)
{
  lv_obj_set_x((lv_obj_t *)var, value);
}

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
  s->lvgl_frames = g_lv_port_stats.frame_seq;
  s->swap_submit = g_fb_swap_submit_seq;
  s->swap_done = g_fb_reload_done_seq;
  s->refr_calls = 0U;
  s->refr_cycles = 0U;
  s->sync_calls = 0U;
  s->sync_cycles = 0U;
  s->wait_calls = 0U;
  s->wait_cycles = 0U;
  m3_slot("lv_display_refr_timer", &s->refr_calls, &s->refr_cycles);
  m3_slot("refr_sync_areas", &s->sync_calls, &s->sync_cycles);
  m3_slot("wait_for_flushing", &s->wait_calls, &s->wait_cycles);
  s->est_copy_bytes = g_lv_port_stats.est_copy_bytes;
  s->line_events = Board_LCD_GetLineEvents();
  s->idle_percent = lv_os_get_idle_percent();
  s->lv_used_max = (uint32_t)mm.max_used;
  s->rtos_heap_free_min = xPortGetMinimumEverFreeHeapSize();
  s->task_stack_hwm = uxTaskGetStackHighWaterMark(NULL);
  s->err_fb = g_fb_swap_errors;
  s->err_dsi = g_board_lcd_dsi_error_count;
  s->err_ltdc = g_board_lcd_ltdc_error_count;
  s->err_gfxmmu = g_board_lcd_gfxmmu_error_count;
  s->front_virtual = g_fb_front_virtual;
}
/* USER CODE END Application */
