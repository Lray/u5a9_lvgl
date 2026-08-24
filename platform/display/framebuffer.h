#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32u5xx_hal.h"

#define FB_VIRT_BUFFER0 0x24000000UL
#define FB_VIRT_BUFFER1 0x24400000UL
#define FB_LOGICAL_HEIGHT 480UL
#define FB_STRIDE_BYTES 3072UL

extern uint16_t m_fb0_phys[];
extern uint16_t m_fb1_phys[];

extern volatile uint32_t g_fb_swap_submit_seq;
extern volatile uint32_t g_fb_reload_done_seq;
extern volatile uint32_t g_fb_swap_pending;
extern volatile uint32_t g_fb_swap_errors;
extern volatile uint32_t g_fb_front_virtual;
extern volatile uint32_t g_fb_back_virtual;
extern volatile uint32_t g_fb_submit_ts[8];
extern volatile uint32_t g_fb_done_ts[8];

void FB_Init(void);
HAL_StatusTypeDef FB_Submit(void);
uint32_t FB_GetBackVirtual(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_H */