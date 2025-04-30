#ifndef RTC_H
#define RTC_H

#include <asf.h>
#include "FreeRTOS.h"
#include "task.h"
#include "LED/LED.h"

// Time structure to hold current time
typedef struct {
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	uint8_t day;
	uint8_t month;
	uint16_t year;
} rtc_time_t;

// RTC initialization and control functions
void rtc_initialize(void);
void rtc_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds);
void rtc_set_date(uint8_t day, uint8_t month, uint16_t year);
void rtc_get_time(rtc_time_t *time);
void rtc_set_alarm(uint8_t hours, uint8_t minutes, uint8_t seconds);
void rtc_enable_alarm_interrupt(void);
void rtc_disable_alarm_interrupt(void);

// Callback function prototype
typedef void (*rtc_callback_t)(void);
void rtc_set_callback(rtc_callback_t callback);


void lcd_draw_char(uint8_t x, uint8_t y, char c, uint16_t color, uint16_t bg_color, uint8_t size);
void lcd_draw_text(uint8_t x, uint8_t y, const char *text, uint16_t color, uint16_t bg_color, uint8_t size);
void lcd_update_time_display(rtc_time_t *time);
void lcd_update_time_display_one_time(rtc_time_t *time);

void counting_down_time(void);

// Task functions
void rtc_lcd_display_task(void *pvParameters);

extern volatile rtc_time_t current_time;

#endif // RTC_H