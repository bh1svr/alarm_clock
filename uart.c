#include "uart.h"

static void init_pins_lpuart0()
{
    /* PORT0: Peripheral clock is enabled */
    CLOCK_EnableClock(kCLOCK_GatePORT0);
    /* LPUART0 peripheral is released from reset */
    RESET_ReleasePeripheralReset(kLPUART0_RST_SHIFT_RSTn);
    /* PORT0 peripheral is released from reset */
    RESET_ReleasePeripheralReset(kPORT0_RST_SHIFT_RSTn);

    const port_pin_config_t port0_2_pin78_config = {/* Internal pull-up resistor is enabled */
                                                    kPORT_PullUp,
                                                    /* Low internal pull resistor value is selected. */
                                                    kPORT_LowPullResistor,
                                                    /* Fast slew rate is configured */
                                                    kPORT_FastSlewRate,
                                                    /* Passive input filter is disabled */
                                                    kPORT_PassiveFilterDisable,
                                                    /* Open drain output is disabled */
                                                    kPORT_OpenDrainDisable,
                                                    /* Low drive strength is configured */
                                                    kPORT_LowDriveStrength,
                                                    /* Normal drive strength is configured */
                                                    kPORT_NormalDriveStrength,
                                                    /* Pin is configured as LPUART0_RXD */
                                                    kPORT_MuxAlt2,
                                                    /* Digital input enabled */
                                                    kPORT_InputBufferEnable,
                                                    /* Digital input is not inverted */
                                                    kPORT_InputNormal,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
    /* PORT0_2 (pin 78) is configured as LPUART0_RXD */
    PORT_SetPinConfig(PORT0, 2U, &port0_2_pin78_config);

    const port_pin_config_t port0_3_pin79_config = {/* Internal pull-up resistor is enabled */
                                                    kPORT_PullUp,
                                                    /* Low internal pull resistor value is selected. */
                                                    kPORT_LowPullResistor,
                                                    /* Fast slew rate is configured */
                                                    kPORT_FastSlewRate,
                                                    /* Passive input filter is disabled */
                                                    kPORT_PassiveFilterDisable,
                                                    /* Open drain output is disabled */
                                                    kPORT_OpenDrainDisable,
                                                    /* Low drive strength is configured */
                                                    kPORT_LowDriveStrength,
                                                    /* Normal drive strength is configured */
                                                    kPORT_NormalDriveStrength,
                                                    /* Pin is configured as LPUART0_TXD */
                                                    kPORT_MuxAlt2,
                                                    /* Digital input enabled */
                                                    kPORT_InputBufferEnable,
                                                    /* Digital input is not inverted */
                                                    kPORT_InputNormal,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
    /* PORT0_3 (pin 79) is configured as LPUART0_TXD */
    PORT_SetPinConfig(PORT0, 3U, &port0_3_pin79_config);
}

void init_lpuart0()
{
    init_pins_lpuart0();
    /* attach 12 MHz clock to FLEXCOMM0 (debug console) */
    CLOCK_SetClockDiv(kCLOCK_DivLPUART0, 1u);
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    RESET_PeripheralReset(BOARD_DEBUG_UART_RST);

    DbgConsole_Init(BOARD_DEBUG_UART_INSTANCE, BOARD_DEBUG_UART_BAUDRATE, BOARD_DEBUG_UART_TYPE,
                    BOARD_DEBUG_UART_CLK_FREQ);

}

void init_lpuart1()
{
    CLOCK_SetClockDiv(kCLOCK_DivLPUART1, 1u);
    CLOCK_AttachClk(kFRO12M_to_LPUART1);

    lpuart_config_t config;
    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;

    LPUART_Init(LPUART1, &config, BOARD_DEBUG_UART_CLK_FREQ);

    /* Enable RX interrupt. */
    LPUART_EnableInterrupts(LPUART1, kLPUART_RxDataRegFullInterruptEnable);
    EnableIRQ(LPUART1_IRQn);

}

