#include "alarm_clock.h"
#include <stdbool.h>

void BOARD_SW2_IRQ_HANDLER(void)
{
    GPIO_GpioClearInterruptFlags(BOARD_SW2_GPIO, 1U << BOARD_SW2_GPIO_PIN);
    /*蜂鸣器正在响，此时按键关掉蜂鸣器*/
    if (is_beeping)
    {
        is_beeping = false;
    }
    /*平时做闹钟开关*/
    else
    {
        is_alarm_enabled = !is_alarm_enabled;
    }
    SDK_ISR_EXIT_BARRIER;
}

void BOARD_SW3_IRQ_HANDLER(void)
{
    GPIO_GpioClearInterruptFlags(BOARD_SW3_GPIO, 1U << BOARD_SW3_GPIO_PIN);
    current_disp_mode = (current_disp_mode + 1) % DISP_MAX;
    SDK_ISR_EXIT_BARRIER;
}


