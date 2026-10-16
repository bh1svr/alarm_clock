#include "alarm_clock.h"
#include "fsl_debug_console.h"
#include "gps_parser.h"
#include <string.h>
#include <stdlib.h>

static char nmea_buffer[NMEA_MAX_LENGTH];
char rmc[NMEA_MAX_LENGTH];
static uint8_t nmea_index = 0;
// 每个月的天数（平年）
static const uint8_t days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
volatile _Bool is_rmc_ready;
local_time_t current_local_time = {0};

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

    gps_rand += c;
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
static void convert_to_local_time(const char* time_str, const char* date_str, local_time_t* t)
{
    if (strlen(time_str) < 6)
    {
        return;
    }

    // 解析 UTC
    uint8_t u_hh = (time_str[0] - '0') * 10 + (time_str[1] - '0');
    uint8_t u_mm = (time_str[2] - '0') * 10 + (time_str[3] - '0');
    uint8_t u_ss = (time_str[4] - '0') * 10 + (time_str[5] - '0');

    uint16_t u_y;
    uint8_t  u_M, u_d;
    if (*date_str)
    {
        u_d = (date_str[0] - '0') * 10 + (date_str[1] - '0');
        u_M = (date_str[2] - '0') * 10 + (date_str[3] - '0');
        u_y = (date_str[4] - '0') * 10 + (date_str[5] - '0') + 2000;
    }
    else
    {
        u_y = 0;
        u_M = 0;
        u_d = 0;
    }
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
void parse_gprmc(char* nmea)
{
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


