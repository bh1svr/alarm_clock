#include "pin_mux.h"
#include "board.h"
#include "fsl_lpuart.h"
#include "fsl_debug_console.h"
#include "fsl_clock.h"
#include "fsl_reset.h"
#include "gps_parser.h"
#include <stdbool.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEMO_LPUART            LPUART1
#define DEMO_LPUART_CLK_FREQ   (BOARD_DEBUG_UART_CLK_FREQ)
#define DEMO_LPUART_IRQn       LPUART1_IRQn
#define DEMO_LPUART_IRQHandler LPUART1_IRQHandler

extern int pwm_main(void);
// 显示模式枚举
typedef enum {
    DISP_MONTH_DAY = 0,
    DISP_TIME,
    DISP_ALARM,
    DISP_TEMP
} display_mode_t;

// 全局状态变量
display_mode_t current_disp_mode = DISP_TIME;

// 假定设定的闹钟时间（实际应从 LittleFS 读取）

void DEMO_LPUART_IRQHandler(void)
{
    uint8_t data;

    /* If new data arrived. */
    if ((kLPUART_RxDataRegFullFlag)&LPUART_GetStatusFlags(DEMO_LPUART))
    {
        data = LPUART_ReadByte(DEMO_LPUART);
        GPS_ParseChar(data);
    }
    SDK_ISR_EXIT_BARRIER;
}

/*!
 * @brief Main function
 */
int main(void)
{
    lpuart_config_t config;

    /* Attach peripheral clock */
    CLOCK_SetClockDiv(kCLOCK_DivLPUART1, 1u);
    CLOCK_AttachClk(kFRO12M_to_LPUART1);

    BOARD_InitPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    PRINTF("Alarm Clock 1\r\n");
    /*
     * config.baudRate_Bps = 115200U;
     * config.parityMode = kLPUART_ParityDisabled;
     * config.stopBitCount = kLPUART_OneStopBit;
     * config.txFifoWatermark = 0;
     * config.rxFifoWatermark = 0;
     * config.enableTx = false;
     * config.enableRx = false;
     */
    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;

    LPUART_Init(DEMO_LPUART, &config, DEMO_LPUART_CLK_FREQ);

    /* Enable RX interrupt. */
    LPUART_EnableInterrupts(DEMO_LPUART, kLPUART_RxDataRegFullInterruptEnable);
    EnableIRQ(DEMO_LPUART_IRQn);

    pwm_main();
}
