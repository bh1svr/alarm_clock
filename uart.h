#ifndef UART_H
#define UART_H
#include "fsl_clock.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "fsl_lpuart.h"
#include "board.h"
void init_lpuart0(void);
void init_lpuart1(void);
#endif
