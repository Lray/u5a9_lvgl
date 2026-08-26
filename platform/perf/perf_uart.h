#ifndef PERF_UART_H
#define PERF_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct
{
  volatile uint32_t lines;
  volatile uint32_t drops;
} perf_uart_stats_t;

extern perf_uart_stats_t g_perf_uart_stats;

void Perf_Uart_Start(void);

#ifdef __cplusplus
}
#endif

#endif /* PERF_UART_H */
