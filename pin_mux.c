#include "fsl_common.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "pin_mux.h"

void BOARD_InitBootPins(void)
{
    BOARD_InitPins();
}

void BOARD_InitPins(void)
{
    gpio_pin_config_t gpio3_pin63_config = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 0U
    };
    /* Initialize GPIO functionality on pin PIO3_12 (pin 63)  */
    GPIO_PinInit(GPIO3, 12U, &gpio3_pin63_config);

    const port_pin_config_t port3_12_pin63_config = {/* Internal pull-up/down resistor is disabled */
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
                                                     /* Pin is configured as P3_12 */
                                                     kPORT_MuxAlt0,
                                                     /* Digital input enabled */
                                                     kPORT_InputBufferEnable,
                                                     /* Digital input is not inverted */
                                                     kPORT_InputNormal,
                                                     /* Pin Control Register fields [15:0] are not locked */
                                                     kPORT_UnlockRegister};
    /* PORT3_12 (pin 63) is configured as P3_12 */
    PORT_SetPinConfig(PORT3, 12U, &port3_12_pin63_config);

    gpio_pin_config_t gpio1_pin100_config = {
        .pinDirection = kGPIO_DigitalInput,
        .outputLogic = 0U
    };
    /* Initialize GPIO functionality on pin PIO1_7 (pin 100)  */
    GPIO_PinInit(GPIO1, 7U, &gpio1_pin100_config);

    const port_pin_config_t port1_7_pin100_config = {/* Internal pull-up/down resistor is disabled */
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
                                                     /* Pin is configured as P1_7 */
                                                     kPORT_MuxAlt0,
                                                     /* Digital input enabled */
                                                     kPORT_InputBufferEnable,
                                                     /* Digital input is not inverted */
                                                     kPORT_InputNormal,
                                                     /* Pin Control Register fields [15:0] are not locked */
                                                     kPORT_UnlockRegister};
    /* PORT1_7 (pin 100) is configured as P1_7 */
    PORT_SetPinConfig(PORT1, 7U, &port1_7_pin100_config);

}
