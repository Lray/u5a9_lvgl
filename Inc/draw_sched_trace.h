#ifndef DRAW_SCHED_TRACE_H
#define DRAW_SCHED_TRACE_H

#include <stdint.h>

#if LV_DRAW_SCHED_TRACE

struct _lv_draw_task_t;

typedef enum {
    DRAW_SCHED_TRACE_CANDIDATE_SW = 1U << 0,
    DRAW_SCHED_TRACE_CANDIDATE_DMA2D = 1U << 1,
    DRAW_SCHED_TRACE_CANDIDATE_NEMA_GFX = 1U << 2,
} draw_sched_trace_candidate_t;

typedef enum {
    DRAW_SCHED_TRACE_DISPATCH_UNIT_SW,
    DRAW_SCHED_TRACE_DISPATCH_UNIT_DMA2D,
    DRAW_SCHED_TRACE_DISPATCH_UNIT_NEMA_GFX,
} draw_sched_trace_dispatch_unit_t;

void draw_sched_trace_reset(void);
void draw_sched_trace_scene_begin(uint32_t scene_id, const char * scene_name);
void draw_sched_trace_scene_end(void);
void draw_sched_trace_task_created(struct _lv_draw_task_t * task);
void draw_sched_trace_task_dispatched(const struct _lv_draw_task_t * task,
                                      draw_sched_trace_dispatch_unit_t dispatch_unit);
void draw_sched_trace_dump_csv(void);

#endif

#endif
