#include "bench_alpha_layers.h"
#include "lvgl.h"

#define BENCH_ALPHA_LAYERS 4U

static lv_obj_t *s_layers[BENCH_ALPHA_LAYERS];

void Bench_Alpha_Setup(void)
{
  static const uint32_t colors[BENCH_ALPHA_LAYERS] = {0xFF0000U, 0x00FF00U, 0x0000FFU, 0xFFFF00U};

  for (uint32_t i = 0U; i < BENCH_ALPHA_LAYERS; i++)
  {
    s_layers[i] = lv_obj_create(lv_screen_active());
    lv_obj_set_size(s_layers[i], 480, 480);
    lv_obj_set_pos(s_layers[i], 0, 0);
    lv_obj_set_style_bg_color(s_layers[i], lv_color_hex(colors[i]), 0);
    lv_obj_set_style_bg_opa(s_layers[i], LV_OPA_30, 0);
    lv_obj_set_style_border_width(s_layers[i], 0, 0);
    lv_obj_set_style_radius(s_layers[i], 0, 0);
  }
}

void Bench_Alpha_Step(uint32_t ticks)
{
  for (uint32_t i = 0U; i < BENCH_ALPHA_LAYERS; i++)
  {
    lv_obj_set_style_bg_opa(s_layers[i], 10U + (uint8_t)(((ticks / 50U) + (i * 17U)) & 0x3FU), 0);
    lv_obj_invalidate(s_layers[i]);
  }
}