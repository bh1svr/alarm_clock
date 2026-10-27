#include "fsl_common.h"
#include "fsl_port.h"
#include "pin_mux.h"

void BOARD_InitPWMPins(void)
{
    /* FLEXPWM0 peripheral is released from reset */
    RESET_ReleasePeripheralReset(kFLEXPWM0_RST_SHIFT_RSTn);
    /* PORT0: Peripheral clock is enabled */
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
}
/***********************************************************************************************************************
 * EOF
 **********************************************************************************************************************/
