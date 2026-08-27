#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define FB_WIDTH 480U
#define FB_HEIGHT 480U
#define FB_PIXELS (FB_WIDTH * FB_HEIGHT)
#define FB_RGB565_BYTES (FB_PIXELS * sizeof(uint16_t))

extern uint16_t m_fb0_phys[];
extern uint16_t m_fb1_phys[];

void FB_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_H */
