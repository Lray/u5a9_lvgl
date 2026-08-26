#include "bench_transform.h"
#include "lvgl.h"

static lv_obj_t *s_img;
static uint8_t s_img_px[80 * 80 * 2];
static lv_image_dsc_t s_img_dsc;

static void bench_transform_x_cb(void *var, int32_t value)
{
  lv_obj_set_x((lv_obj_t *)var, value);
}

void Bench_Transform_Setup(void)
{
  lv_obj_t *scr = lv_screen_active();

  for (uint32_t y = 0U; y < 80U; y++)
  {
    for (uint32_t x = 0U; x < 80U; x++)
    {
      uint16_t px = 0x841FU;
      if (((x / 8U) + (y / 8U)) & 1U)
      {
        px = 0x07E0U;
      }
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

  {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_exec_cb(&a, bench_transform_x_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_duration(&a, 1400);
    lv_anim_set_playback_duration(&a, 1400);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_var(&a, s_img);
    lv_anim_set_values(&a, 20, 330);
    lv_anim_start(&a);
  }
}

void Bench_Transform_Step(uint32_t ticks)
{
  (void)ticks;
}