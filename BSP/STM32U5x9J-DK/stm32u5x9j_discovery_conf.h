/**
  ******************************************************************************
  * @file    stm32u5x9j_discovery_conf.h
  * @author  MCD Application Team
  * @brief   STM32U5x9J_DISCOVERY board configuration file.
  *          Copied from stm32u5x9j_discovery_conf_template.h; only the entries
  *          used by the compiled BSP drivers (HSPI RAM, OSPI NOR) are kept.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32U5x9J_DISCOVERY_CONF_H
#define STM32U5x9J_DISCOVERY_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"

/* Usage of STM32U5x9J_DISCOVERY board */
#define USE_STM32U5x9J_DISCOVERY          1U

/* HSPI RAM interrupt priority */
#define BSP_HSPI_RAM_IT_PRIORITY         0x0FUL  /* Default is lowest priority level */
#define BSP_HSPI_RAM_DMA_IT_PRIORITY     0x0FUL  /* Default is lowest priority level */

#ifdef __cplusplus
}
#endif

#endif /* STM32U5x9J_DISCOVERY_CONF_H */
