#include "fsl_common.h"
#include "fsl_port.h"
#include "pin_mux.h"

/* FUNCTION ************************************************************************************************************
 *
 * Function Name : BOARD_InitBootPins
 * Description   : Calls initialization functions.
 *
 * END ****************************************************************************************************************/
void BOARD_InitBootPins(void)
{
    BOARD_InitPins();
}

/* clang-format off */
/*
 * TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
BOARD_InitPins:
- options: {callFromInitBoot: 'true', prefix: BOARD_, coreID: cm33_core0, enableClock: 'true'}
- pin_list:
  - {pin_num: '78', peripheral: LPUART0, signal: RX, pin_signal: P0_2/TDO/SWO/LPUART0_RXD/LPSPI0_SCK/CT0_MAT0/UTICK_CAP0/FLEXIO0_D2, slew_rate: fast, open_drain: disable,
    drive_strength: low, pull_select: up, pull_enable: enable, input_buffer: enable, invert_input: normal}
  - {pin_num: '79', peripheral: LPUART0, signal: TX, pin_signal: P0_3/TDI/LPUART0_TXD/LPSPI0_SDO/CT0_MAT1/UTICK_CAP1/FLEXIO0_D3/CMP0_OUT, slew_rate: fast, open_drain: disable,
    drive_strength: low, pull_select: up, pull_enable: enable, input_buffer: enable, invert_input: normal}
  - {pin_num: '71', peripheral: FlexPWM0, signal: 'A, 0', pin_signal: P3_6/LPSPI1_PCS3/LPUART3_RTS_B/CT4_MAT2/PWM0_A0/FLEXIO0_D14/PWM1_A0, slew_rate: fast, open_drain: disable,
    drive_strength: low, pull_select: down, pull_enable: disable, input_buffer: enable, invert_input: normal}
  - {pin_num: '70', peripheral: FlexPWM0, signal: 'B, 0', pin_signal: P3_7/TRIG_IN2/LPSPI1_PCS2/LPUART3_CTS_B/CT4_MAT3/PWM0_B0/FLEXIO0_D15/PWM1_B0, slew_rate: fast,
    open_drain: disable, drive_strength: low, pull_select: down, pull_enable: disable, input_buffer: enable, invert_input: normal}
  - {pin_num: '69', peripheral: FlexPWM0, signal: 'A, 1', pin_signal: P3_8/TRIG_IN3/LPSPI1_SDO/LPUART1_RXD/CT_INP4/PWM0_A1/FLEXIO0_D16, slew_rate: fast, open_drain: disable,
    drive_strength: low, pull_select: down, pull_enable: disable, input_buffer: enable, invert_input: normal}
  - {pin_num: '67', peripheral: FlexPWM0, signal: 'A, 2', pin_signal: P3_10/TRIG_IN5/LPSPI1_SCK/LPUART1_RTS_B/CT1_MAT0/PWM0_A2/FLEXIO0_D18, slew_rate: fast, open_drain: disable,
    drive_strength: low, pull_select: down, pull_enable: disable, input_buffer: enable, invert_input: normal}
  - {pin_num: '29', peripheral: INPUTMUX0, signal: 'TRIG_IN, 5', pin_signal: P2_7/TRIG_IN5/LPUART4_TXD/CT_INP19/CT1_MAT3/FLEXIO0_D15, slew_rate: fast, open_drain: disable,
    drive_strength: low, pull_select: down, pull_enable: disable, input_buffer: enable, invert_input: normal}
  - {pin_num: '40', peripheral: INPUTMUX0, signal: 'TRIG_IN, 8', pin_signal: P2_20/TRIG_IN8/LPSPI1_PCS2/CT2_MAT0/FLEXIO0_D28, slew_rate: fast, open_drain: disable,
    drive_strength: low, pull_select: down, pull_enable: disable, input_buffer: enable, invert_input: normal}
  - {pin_num: '38', peripheral: INPUTMUX0, signal: 'TRIG_IN, 9', pin_signal: P2_17/TRIG_IN9/LPSPI1_PCS0/LPUART1_CTS_B/CT3_MAT1/CT0_MAT3/FLEXIO0_D25, slew_rate: fast,
    open_drain: disable, drive_strength: low, pull_select: down, pull_enable: disable, input_buffer: enable, invert_input: normal}
  - {pin_num: '48', peripheral: INPUTMUX0, signal: 'TRIG_IN, 10', pin_signal: P3_31/TRIG_IN10/LPI2C3_SDAS/LPUART4_CTS_B/CT0_MAT3/FLEXIO0_D31, slew_rate: fast, open_drain: disable,
    drive_strength: low, pull_select: down, pull_enable: disable, input_buffer: enable, invert_input: normal}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS ***********
 */
/* clang-format on */

/* FUNCTION ************************************************************************************************************
 *
 * Function Name : BOARD_InitPins
 * Description   : Configures pin routing and optionally pin electrical features.
 *
 * END ****************************************************************************************************************/
