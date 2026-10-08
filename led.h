#ifndef LED_H
#define LED_H
#define LED_CNT 8
void init_led(void);
void disable_led(void);
void enable_led(void);
void update_led(uint8_t *to_show);
#endif
