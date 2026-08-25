#ifndef __GPU2D_H
#define __GPU2D_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32u5xx_hal.h"

extern GPU2D_HandleTypeDef hgpu2d;

void MX_GPU2D_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __GPU2D_H */
