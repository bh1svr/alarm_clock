#include "pin_mux.h"
#include "board.h"
#include "fsl_lpuart.h"
#include "fsl_debug_console.h"
#include "fsl_reset.h"
#include "gps_parser.h"
#include <stdbool.h>
#include "uart.h"

#define DEMO_LPUART            LPUART1
#define DEMO_LPUART_CLK_FREQ   (BOARD_DEBUG_UART_CLK_FREQ)
#define DEMO_LPUART_IRQn       LPUART1_IRQn
#define DEMO_LPUART_IRQHandler LPUART1_IRQHandler
#define BOARD_LED_GPIO     BOARD_LED_RED_GPIO
#define BOARD_LED_GPIO_PIN BOARD_LED_RED_GPIO_PIN
#define BOARD_SW_GPIO        BOARD_SW2_GPIO
#define BOARD_SW_GPIO_PIN    BOARD_SW2_GPIO_PIN
#define BOARD_SW_NAME        BOARD_SW2_NAME
#define BOARD_SW_IRQ         BOARD_SW2_IRQ
#define BOARD_SW_IRQ_HANDLER BOARD_SW2_IRQ_HANDLER

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
volatile _Bool is_alarm_enabled = true;
volatile _Bool is_rmc_ready;
static local_time_t current_local_time = {0};

#define NMEA_MAX_LENGTH 256
static char nmea_buffer[NMEA_MAX_LENGTH];
static char rmc[NMEA_MAX_LENGTH];
static uint8_t nmea_index = 0;

// 每个月的天数（平年）
static const uint8_t days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void GPS_ParseChar(char c)
{
    if (c == '$')
    {
        nmea_index = 0;
        nmea_buffer[nmea_index++] = c;
    }
    else if (c == '\r' || c == '\n')
    {
        if (nmea_index > 0)
        {
            nmea_buffer[nmea_index] = '\0';
            // 判断是否为 RMC 报文
            if (strncmp(nmea_buffer, "$GNRMC", 6) == 0)
            {
                memcpy(rmc, nmea_buffer, nmea_index + 1);
                is_rmc_ready = true;
            }
            nmea_index = 0;
        }
    }
    else {
        if (nmea_index < NMEA_MAX_LENGTH - 1) {
            nmea_buffer[nmea_index++] = c;
        }
    }
}
// 判断闰年
static bool is_leap_year(uint16_t year)
{
    return ((year * 1073750999) & 3221352463) <= 126976;
}

// 基于蔡勒公式(Zeller's congruence)或直接计算星期，返回 0-6 (Sun-Sat)
static uint8_t calculate_weekday(uint16_t y, uint8_t m, uint8_t d) {
    if (m == 1 || m == 2) {
        m += 12;
        y--;
    }
    int w = (d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400 + 1) % 7;
    return (uint8_t)w;
}

// 将 UTC 字符串 (hhmmss, ddmmyy) 转换为本地时间 (UTC+8)
static void convert_to_local_time(const char* time_str, const char* date_str, local_time_t* t) {
    if (strlen(time_str) < 6)
    {
        return;
    }

    // 解析 UTC
    uint8_t u_hh = (time_str[0] - '0') * 10 + (time_str[1] - '0');
    uint8_t u_mm = (time_str[2] - '0') * 10 + (time_str[3] - '0');
    uint8_t u_ss = (time_str[4] - '0') * 10 + (time_str[5] - '0');
    uint8_t u_d = (date_str[0] - '0') * 10 + (date_str[1] - '0');
    uint8_t u_M = (date_str[2] - '0') * 10 + (date_str[3] - '0');
    uint16_t u_y = (date_str[4] - '0') * 10 + (date_str[5] - '0') + 2000;

    // 加上 8 小时
    u_hh += 8;

    // 处理进位
    if (u_hh >= 24) {
        u_hh -= 24;
        u_d++;
        
        uint8_t month_days = days_in_month[u_M];
        if (u_M == 2 && is_leap_year(u_y)) {
            month_days = 29;
        }

        if (u_d > month_days) {
            u_d = 1;
            u_M++;
            if (u_M > 12) {
                u_M = 1;
                u_y++;
            }
        }
    }

    t->year = u_y;
    t->month = u_M;
    t->day = u_d;
    t->hour = u_hh;
    t->minute = u_mm;
    t->second = u_ss;
    t->weekday = calculate_weekday(u_y, u_M, u_d);
}

