#include "bench_text_scroll.h"
#include "lvgl.h"

static lv_obj_t *s_label;

void Bench_Text_Setup(void)
{
  s_label = lv_label_create(lv_screen_active());
  lv_label_set_text(s_label, "The quick brown fox jumps over the lazy dog 0123456789 "
                             "LVGL 9.3.0 benchmark text scroll scene - fixed string, "
                             "continuous horizontal and vertical motion");
  lv_obj_set_style_text_font(s_label, &lv_font_montserrat_14, 0);
  lv_obj_set_pos(s_label, 0, 200);
}

void Bench_Text_Step(uint32_t ticks)
{
  lv_obj_set_x(s_label, 480 - (int32_t)((ticks / 8U) % 960U));
  lv_obj_set_y(s_label, 200 + (int32_t)((ticks / 16U) % 80U));
  lv_obj_invalidate(s_label);
}