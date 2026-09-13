#include "draw_sched_trace.h"

#if LV_DRAW_SCHED_TRACE

#include "draw/lv_draw_image.h"
#include "draw/lv_draw_private.h"
#include "draw/lv_draw_rect.h"
#include "misc/lv_area_private.h"
#include "misc/lv_log.h"
#include "stdlib/lv_sprintf.h"

#include <string.h>

#define TRACE_SCENE_COUNT 16U
#define TRACE_CANDIDATE_MASK_COUNT 8U
#define TRACE_DISPATCH_UNIT_COUNT 3U
#define TRACE_AREA_BUCKET_COUNT 8U
#define TRACE_SCENE_NONE UINT8_MAX

typedef enum {
    TRACE_TASK_CLASS_NONE,
    TRACE_TASK_CLASS_FILL_OPAQUE_SIMPLE,
    TRACE_TASK_CLASS_FILL_ALPHA_SIMPLE,
    TRACE_TASK_CLASS_FILL_OTHER,
    TRACE_TASK_CLASS_BORDER,
    TRACE_TASK_CLASS_BOX_SHADOW,
    TRACE_TASK_CLASS_LETTER,
    TRACE_TASK_CLASS_LABEL,
    TRACE_TASK_CLASS_IMAGE_COPY_OPAQUE,
    TRACE_TASK_CLASS_IMAGE_ALPHA,
    TRACE_TASK_CLASS_IMAGE_TRANSFORMED,
    TRACE_TASK_CLASS_IMAGE_OTHER,
    TRACE_TASK_CLASS_LAYER,
    TRACE_TASK_CLASS_LINE,
    TRACE_TASK_CLASS_ARC,
    TRACE_TASK_CLASS_TRIANGLE,
    TRACE_TASK_CLASS_MASK_RECTANGLE,
    TRACE_TASK_CLASS_MASK_BITMAP,
#if LV_USE_VECTOR_GRAPHIC
    TRACE_TASK_CLASS_VECTOR,
#endif
#if LV_USE_3DTEXTURE
    TRACE_TASK_CLASS_3D,
#endif
    TRACE_TASK_CLASS_COUNT
} trace_task_class_t;

static uint32_t task_counters[TRACE_SCENE_COUNT][TRACE_TASK_CLASS_COUNT][TRACE_CANDIDATE_MASK_COUNT]
                             [TRACE_DISPATCH_UNIT_COUNT][TRACE_AREA_BUCKET_COUNT];
static uint64_t pixel_counters[TRACE_SCENE_COUNT][TRACE_TASK_CLASS_COUNT][TRACE_CANDIDATE_MASK_COUNT]
                              [TRACE_DISPATCH_UNIT_COUNT][TRACE_AREA_BUCKET_COUNT];
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

static const char * candidate_units_name(uint32_t candidate_mask)
{
    static const char * const names[TRACE_CANDIDATE_MASK_COUNT] = {
        "NONE", "SW", "DMA2D", "SW|DMA2D", "NEMA", "SW|NEMA", "DMA2D|NEMA", "SW|DMA2D|NEMA"
    };
    return names[candidate_mask];
}

