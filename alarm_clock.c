#include "alarm_clock.h"
#include "fsl_lpuart.h"
#include "fsl_debug_console.h"
#include "fsl_pwm.h"
#include "fsl_reset.h"
#include "gps_parser.h"
#include "led.h"
#include "pin_mux.h"
#include "pwm.h"
#include <stdbool.h>
#include "uart.h"
/* P0_2  LPUART0_RX
 * P0_3  LPUART0_TX
 * P1_14 LED_DIN
 * P3_1  LED_CS
 * P3_20 LPUART1_RX
 * P3_21 LPUART1_TX
 * P3_31 LED_CLK*/
#define BOARD_LED_GPIO     BOARD_LED_RED_GPIO
#define BOARD_LED_GPIO_PIN BOARD_LED_RED_GPIO_PIN
#define BEEP_ON()            PWM_StartTimer(FLEXPWM0, kPWM_Control_Module_0);
#define BEEP_OFF()           PWM_StopTimer(FLEXPWM0, kPWM_Control_Module_0);

extern int pwm_main(void);

// 全局状态变量
volatile display_mode_t current_disp_mode = DISP_TIME;
volatile _Bool is_alarm_enabled = true;
volatile _Bool is_beeping = false;

static void update_display()
{
    switch (current_disp_mode)
    {
        case DISP_OFF:
            disable_led();
            break;
        case DISP_TIME:
            enable_led();
            display_time();
            break;
        default:
            break;
    }
}

static void alarm_clock_check()
{
    static _Bool is_alarm_triggered = false;
    const uint8_t alarm_hour = 6;
    const uint8_t alarm_min = 30;

    // 当达到闹钟时间时
    if (current_local_time.hour == alarm_hour && current_local_time.minute == alarm_min)
    {       
        if (is_alarm_triggered)
        {
            return;
        }
        if (is_alarm_enabled)
        {
            is_beeping = true;
        }
        is_alarm_triggered = true;
    }
    else
    {
        if (is_alarm_triggered)
        {
            /*刚刚触发了闹钟*/
            is_alarm_triggered = false;
            
            // 1. 周五闹钟响完后自动 disable 的逻辑
            if (current_local_time.weekday == 5)    /*周五*/
            { // 5 = Friday
                is_alarm_enabled = false;
            }
            /*周六、周日或周一至周五人为关掉了闹钟*/
            else if (current_local_time.weekday == 0)    /*周日*/
            {
                is_alarm_enabled = true;
            }

            if (is_beeping)
            {
                is_beeping = false;
            }
        }
    }
}

static void self_test()
{
    LED_RED_ON();
    while (!is_rmc_ready)
    {
        ;
    }
    BEEP_ON();
    SDK_DelayAtLeastUs(9e5, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
}

int main(void)
{
    BOARD_InitPins();
    BOARD_InitBootClocks();
    init_lpuart0();
    init_lpuart1();
    
    /* Define the init structure for the output LED pin*/
    gpio_pin_config_t led_config = {
        kGPIO_DigitalOutput,
        0,
    };
    /* Define the init structure for the input switch pin */
    gpio_pin_config_t sw_config = {
        kGPIO_DigitalInput,
        0,
    };

    /* Board pin, clock, debug console init */
    /* Release peripheral reset */
    RESET_ReleasePeripheralReset(kLPUART0_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kPORT0_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kPORT1_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kGPIO1_RST_SHIFT_RSTn);
    CLOCK_EnableClock(kCLOCK_GateGPIO1);
    
    /* Init output LED GPIO. */
    GPIO_PinInit(BOARD_LED_GPIO, BOARD_LED_GPIO_PIN, &led_config);

    GPIO_SetPinInterruptConfig(BOARD_SW2_GPIO, BOARD_SW2_GPIO_PIN, kGPIO_InterruptFallingEdge);
    EnableIRQ(BOARD_SW2_IRQ);
    GPIO_PinInit(BOARD_SW2_GPIO, BOARD_SW2_GPIO_PIN, &sw_config);

    GPIO_SetPinInterruptConfig(BOARD_SW3_GPIO, BOARD_SW3_GPIO_PIN, kGPIO_InterruptFallingEdge);
    EnableIRQ(BOARD_SW3_IRQ);
    GPIO_PinInit(BOARD_SW3_GPIO, BOARD_SW3_GPIO_PIN, &sw_config);

    pwm_main();
    init_led();
    self_test();
    
    uint32_t cycle = 0;
    for (;;)
    {
        /*收到GPS数据，解析并判断闹钟*/
        if (is_rmc_ready)
        {
            is_rmc_ready = false;
            parse_gprmc(rmc);
            alarm_clock_check();
            update_display();
        }
        /*LED状态刷新*/
        if (is_alarm_enabled)
        {
            LED_RED_OFF();
        }
        else
        {
            LED_RED_ON();
        }

        if (is_beeping)
        {
            uint32_t flag = cycle++ & 0x1FFFFF;
            if (flag == 0)
            {
                BEEP_ON();
            }
            else if (flag == 0x100000)
            {
                BEEP_OFF();
            }
        }
        else
        {
            BEEP_OFF();
        }
    }
}
