#include "draw_sched_trace.h"

#if LV_DRAW_SCHED_TRACE

#include "draw/lv_draw_private.h"
#include "misc/lv_area_private.h"
#include "misc/lv_log.h"
#include "stdlib/lv_sprintf.h"

#include <string.h>

#define TRACE_SCENE_COUNT 16U
#if LV_USE_3DTEXTURE
#define TRACE_TASK_TYPE_COUNT ((uint32_t)LV_DRAW_TASK_TYPE_3D + 1U)
#elif LV_USE_VECTOR_GRAPHIC
#define TRACE_TASK_TYPE_COUNT ((uint32_t)LV_DRAW_TASK_TYPE_VECTOR + 1U)
#else
#define TRACE_TASK_TYPE_COUNT ((uint32_t)LV_DRAW_TASK_TYPE_MASK_BITMAP + 1U)
#endif
#define TRACE_RENDERER_COUNT 3U
#define TRACE_AREA_BUCKET_COUNT 8U
#define TRACE_SCENE_NONE UINT8_MAX

typedef struct {
    uint32_t task_count;
    uint64_t pixel_count;
} trace_counter_t;

static trace_counter_t counters[TRACE_SCENE_COUNT][TRACE_TASK_TYPE_COUNT][TRACE_RENDERER_COUNT]
                               [TRACE_AREA_BUCKET_COUNT];
static const char * scene_names[TRACE_SCENE_COUNT];
static uint8_t current_scene = TRACE_SCENE_NONE;

static uint32_t area_bucket(uint32_t pixels)
{
    if(pixels == 0U) return 0U;
    if(pixels <= 64U) return 1U;
    if(pixels <= 256U) return 2U;
    if(pixels <= 1024U) return 3U;
    if(pixels <= 4096U) return 4U;
    if(pixels <= 16384U) return 5U;
    if(pixels <= 65536U) return 6U;
    return 7U;
}

static const char * area_bucket_name(uint32_t bucket)
{
    static const char * const names[TRACE_AREA_BUCKET_COUNT] = {
        "0", "1-64", "65-256", "257-1024", "1025-4096", "4097-16384", "16385-65536", ">65536"
    };
    return names[bucket];
}

static const char * renderer_name(uint32_t renderer)
{
    static const char * const names[TRACE_RENDERER_COUNT] = {"SW", "DMA2D", "NEMA"};
    return names[renderer];
}

static const char * task_type_name(lv_draw_task_type_t type)
{
    switch(type) {
        case LV_DRAW_TASK_TYPE_NONE: return "NONE";
        case LV_DRAW_TASK_TYPE_FILL: return "FILL";
        case LV_DRAW_TASK_TYPE_BORDER: return "BORDER";
        case LV_DRAW_TASK_TYPE_BOX_SHADOW: return "BOX_SHADOW";
        case LV_DRAW_TASK_TYPE_LETTER: return "LETTER";
        case LV_DRAW_TASK_TYPE_LABEL: return "LABEL";
        case LV_DRAW_TASK_TYPE_IMAGE: return "IMAGE";
        case LV_DRAW_TASK_TYPE_LAYER: return "LAYER";
        case LV_DRAW_TASK_TYPE_LINE: return "LINE";
        case LV_DRAW_TASK_TYPE_ARC: return "ARC";
        case LV_DRAW_TASK_TYPE_TRIANGLE: return "TRIANGLE";
        case LV_DRAW_TASK_TYPE_MASK_RECTANGLE: return "MASK_RECTANGLE";
        case LV_DRAW_TASK_TYPE_MASK_BITMAP: return "MASK_BITMAP";
#if LV_USE_VECTOR_GRAPHIC
        case LV_DRAW_TASK_TYPE_VECTOR: return "VECTOR";
#endif
#if LV_USE_3DTEXTURE
        case LV_DRAW_TASK_TYPE_3D: return "3D";
#endif
    }
    return "UNKNOWN";
}

void draw_sched_trace_reset(void)
{
    memset(counters, 0, sizeof(counters));
    memset(scene_names, 0, sizeof(scene_names));
    current_scene = TRACE_SCENE_NONE;
}

void draw_sched_trace_scene_begin(uint32_t scene_id, const char * scene_name)
{
    if(scene_id >= TRACE_SCENE_COUNT) {
        current_scene = TRACE_SCENE_NONE;
        return;
    }

    scene_names[scene_id] = scene_name;
    current_scene = (uint8_t)scene_id;
}

void draw_sched_trace_scene_end(void)
{
    current_scene = TRACE_SCENE_NONE;
}

void draw_sched_trace_task_created(lv_draw_task_t * task)
{
    task->trace_scene_id = current_scene;
}

void draw_sched_trace_task_exec(const lv_draw_task_t * task, draw_sched_trace_renderer_t renderer)
{
    uint32_t pixels = 0U;
    lv_area_t effective_area;

    if(task->trace_scene_id >= TRACE_SCENE_COUNT || (uint32_t)task->type >= TRACE_TASK_TYPE_COUNT ||
       (uint32_t)renderer >= TRACE_RENDERER_COUNT) {
        return;
    }

    if(lv_area_intersect(&effective_area, &task->area, &task->clip_area)) {
        pixels = lv_area_get_size(&effective_area);
    }

    trace_counter_t * counter = &counters[task->trace_scene_id][task->type][renderer][area_bucket(pixels)];
    counter->task_count++;
    counter->pixel_count += pixels;
}

void draw_sched_trace_dump_csv(void)
{
    LV_LOG("DRAW_SCHED_TRACE_BEGIN\r\n");
    LV_LOG("scene_id,scene_name,task_type,renderer,area_bucket,task_count,pixel_count\r\n");

    for(uint32_t scene = 0U; scene < TRACE_SCENE_COUNT; scene++) {
        if(scene_names[scene] == NULL) continue;

        for(uint32_t type = 0U; type < TRACE_TASK_TYPE_COUNT; type++) {
            for(uint32_t renderer = 0U; renderer < TRACE_RENDERER_COUNT; renderer++) {
                for(uint32_t bucket = 0U; bucket < TRACE_AREA_BUCKET_COUNT; bucket++) {
                    const trace_counter_t * counter = &counters[scene][type][renderer][bucket];
                    if(counter->task_count == 0U) continue;

                    char pixel_count[21];
                    lv_snprintf(pixel_count, sizeof(pixel_count), "%" LV_PRIu64, counter->pixel_count);
                    LV_LOG("%lu,%s,%s,%s,%s,%lu,%s\r\n",
                           (unsigned long)scene,
                           scene_names[scene],
                           task_type_name((lv_draw_task_type_t)type),
                           renderer_name(renderer),
                           area_bucket_name(bucket),
                           (unsigned long)counter->task_count,
                           pixel_count);
                }
            }
        }
    }

    LV_LOG("DRAW_SCHED_TRACE_END\r\n");
}

#endif