void BOARD_InitPWMPins(void)
{
    /* PORT0: Peripheral clock is enabled */
    CLOCK_EnableClock(kCLOCK_GatePORT0);
    /* PORT2: Peripheral clock is enabled */
    CLOCK_EnableClock(kCLOCK_GatePORT2);
    /* PORT3: Peripheral clock is enabled */
    CLOCK_EnableClock(kCLOCK_GatePORT3);
    /* LPUART0 peripheral is released from reset */
    RESET_ReleasePeripheralReset(kLPUART0_RST_SHIFT_RSTn);
    /* PORT0 peripheral is released from reset */
    RESET_ReleasePeripheralReset(kPORT0_RST_SHIFT_RSTn);
    /* INPUTMUX0 peripheral is released from reset */
    RESET_ReleasePeripheralReset(kINPUTMUX0_RST_SHIFT_RSTn);
    /* PORT2 peripheral is released from reset */
    RESET_ReleasePeripheralReset(kPORT2_RST_SHIFT_RSTn);
    /* FLEXPWM0 peripheral is released from reset */
    RESET_ReleasePeripheralReset(kFLEXPWM0_RST_SHIFT_RSTn);
    /* PORT3 peripheral is released from reset */
    RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);

    const port_pin_config_t port2_17_pin38_config = {/* Internal pull-up/down resistor is disabled */
                                                     kPORT_PullDisable,
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
                                                     /* Pin is configured as TRIG_IN9 */
                                                     kPORT_MuxAlt1,
                                                     /* Digital input enabled */
                                                     kPORT_InputBufferEnable,
                                                     /* Digital input is not inverted */
                                                     kPORT_InputNormal,
                                                     /* Pin Control Register fields [15:0] are not locked */
                                                     kPORT_UnlockRegister};
    /* PORT2_17 (pin 38) is configured as TRIG_IN9 */
    PORT_SetPinConfig(PORT2, 17U, &port2_17_pin38_config);

    const port_pin_config_t port2_20_pin40_config = {/* Internal pull-up/down resistor is disabled */
                                                     kPORT_PullDisable,
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
                                                     /* Pin is configured as TRIG_IN8 */
                                                     kPORT_MuxAlt1,
                                                     /* Digital input enabled */
                                                     kPORT_InputBufferEnable,
                                                     /* Digital input is not inverted */
                                                     kPORT_InputNormal,
                                                     /* Pin Control Register fields [15:0] are not locked */
                                                     kPORT_UnlockRegister};
    /* PORT2_20 (pin 40) is configured as TRIG_IN8 */
    PORT_SetPinConfig(PORT2, 20U, &port2_20_pin40_config);

    const port_pin_config_t port2_7_pin29_config = {/* Internal pull-up/down resistor is disabled */
                                                    kPORT_PullDisable,
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
                                                    /* Pin is configured as TRIG_IN5 */
                                                    kPORT_MuxAlt1,
                                                    /* Digital input enabled */
                                                    kPORT_InputBufferEnable,
                                                    /* Digital input is not inverted */
                                                    kPORT_InputNormal,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
    /* PORT2_7 (pin 29) is configured as TRIG_IN5 */
    PORT_SetPinConfig(PORT2, 7U, &port2_7_pin29_config);

    const port_pin_config_t port3_10_pin67_config = {/* Internal pull-up/down resistor is disabled */
                                                     kPORT_PullDisable,
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
                                                     /* Pin is configured as PWM0_A2 */
                                                     kPORT_MuxAlt5,
                                                     /* Digital input enabled */
                                                     kPORT_InputBufferEnable,
                                                     /* Digital input is not inverted */
                                                     kPORT_InputNormal,
                                                     /* Pin Control Register fields [15:0] are not locked */
                                                     kPORT_UnlockRegister};
    /* PORT3_10 (pin 67) is configured as PWM0_A2 */
    PORT_SetPinConfig(PORT3, 10U, &port3_10_pin67_config);

    const port_pin_config_t port3_31_pin48_config = {/* Internal pull-up/down resistor is disabled */
                                                     kPORT_PullDisable,
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
                                                     /* Pin is configured as TRIG_IN10 */
                                                     kPORT_MuxAlt1,
                                                     /* Digital input enabled */
                                                     kPORT_InputBufferEnable,
                                                     /* Digital input is not inverted */
                                                     kPORT_InputNormal,
                                                     /* Pin Control Register fields [15:0] are not locked */
                                                     kPORT_UnlockRegister};
    /* PORT3_31 (pin 48) is configured as TRIG_IN10 */
    PORT_SetPinConfig(PORT3, 31U, &port3_31_pin48_config);

    const port_pin_config_t port3_6_pin71_config = {/* Internal pull-up/down resistor is disabled */
                                                    kPORT_PullDisable,
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
                                                    /* Pin is configured as PWM0_A0 */
                                                    kPORT_MuxAlt5,
                                                    /* Digital input enabled */
                                                    kPORT_InputBufferEnable,
                                                    /* Digital input is not inverted */
                                                    kPORT_InputNormal,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
    /* PORT3_6 (pin 71) is configured as PWM0_A0 */
    PORT_SetPinConfig(PORT3, 6U, &port3_6_pin71_config);

    const port_pin_config_t port3_7_pin70_config = {/* Internal pull-up/down resistor is disabled */
                                                    kPORT_PullDisable,
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
                                                    /* Pin is configured as PWM0_B0 */
                                                    kPORT_MuxAlt5,
                                                    /* Digital input enabled */
                                                    kPORT_InputBufferEnable,
                                                    /* Digital input is not inverted */
                                                    kPORT_InputNormal,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
    /* PORT3_7 (pin 70) is configured as PWM0_B0 */
    PORT_SetPinConfig(PORT3, 7U, &port3_7_pin70_config);

    const port_pin_config_t port3_8_pin69_config = {/* Internal pull-up/down resistor is disabled */
                                                    kPORT_PullDisable,
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
                                                    /* Pin is configured as PWM0_A1 */
                                                    kPORT_MuxAlt5,
                                                    /* Digital input enabled */
                                                    kPORT_InputBufferEnable,
                                                    /* Digital input is not inverted */
                                                    kPORT_InputNormal,
                                                    /* Pin Control Register fields [15:0] are not locked */
                                                    kPORT_UnlockRegister};
    /* PORT3_8 (pin 69) is configured as PWM0_A1 */
    PORT_SetPinConfig(PORT3, 8U, &port3_8_pin69_config);
}
/***********************************************************************************************************************
 * EOF
 **********************************************************************************************************************/
