#include "alarm_clock.h"
#include "fsl_debug_console.h"
#include "gps_parser.h"
#include <string.h>
#include <stdlib.h>

void NMEA_Parse(uint8_t *line, uint32_t lineLen);

// 每个月的天数（平年）
static const uint8_t days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
local_time_t current_local_time = {0};

static bool is_leap_year(uint16_t year)
{
    return ((year % 4U) == 0U) &&
           (((year % 100U) != 0U) ||
            ((year % 400U) == 0U));
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

static bool valid_date(uint16_t y, uint8_t m, uint8_t d)
{
    if (m < 1U || m > 12U)
        return false;

    uint8_t max_day = days_in_month[m];

    if ((m == 2U) && is_leap_year(y))
        max_day = 29U;

    return (d >= 1U && d <= max_day);
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
    if (u_hh > 23)
    {
        u_hh = 0;
    }
    if (u_mm > 59)
    {
        u_mm = 0;
    }
    if (u_ss > 60)
    {
        u_ss = 0;
    }

    uint16_t u_y = 1;
    uint8_t  u_M = 1, u_d = 1;
    if (strlen(date_str) >= 6)
    {
        uint8_t day   = (date_str[0] - '0') * 10 + (date_str[1] - '0');
        uint8_t  month = (date_str[2] - '0') * 10 + (date_str[3] - '0');
        uint16_t  year  = (date_str[4] - '0') * 10 + (date_str[5] - '0') + 2000;
        if (valid_date(year, month, day))
        {
            u_y = year;
            u_M = month;
            u_d = day;
        }
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

#define RX_CHUNK   256U
#define LINE_MAX   96U      /* NMEA 最长 82 字符 + 余量 */

static uint8_t  line[LINE_MAX];
static uint32_t lineLen;
static bool     dropLine;          /* 超长丢弃，等 \n 复位 */

/* 每次收到一批数据（无论来自 IDLE 还是 TC 事件）时投喂 */
void RxFeed(const uint8_t *p, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        uint8_t c = p[i];
        gps_rand += c;
        if (c == '\n')                    /* 唯一可信的帧界 */
        {
            if (!dropLine && lineLen > 0)
            {
                NMEA_Parse(line, lineLen);
            }
            lineLen = 0;
            dropLine = false;
        }
        else if (c != '\r')
        {
            if (lineLen < LINE_MAX - 1) line[lineLen++] = c;
            else                        dropLine = true;
        }
    }
}

/*═══════════════════════════════════════════════════════════
 *  nmea_parse.c —— NMEA 句子校验 + 查表分发
 *  契约：line 指向的缓冲区必须至少有 lineLen+1 字节
 *        （RxFeed 的 LINE_MAX 数组天然满足）
 *═══════════════════════════════════════════════════════════*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

extern void parse_rmc(char *nmea);   /* 你已有的函数，换你的实际声明/头文件 */

/*---------------- 分发表 ----------------*/
typedef struct {
    const char *talker;             /* NULL = 任意 talker（GN/GP/GB/GA/GL…）*/
    const char *format;             /* 3 字符语句格式                        */
    void (*handler)(char *nmea);    /* 收到以 '\0' 结尾的完整句子（含 *CS）  */
} nmea_entry_t;

static const nmea_entry_t s_nmeaTable[] =
{
    /* talker    format    handler       */
    {   NULL,    "RMC",    parse_rmc },   /* GNRMC / GPRMC / GBRMC…        */
    /* 以后加报文在这里各加一行：        */
    /* {   NULL,    "GGA",    parse_gga  },  *//* 任意 talker 的 GGA           */
    /* {   "GN",    "GSV",    parse_gsv  },  *//* 也可限定只收 GNGSV           */
    /* {   NULL,    "ZDA",    parse_zda  },  *//* 备用时间源                    */
};

#define NMEA_ENTRY_NUM   (sizeof(s_nmeaTable) / sizeof(s_nmeaTable[0]))

/* 健康指标：与 g_ringOvf / g_nmeaDrop 同一套观测思路 */
volatile uint32_t g_nmeaCksumErr    = 0U;   /* 校验和失败的句子数 */
volatile uint32_t g_nmeaDispatched  = 0U;   /* 成功分发的句子数   */

/*---------------- 单个 hex 字符 → 数值 ----------------*/
static bool HexVal(char c, uint8_t *v)
{
    if ((c >= '0') && (c <= '9')) { *v = (uint8_t)(c - '0');        return true; }
    if ((c >= 'A') && (c <= 'F')) { *v = (uint8_t)(c - 'A' + 10U);  return true; }
    if ((c >= 'a') && (c <= 'f')) { *v = (uint8_t)(c - 'a' + 10U);  return true; }
    return false;
}

/*---------------- 校验和 ----------------
 * 结构要求：$....*XY（'XOR 区间' = '$' 之后、'*' 之前的所有字符）
 * 入口已保证 len >= 9 */
static bool NmeaChecksumOk(const uint8_t *line, uint32_t len)
{
    if (line[len - 3U] != (uint8_t)'*') { return false; }

    uint8_t x = 0U;
    for (uint32_t i = 1U; i < (len - 3U); i++) { x ^= line[i]; }

    uint8_t hi, lo;
    if (!HexVal((char)line[len - 2U], &hi)) { return false; }
    if (!HexVal((char)line[len - 1U], &lo)) { return false; }
    return (x == (uint8_t)((hi << 4) | lo));
}

/*---------------- 对外入口 ----------------*/
void NMEA_Parse(uint8_t *line, uint32_t lineLen)
{
    /* 结构下限：'$' + 2 talker + 3 format + '*' + 2 hex = 9 字符。
       || 短路保证 lineLen==0 时不会访问 line[0] */
    if ((line == NULL) || (lineLen < 9U) || (line[0] != (uint8_t)'$'))
    { return; }

    if (!NmeaChecksumOk(line, lineLen))
    {
        g_nmeaCksumErr++;
        return;
    }

    /* 变成 parse_gprmc 需要的 C 字符串（契约：buffer >= lineLen+1） */
    line[lineLen] = (uint8_t)'\0';

    /* 查表：talker = line[1..2]，format = line[3..5] */
    for (uint32_t e = 0U; e < NMEA_ENTRY_NUM; e++)
    {
        if ((s_nmeaTable[e].talker != NULL) &&
            (memcmp(&line[1], s_nmeaTable[e].talker, 2U) != 0))
        { continue; }

        if (memcmp(&line[3], s_nmeaTable[e].format, 3U) == 0)
        {
            g_nmeaDispatched++;
            s_nmeaTable[e].handler((char *)line);
            return;    /* 命中一条即返回：表里别放重复 format 的表项 */
        }
    }
}

// 解析 $GPRMC 报文
// 示例格式: $GPRMC,072242.000,A,3723.2475,N,12158.3416,W,0.13,309.62,260826,,,A*10
void parse_rmc(char* nmea)
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


