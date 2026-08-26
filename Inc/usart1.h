#ifndef USART1_H
#define USART1_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32u5xx_hal.h"

extern UART_HandleTypeDef huart1;

void MX_USART1_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* USART1_H */
