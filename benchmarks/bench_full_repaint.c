#include "bench_full_repaint.h"
#include "lvgl.h"

void Bench_FullRepaint_Setup(void)
{
}

void Bench_FullRepaint_Step(uint32_t ticks)
{
  (void)ticks;
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x203080), 0);
  lv_obj_invalidate(lv_screen_active());
}