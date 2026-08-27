#include "framebuffer.h"
#include <string.h>

uint16_t m_fb0_phys[FB_PIXELS] __attribute__((section(".fb0_phys"), aligned(16)));
uint16_t m_fb1_phys[FB_PIXELS] __attribute__((section(".fb1_phys"), aligned(16)));

void FB_Init(void)
{
  memset(m_fb0_phys, 0, sizeof(m_fb0_phys));
  memset(m_fb1_phys, 0, sizeof(m_fb1_phys));
}
