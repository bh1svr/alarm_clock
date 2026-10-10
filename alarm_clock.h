#ifndef ALARM_CLOCK_H
#define ALARM_CLOCK_H
#include "board.h"
// 显示模式枚举
typedef enum {
    DISP_OFF,
    DISP_TIME,
    DISP_RANDOM,
    DISP_DATE,
    /*DISP_ALARM,
    DISP_TEMP*/
    DISP_MAX
} display_mode_t;

// 全局状态变量
extern volatile display_mode_t current_disp_mode;
extern volatile _Bool is_alarm_enabled;
extern volatile _Bool is_beeping;
extern volatile uint32_t gps_rand;
#endif