static const char * dispatch_unit_name(uint32_t dispatch_unit)
{
    static const char * const names[TRACE_DISPATCH_UNIT_COUNT] = {"SW", "DMA2D", "NEMA"};
    return names[dispatch_unit];
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

static trace_task_class_t task_class(const lv_draw_task_t * task)
{
    switch(task->type) {
        case LV_DRAW_TASK_TYPE_FILL: {
                const lv_draw_fill_dsc_t * dsc = task->draw_dsc;
                if(dsc->radius != 0 || dsc->grad.dir != LV_GRAD_DIR_NONE) return TRACE_TASK_CLASS_FILL_OTHER;
                return dsc->opa >= LV_OPA_MAX ? TRACE_TASK_CLASS_FILL_OPAQUE_SIMPLE :
                       TRACE_TASK_CLASS_FILL_ALPHA_SIMPLE;
            }
        case LV_DRAW_TASK_TYPE_IMAGE: {
                const lv_draw_image_dsc_t * dsc = task->draw_dsc;
                if(dsc->rotation != 0 || dsc->scale_x != LV_SCALE_NONE || dsc->scale_y != LV_SCALE_NONE ||
                   dsc->skew_x != 0 || dsc->skew_y != 0) {
                    return TRACE_TASK_CLASS_IMAGE_TRANSFORMED;
                }

                if(dsc->clip_radius != 0 || dsc->bitmap_mask_src != NULL || dsc->sup != NULL || dsc->tile != 0 ||
                   dsc->blend_mode != LV_BLEND_MODE_NORMAL || dsc->recolor_opa > LV_OPA_MIN ||
                   lv_image_src_get_type(dsc->src) != LV_IMAGE_SRC_VARIABLE) {
                    return TRACE_TASK_CLASS_IMAGE_OTHER;
                }

                return dsc->opa >= LV_OPA_MAX && !lv_color_format_has_alpha(dsc->header.cf) ?
                       TRACE_TASK_CLASS_IMAGE_COPY_OPAQUE : TRACE_TASK_CLASS_IMAGE_ALPHA;
            }
        case LV_DRAW_TASK_TYPE_NONE: return TRACE_TASK_CLASS_NONE;
        case LV_DRAW_TASK_TYPE_BORDER: return TRACE_TASK_CLASS_BORDER;
        case LV_DRAW_TASK_TYPE_BOX_SHADOW: return TRACE_TASK_CLASS_BOX_SHADOW;
        case LV_DRAW_TASK_TYPE_LETTER: return TRACE_TASK_CLASS_LETTER;
        case LV_DRAW_TASK_TYPE_LABEL: return TRACE_TASK_CLASS_LABEL;
        case LV_DRAW_TASK_TYPE_LAYER: return TRACE_TASK_CLASS_LAYER;
        case LV_DRAW_TASK_TYPE_LINE: return TRACE_TASK_CLASS_LINE;
        case LV_DRAW_TASK_TYPE_ARC: return TRACE_TASK_CLASS_ARC;
        case LV_DRAW_TASK_TYPE_TRIANGLE: return TRACE_TASK_CLASS_TRIANGLE;
        case LV_DRAW_TASK_TYPE_MASK_RECTANGLE: return TRACE_TASK_CLASS_MASK_RECTANGLE;
        case LV_DRAW_TASK_TYPE_MASK_BITMAP: return TRACE_TASK_CLASS_MASK_BITMAP;
#if LV_USE_VECTOR_GRAPHIC
        case LV_DRAW_TASK_TYPE_VECTOR: return TRACE_TASK_CLASS_VECTOR;
#endif
#if LV_USE_3DTEXTURE
        case LV_DRAW_TASK_TYPE_3D: return TRACE_TASK_CLASS_3D;
#endif
    }
    return TRACE_TASK_CLASS_NONE;
}

static lv_draw_task_type_t task_class_type(trace_task_class_t task_class_value)
{
    switch(task_class_value) {
        case TRACE_TASK_CLASS_FILL_OPAQUE_SIMPLE:
        case TRACE_TASK_CLASS_FILL_ALPHA_SIMPLE:
        case TRACE_TASK_CLASS_FILL_OTHER:
            return LV_DRAW_TASK_TYPE_FILL;
        case TRACE_TASK_CLASS_IMAGE_COPY_OPAQUE:
        case TRACE_TASK_CLASS_IMAGE_ALPHA:
        case TRACE_TASK_CLASS_IMAGE_TRANSFORMED:
        case TRACE_TASK_CLASS_IMAGE_OTHER:
            return LV_DRAW_TASK_TYPE_IMAGE;
        case TRACE_TASK_CLASS_NONE: return LV_DRAW_TASK_TYPE_NONE;
        case TRACE_TASK_CLASS_BORDER: return LV_DRAW_TASK_TYPE_BORDER;
        case TRACE_TASK_CLASS_BOX_SHADOW: return LV_DRAW_TASK_TYPE_BOX_SHADOW;
        case TRACE_TASK_CLASS_LETTER: return LV_DRAW_TASK_TYPE_LETTER;
        case TRACE_TASK_CLASS_LABEL: return LV_DRAW_TASK_TYPE_LABEL;
        case TRACE_TASK_CLASS_LAYER: return LV_DRAW_TASK_TYPE_LAYER;
        case TRACE_TASK_CLASS_LINE: return LV_DRAW_TASK_TYPE_LINE;
        case TRACE_TASK_CLASS_ARC: return LV_DRAW_TASK_TYPE_ARC;
        case TRACE_TASK_CLASS_TRIANGLE: return LV_DRAW_TASK_TYPE_TRIANGLE;
        case TRACE_TASK_CLASS_MASK_RECTANGLE: return LV_DRAW_TASK_TYPE_MASK_RECTANGLE;
        case TRACE_TASK_CLASS_MASK_BITMAP: return LV_DRAW_TASK_TYPE_MASK_BITMAP;
#if LV_USE_VECTOR_GRAPHIC
        case TRACE_TASK_CLASS_VECTOR: return LV_DRAW_TASK_TYPE_VECTOR;
#endif
#if LV_USE_3DTEXTURE
        case TRACE_TASK_CLASS_3D: return LV_DRAW_TASK_TYPE_3D;
#endif
        case TRACE_TASK_CLASS_COUNT: break;
    }
    return LV_DRAW_TASK_TYPE_NONE;
}

static const char * task_subtype_name(trace_task_class_t task_class_value)
{
    switch(task_class_value) {
        case TRACE_TASK_CLASS_FILL_OPAQUE_SIMPLE: return "FILL_OPAQUE_SIMPLE";
        case TRACE_TASK_CLASS_FILL_ALPHA_SIMPLE: return "FILL_ALPHA_SIMPLE";
        case TRACE_TASK_CLASS_FILL_OTHER: return "FILL_OTHER";
        case TRACE_TASK_CLASS_IMAGE_COPY_OPAQUE: return "IMAGE_COPY_OPAQUE";
        case TRACE_TASK_CLASS_IMAGE_ALPHA: return "IMAGE_ALPHA";
        case TRACE_TASK_CLASS_IMAGE_TRANSFORMED: return "IMAGE_TRANSFORMED";
        case TRACE_TASK_CLASS_IMAGE_OTHER: return "IMAGE_OTHER";
        default: return "NONE";
    }
}

void draw_sched_trace_reset(void)
{
    memset(task_counters, 0, sizeof(task_counters));
    memset(pixel_counters, 0, sizeof(pixel_counters));
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
    task->trace_candidate_mask = 0U;
}

void draw_sched_trace_task_dispatched(const lv_draw_task_t * task, draw_sched_trace_dispatch_unit_t dispatch_unit)
{
    uint32_t pixels = 0U;
    lv_area_t effective_area;

    if(task->trace_scene_id >= TRACE_SCENE_COUNT || task->trace_candidate_mask >= TRACE_CANDIDATE_MASK_COUNT ||
       (uint32_t)dispatch_unit >= TRACE_DISPATCH_UNIT_COUNT) {
        return;
    }

    if(lv_area_intersect(&effective_area, &task->area, &task->clip_area)) {
        pixels = lv_area_get_size(&effective_area);
    }

    trace_task_class_t class_value = task_class(task);
    uint32_t bucket = area_bucket(pixels);
    task_counters[task->trace_scene_id][class_value][task->trace_candidate_mask][dispatch_unit][bucket]++;
    pixel_counters[task->trace_scene_id][class_value][task->trace_candidate_mask][dispatch_unit][bucket] += pixels;
}

void draw_sched_trace_dump_csv(void)
{
    LV_LOG("DRAW_SCHED_TRACE_BEGIN\r\n");
    LV_LOG("scene_id,scene_name,task_type,task_subtype,candidate_mask,candidate_units,dispatch_unit,area_bucket,task_count,pixel_count\r\n");

    for(uint32_t scene = 0U; scene < TRACE_SCENE_COUNT; scene++) {
        if(scene_names[scene] == NULL) continue;

        for(uint32_t class_value = 0U; class_value < TRACE_TASK_CLASS_COUNT; class_value++) {
            for(uint32_t candidate_mask = 0U; candidate_mask < TRACE_CANDIDATE_MASK_COUNT; candidate_mask++) {
                for(uint32_t dispatch_unit = 0U; dispatch_unit < TRACE_DISPATCH_UNIT_COUNT; dispatch_unit++) {
                    for(uint32_t bucket = 0U; bucket < TRACE_AREA_BUCKET_COUNT; bucket++) {
                        uint32_t task_count = task_counters[scene][class_value][candidate_mask][dispatch_unit][bucket];
                        if(task_count == 0U) continue;

                        char pixel_count[21];
                        lv_snprintf(pixel_count, sizeof(pixel_count), "%" LV_PRIu64,
                                    pixel_counters[scene][class_value][candidate_mask][dispatch_unit][bucket]);
                        LV_LOG("%lu,%s,%s,%s,%lu,%s,%s,%s,%lu,%s\r\n",
                               (unsigned long)scene,
                               scene_names[scene],
                               task_type_name(task_class_type((trace_task_class_t)class_value)),
                               task_subtype_name((trace_task_class_t)class_value),
                               (unsigned long)candidate_mask,
                               candidate_units_name(candidate_mask),
                               dispatch_unit_name(dispatch_unit),
                               area_bucket_name(bucket),
                               (unsigned long)task_count,
                               pixel_count);
                    }
                }
            }
        }
    }

    LV_LOG("DRAW_SCHED_TRACE_END\r\n");
}

#endif
