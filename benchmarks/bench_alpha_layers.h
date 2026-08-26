#ifndef BENCH_ALPHA_LAYERS_H
#define BENCH_ALPHA_LAYERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Bench_Alpha_Setup(void);
void Bench_Alpha_Step(uint32_t ticks);

#ifdef __cplusplus
}
#endif

#endif /* BENCH_ALPHA_LAYERS_H */