#ifndef BENCH_TEXT_SCROLL_H
#define BENCH_TEXT_SCROLL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Bench_Text_Setup(void);
void Bench_Text_Step(uint32_t ticks);

#ifdef __cplusplus
}
#endif

#endif /* BENCH_TEXT_SCROLL_H */