#include "bench_runner.h"

#if defined(BENCH_SCENE_FULL_REPAINT)
#include "bench_full_repaint.h"
#elif defined(BENCH_SCENE_TRANSFORM)
#include "bench_transform.h"
#elif defined(BENCH_SCENE_ALPHA)
#include "bench_alpha_layers.h"
#elif defined(BENCH_SCENE_TEXT)
#include "bench_text_scroll.h"
#else
#include "bench_mixed.h"
#endif

void Bench_Scene_Setup(void)
{
#if defined(BENCH_SCENE_FULL_REPAINT)
  Bench_FullRepaint_Setup();
#elif defined(BENCH_SCENE_TRANSFORM)
  Bench_Transform_Setup();
#elif defined(BENCH_SCENE_ALPHA)
  Bench_Alpha_Setup();
#elif defined(BENCH_SCENE_TEXT)
  Bench_Text_Setup();
#else
  Bench_Mixed_Setup();
#endif
}

void Bench_Scene_Step(uint32_t ticks)
{
#if defined(BENCH_SCENE_FULL_REPAINT)
  Bench_FullRepaint_Step(ticks);
#elif defined(BENCH_SCENE_TRANSFORM)
  Bench_Transform_Step(ticks);
#elif defined(BENCH_SCENE_ALPHA)
  Bench_Alpha_Step(ticks);
#elif defined(BENCH_SCENE_TEXT)
  Bench_Text_Step(ticks);
#else
  Bench_Mixed_Step(ticks);
#endif
}