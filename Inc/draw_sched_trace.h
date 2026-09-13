#ifndef DRAW_SCHED_TRACE_H
#define DRAW_SCHED_TRACE_H

#include <stdint.h>

#ifndef LV_DRAW_SCHED_TRACE
#define LV_DRAW_SCHED_TRACE 0
#endif

#if LV_DRAW_SCHED_TRACE

struct _lv_draw_task_t;

typedef enum {
    DRAW_SCHED_TRACE_RENDERER_SW,
    DRAW_SCHED_TRACE_RENDERER_DMA2D,
    DRAW_SCHED_TRACE_RENDERER_NEMA_GFX,
} draw_sched_trace_renderer_t;

void draw_sched_trace_reset(void);
void draw_sched_trace_scene_begin(uint32_t scene_id, const char * scene_name);
void draw_sched_trace_scene_end(void);
void draw_sched_trace_task_created(struct _lv_draw_task_t * task);
void draw_sched_trace_task_exec(const struct _lv_draw_task_t * task, draw_sched_trace_renderer_t renderer);
void draw_sched_trace_dump_csv(void);

#endif

#endif
