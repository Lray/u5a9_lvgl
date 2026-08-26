#include "bench_mixed.h"
#include "lvgl.h"
#include "perf_profiler.h"

#define BENCH_MIXED_PHASE_SECS 25U

static lv_obj_t *s_rect;
static lv_obj_t *s_alpha;
static lv_obj_t *s_label;
static lv_obj_t *s_img;
static uint8_t s_img_px[80 * 80 * 2];
static lv_image_dsc_t s_img_dsc;
static uint32_t s_phase;

static void bench_mixed_x_cb(void *var, int32_t value)
{
  lv_obj_set_x((lv_obj_t *)var, value);
}

void Bench_Mixed_Setup(void)
{
  lv_obj_t *scr = lv_screen_active();

  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101010), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(scr, 0, 0);
  lv_obj_set_style_radius(scr, 0, 0);

  s_rect = lv_obj_create(scr);
  lv_obj_remove_style_all(s_rect);
  lv_obj_set_size(s_rect, 140, 140);
  lv_obj_set_style_bg_color(s_rect, lv_color_hex(0xC00000), 0);
  lv_obj_set_style_bg_opa(s_rect, LV_OPA_COVER, 0);

  s_alpha = lv_obj_create(scr);
  lv_obj_remove_style_all(s_alpha);
  lv_obj_set_size(s_alpha, 120, 120);
  lv_obj_set_style_bg_color(s_alpha, lv_color_hex(0x0070E0), 0);
  lv_obj_set_style_bg_opa(s_alpha, LV_OPA_50, 0);

  s_label = lv_label_create(scr);
  lv_label_set_text(s_label, "LVGL 9.3.0 direct 480x480 stride3072 M3");
  lv_obj_set_y(s_label, 12);

  for (uint32_t y = 0; y < 80U; y++)
  {
    for (uint32_t x = 0; x < 80U; x++)
    {
      uint16_t px = (y < 4U || y >= 76U) ? 0xFFFFU
                   : (x < 4U || x >= 76U) ? 0xFFFFU
                   : (uint16_t)((((x * 8U) & 0xF8U) << 8U) | ((y * 8U) & 0xF8U) | 0x1FU);
      s_img_px[(y * 80U + x) * 2U] = (uint8_t)(px & 0xFFU);
      s_img_px[(y * 80U + x) * 2U + 1U] = (uint8_t)(px >> 8U);
    }
  }
  s_img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
  s_img_dsc.header.w = 80U;
  s_img_dsc.header.h = 80U;
  s_img_dsc.header.stride = 160U;
  s_img_dsc.data = s_img_px;
  s_img_dsc.data_size = sizeof(s_img_px);
  s_img = lv_image_create(scr);
  lv_image_set_src(s_img, &s_img_dsc);
  lv_image_set_rotation(s_img, 300);
  lv_image_set_scale(s_img, 150);
  lv_obj_set_pos(s_img, 230, 300);

  lv_obj_set_y(s_rect, 200);
  lv_obj_set_y(s_alpha, 60);
  {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, bench_mixed_x_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_duration(&a, 2800);
    lv_anim_set_playback_duration(&a, 2800);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_var(&a, s_rect);
    lv_anim_set_values(&a, 20, 330);
    lv_anim_start(&a);
    lv_anim_set_var(&a, s_alpha);
    lv_anim_set_values(&a, 10, 350);
    lv_anim_start(&a);
    lv_anim_set_playback_duration(&a, 0);
    lv_anim_set_var(&a, s_label);
    lv_anim_set_values(&a, 480, -270);
    lv_anim_start(&a);
  }
}

void Bench_Mixed_Step(uint32_t ticks)
{
  uint32_t phase = (ticks / (BENCH_MIXED_PHASE_SECS * 1000U)) & 1U;

  if (phase != s_phase)
  {
    s_phase = phase;
    lv_profiler_set_phase((uint8_t)phase);
  }

  if (s_phase == 0U)
  {
    lv_obj_set_style_bg_color(lv_screen_active(),
                              ((ticks / 1000U) & 1U) ? lv_color_hex(0x203080) : lv_color_hex(0x802020),
                              0);
    lv_obj_invalidate(lv_screen_active());
  }
}