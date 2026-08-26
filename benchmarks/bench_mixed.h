#ifndef BENCH_MIXED_H
#define BENCH_MIXED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Bench_Mixed_Setup(void);
void Bench_Mixed_Step(uint32_t ticks);

#ifdef __cplusplus
}
#endif

#endif /* BENCH_MIXED_H */