#ifndef BENCH_TRANSFORM_H
#define BENCH_TRANSFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Bench_Transform_Setup(void);
void Bench_Transform_Step(uint32_t ticks);

#ifdef __cplusplus
}
#endif

#endif /* BENCH_TRANSFORM_H */