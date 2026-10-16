#ifndef GPS_PARSER_H
#define GPS_PARSER_H
#include <stdint.h>
#include <stdbool.h>

#define NMEA_MAX_LENGTH 256
// 本地时间结构体
typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint8_t  weekday; // 0=Sunday, 1=Monday... 6=Saturday
} local_time_t;

extern local_time_t current_local_time;
extern volatile _Bool is_rmc_ready;
extern char rmc[NMEA_MAX_LENGTH];
// 串口接收缓冲解析函数，按字节送入
void GPS_ParseChar(char c);
// 获取解析后的本地时间
void GPS_GetLocalTime(local_time_t *out_time);
void parse_gprmc(char* nmea);
#endif // GPS_PARSER_H