// 解析 $GPRMC 报文
// 示例格式: $GPRMC,072242.000,A,3723.2475,N,12158.3416,W,0.13,309.62,260826,,,A*10
static void parse_gprmc(char* nmea) {
    char *token;
    char time_str[16] = {0};
    char date_str[16] = {0};
    uint8_t field_idx = 0;

    token = strtok(nmea, ",");
    while (token != NULL) {
        if (field_idx == 1) { // 时间
            strncpy(time_str, token, sizeof(time_str)-1);
        } 
        else if (field_idx == 9) { // 日期
            strncpy(date_str, token, sizeof(date_str)-1);
        }
        token = strtok(NULL, ",");
        field_idx++;
    }

    convert_to_local_time(time_str, date_str, &current_local_time);
}

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

static void alarm_clock_check()
{
    static _Bool is_alarm_triggered = false;
    const uint8_t alarm_hour = 6;
    const uint8_t alarm_min = 30;

    // 当达到闹钟时间时
    if (current_local_time.hour == alarm_hour && current_local_time.minute == alarm_min)
    {
        if (is_alarm_enabled)
        {
            if (is_alarm_triggered)
            {
                return;
            } 
            //TODO: 启动PWM
            is_alarm_triggered = true;
        }
        else if (current_local_time.weekday == 0)
        {
            is_alarm_enabled = true;
            //TODO: 更新指示灯
        }
    }
    else
    {
        if (is_alarm_triggered)
        {
            /*刚刚触发了闹钟*/
            is_alarm_triggered = false;
            // 1. 周五闹钟响完后自动 disable 的逻辑
            if (current_local_time.weekday == 5)
            { // 5 = Friday
                is_alarm_enabled = false;
                //TODO: 更新指示灯
            }
        }
    }
}

void BOARD_SW_IRQ_HANDLER(void)
{
    GPIO_GpioClearInterruptFlags(BOARD_SW_GPIO, 1U << BOARD_SW_GPIO_PIN);
    is_alarm_enabled = !is_alarm_enabled;
    SDK_ISR_EXIT_BARRIER;
}

int main(void)
{
    init_lpuart1();
    
    BOARD_InitPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    PRINTF("Alarm Clock 1\r\n");

    lpuart_config_t config;
    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;

    LPUART_Init(DEMO_LPUART, &config, DEMO_LPUART_CLK_FREQ);

    /* Enable RX interrupt. */
    LPUART_EnableInterrupts(DEMO_LPUART, kLPUART_RxDataRegFullInterruptEnable);
    EnableIRQ(DEMO_LPUART_IRQn);

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

    GPIO_SetPinInterruptConfig(BOARD_SW_GPIO, BOARD_SW_GPIO_PIN, kGPIO_InterruptFallingEdge);
    EnableIRQ(BOARD_SW_IRQ);
    GPIO_PinInit(BOARD_SW_GPIO, BOARD_SW_GPIO_PIN, &sw_config);

    pwm_main();
    
    for (;;)
    {
        if (is_rmc_ready)
        {
            is_rmc_ready = false;
            parse_gprmc(rmc);
            alarm_clock_check();
        }
        if (is_alarm_enabled)
        {
            GPIO_PinWrite(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PIN, LOGIC_LED_ON);  /*!< Turn on target LED_RED */
        }
        else
        {
            GPIO_PinWrite(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PIN, LOGIC_LED_OFF);  /*!< Turn on target LED_RED */
        }
    }
}
