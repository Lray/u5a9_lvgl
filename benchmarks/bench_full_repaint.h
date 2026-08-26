#ifndef BENCH_FULL_REPAINT_H
#define BENCH_FULL_REPAINT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Bench_FullRepaint_Setup(void);
void Bench_FullRepaint_Step(uint32_t ticks);

#ifdef __cplusplus
}
#endif

#endif /* BENCH_FULL_REPAINT_H */