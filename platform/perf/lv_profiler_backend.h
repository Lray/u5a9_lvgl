#ifndef LV_PROFILER_BACKEND_H
#define LV_PROFILER_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

void lv_profiler_begin(const char * tag);
void lv_profiler_end(const char * tag);

#ifdef __cplusplus
}
#endif

#define LV_PROFILER_BACKEND_BEGIN_TAG(tag) lv_profiler_begin((tag))
#define LV_PROFILER_BACKEND_END_TAG(tag)   lv_profiler_end((tag))
#define LV_PROFILER_BACKEND_BEGIN          LV_PROFILER_BACKEND_BEGIN_TAG(__func__)
#define LV_PROFILER_BACKEND_END            LV_PROFILER_BACKEND_END_TAG(__func__)

#endif /* LV_PROFILER_BACKEND_H */
