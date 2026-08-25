/**
 * @file perf_gpio.c
 *
 * Optional probe-point module (docs/00_plan.md §5.2/§7 M3): default off,
 * compiles in both states, never participates in milestone acceptance.
 */

#include "perf_gpio.h"
#include "stm32u5xx_hal.h"

#if PERF_GPIO_EN

#include "gpio.h"

void perf_gpio_mark(uint8_t level)
{
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

#else

void perf_gpio_mark(uint8_t level)
{
  (void)level;
}

#endif /* PERF_GPIO_EN */
