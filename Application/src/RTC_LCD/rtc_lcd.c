//#include "rtc.h"
#include <string.h>
#include "SerialConsole.h"
#include "rtc_lcd.h"
#include "../LCD/LCD.h"
#include "flag.h"

// Callback function pointer
static rtc_callback_t rtc_user_callback = NULL;

// RTC interrupt handler


// Initialize RTC in clock/calendar mode
void rtc_initialize(void)
{
	// Define your own RTC_Handler elsewhere
	#define RTC_CALENDAR_INTERRUPT_DISABLE  // Add this to prevent ASF handler

	// Enable RTC clock in the power manager
	PM->APBAMASK.reg |= PM_APBAMASK_RTC;
	
	// Configure GCLK for RTC
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(RTC_GCLK_ID) |
	GCLK_CLKCTRL_GEN_GCLK4 |
	GCLK_CLKCTRL_CLKEN;
	
	// Wait for synchronization
	while (GCLK->STATUS.bit.SYNCBUSY);
	
	// Disable RTC first to modify settings
	RTC->MODE2.CTRL.reg &= ~RTC_MODE2_CTRL_ENABLE;
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
	
	// Software reset
	RTC->MODE2.CTRL.reg |= RTC_MODE2_CTRL_SWRST;
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
	while (RTC->MODE2.CTRL.bit.SWRST);
	
	// Configure RTC in clock/calendar mode (MODE2)
	RTC->MODE2.CTRL.reg = RTC_MODE2_CTRL_MODE_CLOCK |
	RTC_MODE2_CTRL_PRESCALER_DIV1024 |
	RTC_MODE2_CTRL_CLKREP;
	
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
	
	// Configure default time (12:00:00)
	RTC->MODE2.CLOCK.reg = RTC_MODE2_CLOCK_HOUR(22) |
	RTC_MODE2_CLOCK_MINUTE(30) |
	RTC_MODE2_CLOCK_SECOND(56) |
	RTC_MODE2_CLOCK_DAY(25) |
	RTC_MODE2_CLOCK_MONTH(4) |
	RTC_MODE2_CLOCK_YEAR(25);
	
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
	
	// Set up RTC alarm interrupt
	RTC->MODE2.INTENSET.reg = RTC_MODE2_INTENSET_ALARM0;
	
	// Set up interrupt priority
	NVIC_SetPriority(RTC_IRQn, 3);
	NVIC_EnableIRQ(RTC_IRQn);
	
	// Enable RTC
	RTC->MODE2.CTRL.reg |= RTC_MODE2_CTRL_ENABLE;
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
	
	SerialConsoleWriteString("RTC initialized in clock/calendar mode\r\n");
}

// Set the RTC time
void rtc_set_time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
	// Wait until RTC is ready
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
	
	// Read the current value to preserve date
	uint32_t clock_value = RTC->MODE2.CLOCK.reg;
	
	// Update only the time fields
	clock_value &= ~(RTC_MODE2_CLOCK_HOUR_Msk | RTC_MODE2_CLOCK_MINUTE_Msk | RTC_MODE2_CLOCK_SECOND_Msk);
	clock_value |= RTC_MODE2_CLOCK_HOUR(hours) | RTC_MODE2_CLOCK_MINUTE(minutes) | RTC_MODE2_CLOCK_SECOND(seconds);
	
	// Write the new value
	RTC->MODE2.CLOCK.reg = clock_value;
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
}

// Set the RTC date
void rtc_set_date(uint8_t day, uint8_t month, uint16_t year)
{
	// Wait until RTC is ready
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
	
	// Read the current value to preserve time
	uint32_t clock_value = RTC->MODE2.CLOCK.reg;
	
	// Update only the date fields
	clock_value &= ~(RTC_MODE2_CLOCK_YEAR_Msk | RTC_MODE2_CLOCK_MONTH_Msk | RTC_MODE2_CLOCK_DAY_Msk);
	clock_value |= RTC_MODE2_CLOCK_DAY(day) | RTC_MODE2_CLOCK_MONTH(month) | RTC_MODE2_CLOCK_YEAR(year % 100);
	
	// Write the new value
	RTC->MODE2.CLOCK.reg = clock_value;
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
}

// Get the current RTC time and date
void rtc_get_time(rtc_time_t *time)
{
	// Wait until RTC is ready
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
	
	// Read the clock register
	uint32_t clock_value = RTC->MODE2.CLOCK.reg;
	
	// Extract fields
	time->seconds = (clock_value & RTC_MODE2_CLOCK_SECOND_Msk) >> RTC_MODE2_CLOCK_SECOND_Pos;
	time->minutes = (clock_value & RTC_MODE2_CLOCK_MINUTE_Msk) >> RTC_MODE2_CLOCK_MINUTE_Pos;
	time->hours = (clock_value & RTC_MODE2_CLOCK_HOUR_Msk) >> RTC_MODE2_CLOCK_HOUR_Pos;
	time->day = (clock_value & RTC_MODE2_CLOCK_DAY_Msk) >> RTC_MODE2_CLOCK_DAY_Pos;
	time->month = (clock_value & RTC_MODE2_CLOCK_MONTH_Msk) >> RTC_MODE2_CLOCK_MONTH_Pos;
	time->year = 2000 + ((clock_value & RTC_MODE2_CLOCK_YEAR_Msk) >> RTC_MODE2_CLOCK_YEAR_Pos);
}

// Set an alarm for a specific time
void rtc_set_alarm(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
	// Wait until RTC is ready
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
	
	// Configure alarm 0
	RTC->MODE2.Mode2Alarm[0].ALARM.reg = RTC_MODE2_ALARM_HOUR(hours)
	| RTC_MODE2_ALARM_MINUTE(minutes)
	| RTC_MODE2_ALARM_SECOND(seconds);
	
	// Set mask to trigger on hour:minute:second match
	RTC->MODE2.Mode2Alarm[0].MASK.reg = RTC_MODE2_MASK_SEL_HHMMSS;
	
	while (RTC->MODE2.STATUS.bit.SYNCBUSY);
}

// Enable RTC alarm interrupt
void rtc_enable_alarm_interrupt(void)
{
	// Enable RTC alarm interrupt
	RTC->MODE2.INTENSET.reg = RTC_MODE2_INTENSET_ALARM0;
}

// Disable RTC alarm interrupt
void rtc_disable_alarm_interrupt(void)
{
	// Disable RTC alarm interrupt
	RTC->MODE2.INTENCLR.reg = RTC_MODE2_INTENCLR_ALARM0;
}

// Set user callback function
void rtc_set_callback(rtc_callback_t callback)
{
	rtc_user_callback = callback;
}


// Global variables
struct spi_module spi_master_instance;
struct spi_slave_inst lcd_slave;
static volatile bool rtc_alarm_triggered = false;

// Font definition - 5x8 font
static const uint8_t font[][5] = {
	{0x00, 0x00, 0x00, 0x00, 0x00}, // 32 Space
	{0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 !
	{0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "
	{0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 #
	{0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 $
	{0x23, 0x13, 0x08, 0x64, 0x62}, // 37 %
	{0x36, 0x49, 0x55, 0x22, 0x50}, // 38 &
	{0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '
	{0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 (
	{0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 )
	{0x08, 0x2A, 0x1C, 0x2A, 0x08}, // 42 *
	{0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 +
	{0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ,
	{0x08, 0x08, 0x08, 0x08, 0x08}, // 45 -
	{0x00, 0x60, 0x60, 0x00, 0x00}, // 46 .
	{0x20, 0x10, 0x08, 0x04, 0x02}, // 47 /
	{0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 0
	{0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 1
	{0x42, 0x61, 0x51, 0x49, 0x46}, // 50 2
	{0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 3
	{0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 4
	{0x27, 0x45, 0x45, 0x45, 0x39}, // 53 5
	{0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 6
	{0x01, 0x71, 0x09, 0x05, 0x03}, // 55 7
	{0x36, 0x49, 0x49, 0x49, 0x36}, // 56 8
	{0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 9
	{0x00, 0x36, 0x36, 0x00, 0x00}, // 58 :
	{0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ;
	{0x00, 0x08, 0x14, 0x22, 0x41}, // 60
	{0x14, 0x14, 0x14, 0x14, 0x14}, // 61 =
	{0x41, 0x22, 0x14, 0x08, 0x00}, // 62 >
	{0x02, 0x01, 0x51, 0x09, 0x06}, // 63 ?
	{0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 @
	{0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 A
	{0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 B
	{0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 C
	{0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 D
	{0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 E
	{0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 F
	{0x3E, 0x41, 0x41, 0x49, 0x7A}, // 71 G
	{0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 H
	{0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 I
	{0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 J
	{0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 K
	{0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 L
	{0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 M
	{0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 N
	{0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 O
	{0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 P
	{0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 Q
	{0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 R
	{0x46, 0x49, 0x49, 0x49, 0x31}, // 83 S
	{0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 T
	{0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 U
	{0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 V
	{0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 W
	{0x63, 0x14, 0x08, 0x14, 0x63}, // 88 X
	{0x07, 0x08, 0x70, 0x08, 0x07}, // 89 Y
	{0x61, 0x51, 0x49, 0x45, 0x43}, // 90 Z
	{0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 [
	{0x02, 0x04, 0x08, 0x10, 0x20}, // 92 backslash
	{0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ]
	{0x04, 0x02, 0x01, 0x02, 0x04}, // 94 ^
	{0x40, 0x40, 0x40, 0x40, 0x40}, // 95 _
	{0x00, 0x01, 0x02, 0x04, 0x00}, // 96 `
	{0x20, 0x54, 0x54, 0x54, 0x78}, // 97 a
	{0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 b
	{0x38, 0x44, 0x44, 0x44, 0x20}, // 99 c
	{0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 d
	{0x38, 0x54, 0x54, 0x54, 0x18}, // 101 e
	{0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 f
	{0x0C, 0x52, 0x52, 0x52, 0x3E}, // 103 g
	{0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 h
	{0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 i
	{0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 j
	{0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 k
	{0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 l
	{0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 m
	{0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 n
	{0x38, 0x44, 0x44, 0x44, 0x38}, // 111 o
	{0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 p
	{0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 q
	{0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 r
	{0x48, 0x54, 0x54, 0x54, 0x20}, // 115 s
	{0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 t
	{0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 u
	{0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 v
	{0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 w
	{0x44, 0x28, 0x10, 0x28, 0x44}, // 120 x
	{0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 y
	{0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 z
	{0x00, 0x08, 0x36, 0x41, 0x00}, // 123 {
	{0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 |
	{0x00, 0x41, 0x36, 0x08, 0x00}, // 125 }
	{0x10, 0x08, 0x08, 0x10, 0x08}, // 126 ~
	{0x78, 0x46, 0x41, 0x46, 0x78}  // 127 DEL
};

// RTC alarm callback
static void rtc_alarm_callback(void)
{
	rtc_alarm_triggered = true;
}

//-----------------------------------------------------------------------------------
// LCD initialization and functions
//-----------------------------------------------------------------------------------
void lcd_draw_char(uint8_t x, uint8_t y, char c, uint16_t color, uint16_t bg_color, uint8_t size)
{
	if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT) || ((x + 6 * size - 1) < 0) || ((y + 8 * size - 1) < 0))
	return;
	
	// Check for printable ASCII character
	if ((c < 32) || (c > 126))
	c = '?';
	
	// Character index in font table
	c -= 32;
	
	for (uint8_t i = 0; i < 5; i++) {
		uint8_t line = font[c][i];
		
		for (uint8_t j = 0; j < 8; j++) {
			if (line & 0x1) {
				if (size == 1) {
					lcd_fill_rect(x + i, y + j, 1, 1, color);
					} else {
					lcd_fill_rect(x + i * size, y + j * size, size, size, color);
				}
				} else if (bg_color != color) {
				if (size == 1) {
					lcd_fill_rect(x + i, y + j, 1, 1, bg_color);
					} else {
					lcd_fill_rect(x + i * size, y + j * size, size, size, bg_color);
				}
			}
			line >>= 1;
		}
	}
	
	// Draw character spacing (1 pixel)
	if (bg_color != color) {
		if (size == 1) {
			lcd_fill_rect(x + 5, y, 1, 8, bg_color);
			} else {
			lcd_fill_rect(x + 5 * size, y, size, 8 * size, bg_color);
		}
	}
}

void lcd_draw_text(uint8_t x, uint8_t y, const char *text, uint16_t color, uint16_t bg_color, uint8_t size)
{
	uint8_t cursor_x = x;
	uint8_t cursor_y = y;
	
	while (*text) {
		if (*text == '\n') {
			cursor_x = x;
			cursor_y += 8 * size;
			} else if (*text == '\r') {
			// Skip carriage return
			} else {
			lcd_draw_char(cursor_x, cursor_y, *text, color, bg_color, size);
			cursor_x += 6 * size;
			
			// Wrap text if it reaches the end of the screen
			if (cursor_x > (ST7735_WIDTH - 6 * size)) {
				cursor_x = x;
				cursor_y += 8 * size;
			}
		}
		text++;
	}
}

void lcd_update_time_display(rtc_time_t *time)
{
	static uint8_t last_hours = 255;
	static uint8_t last_minutes = 255;
	static uint8_t last_seconds = 255;
	static uint16_t last_year = 0;
	static uint8_t last_month = 0;
	static uint8_t last_day = 0;
	
	uint8_t display_hours = time->hours;
	if (display_hours == 24) {
		display_hours = 0;
		time->day += 1;
	}
	
	char str_buffer[2];
	
	if (last_hours != display_hours) {
		if (last_hours / 10 != display_hours / 10) {
			str_buffer[0] = '0' + (display_hours / 10);
			str_buffer[1] = '\0';
			lcd_fill_rect(46+5, 15, 18, 24, ST7735_BLACK);
			lcd_draw_text(46+5, 15, str_buffer, ST7735_ORANGE, ST7735_BLACK, 4);
		}
		
		if (last_hours % 10 != display_hours % 10) {
			str_buffer[0] = '0' + (display_hours % 10);
			str_buffer[1] = '\0';
			lcd_fill_rect(70+5, 15, 18, 24, ST7735_BLACK); 
			lcd_draw_text(70+5, 15, str_buffer, ST7735_ORANGE, ST7735_BLACK, 4);
		}
		
		lcd_draw_text(88+5, 15, ":", ST7735_ORANGE, ST7735_BLACK, 4);
		
		last_hours = display_hours;
	}
	
	if (last_minutes != time->minutes) {
		if (last_minutes / 10 != time->minutes / 10) {
			str_buffer[0] = '0' + (time->minutes / 10);
			str_buffer[1] = '\0';
			lcd_fill_rect(46+5, 50+3, 18, 24, ST7735_BLACK); 
			lcd_draw_text(46+5, 50+3, str_buffer, ST7735_LIME_GREEN, ST7735_BLACK, 4);
		}
		
		if (last_minutes % 10 != time->minutes % 10) {
			str_buffer[0] = '0' + (time->minutes % 10);
			str_buffer[1] = '\0';
			lcd_fill_rect(70+5, 50+3, 18, 24, ST7735_BLACK); 
			lcd_draw_text(70+5, 50+3, str_buffer, ST7735_LIME_GREEN, ST7735_BLACK, 4);
		}
		
		lcd_draw_text(88+5, 50+3, ":", ST7735_LIME_GREEN, ST7735_BLACK, 4);
		
		last_minutes = time->minutes;
	}
	
	if (last_seconds != time->seconds) {
		if (last_seconds / 10 != time->seconds / 10) {
			str_buffer[0] = '0' + (time->seconds / 10);
			str_buffer[1] = '\0';
			lcd_fill_rect(46+5, 85+6, 18, 24, ST7735_BLACK); 
			lcd_draw_text(46+5, 85+6, str_buffer, ST7735_PURPLE, ST7735_BLACK, 3);
		}
		
		if (last_seconds % 10 != time->seconds % 10) {
			str_buffer[0] = '0' + (time->seconds % 10);
			str_buffer[1] = '\0';
			lcd_fill_rect(70+5, 85+6, 18, 24, ST7735_BLACK); 
			lcd_draw_text(70+5, 85+6, str_buffer, ST7735_PURPLE, ST7735_BLACK, 3);
		}
		
		lcd_draw_text(88+5, 85+6, " ", ST7735_PURPLE, ST7735_BLACK, 3);
		last_seconds = time->seconds;
	}
	
	if (last_year != time->year || last_month != time->month || last_day != time->day) {
		char date_str[20];
		sprintf(date_str, "%04d-%02d-%02d", time->year, time->month, time->day);
		lcd_fill_rect(100, 120, 100, 8, ST7735_BLACK);
		lcd_draw_text(100, 120, date_str, ST7735_CYAN, ST7735_BLACK, 1);
		
		last_year = time->year;
		last_month = time->month;
		last_day = time->day;
	}
}

//For Health Monitor
void lcd_update_time_display_one_time(rtc_time_t *time)
{
	static uint8_t last_hours = 255;
	static uint8_t last_minutes = 255;
	static uint8_t last_seconds = 255;
	static uint16_t last_year = 0;
	static uint8_t last_month = 0;
	static uint8_t last_day = 0;
	
	uint8_t display_hours = time->hours;
	
	if (display_hours == 24) {
		display_hours = 0;
		time->day += 1;
	}
	
	char str_buffer[2];
	
	str_buffer[0] = '0' + (display_hours / 10);
	str_buffer[1] = '\0';
	lcd_fill_rect(46+5, 15, 18, 24, ST7735_BLACK); 
	lcd_draw_text(46+5, 15, str_buffer, ST7735_ORANGE, ST7735_BLACK, 4);
	
	str_buffer[0] = '0' + (display_hours % 10);
	str_buffer[1] = '\0';
	lcd_fill_rect(70+5, 15, 18, 24, ST7735_BLACK); 
	lcd_draw_text(70+5, 15, str_buffer, ST7735_ORANGE, ST7735_BLACK, 4);

	
	lcd_draw_text(88+5, 15, ":", ST7735_ORANGE, ST7735_BLACK, 4);
	
	last_hours = display_hours;

	str_buffer[0] = '0' + (time->minutes / 10);
	str_buffer[1] = '\0';
	lcd_fill_rect(46+5, 50+3, 18, 24, ST7735_BLACK); 
	lcd_draw_text(46+5, 50+3, str_buffer, ST7735_LIME_GREEN, ST7735_BLACK, 4);
	
	str_buffer[0] = '0' + (time->minutes % 10);
	str_buffer[1] = '\0';
	lcd_fill_rect(70+5, 50+3, 18, 24, ST7735_BLACK); 
	lcd_draw_text(70+5, 50+3, str_buffer, ST7735_LIME_GREEN, ST7735_BLACK, 4);
	
	lcd_draw_text(88+5, 50+3, ":", ST7735_LIME_GREEN, ST7735_BLACK, 4);
	last_minutes = time->minutes;

	str_buffer[0] = '0' + (time->seconds / 10);
	str_buffer[1] = '\0';
	lcd_fill_rect(46+5, 85+6, 18, 24, ST7735_BLACK); 
	lcd_draw_text(46+5, 85+6, str_buffer, ST7735_PURPLE, ST7735_BLACK, 3);
	
	str_buffer[0] = '0' + (time->seconds % 10);
	str_buffer[1] = '\0';
	lcd_fill_rect(70+5, 85+6, 18, 24, ST7735_BLACK); 
	lcd_draw_text(70+5, 85+6, str_buffer, ST7735_PURPLE, ST7735_BLACK, 3);
	lcd_draw_text(88+5, 85+6, " ", ST7735_PURPLE, ST7735_BLACK, 3);
	last_seconds = time->seconds;
	
	char date_str[20];
	sprintf(date_str, "%04d-%02d-%02d", time->year, time->month, time->day);
	lcd_fill_rect(100, 120, 100, 8, ST7735_BLACK);
	lcd_draw_text(100, 120, date_str, ST7735_CYAN, ST7735_BLACK, 1);
	
	last_year = time->year;
	last_month = time->month;
	last_day = time->day;
}

//Count Down Time Task
void counting_down_time(void){
	char str_buffer[2];
	static uint8_t minute = 0;
	static uint8_t second = 10;
	static uint8_t last_minute = 255;
	static uint8_t last_second = 255;
	uint16_t time = minute * 60 + second;
	
	stop_rtc_show_flag = 1; // Stop RTC_Realtime_Display_update
	lcd_fill_rect_dma(0, 0, ST7735_WIDTH, ST7735_HEIGHT, ST7735_BLACK);//Background
	
	static TickType_t Last_time = 0;
	volatile TickType_t target_time = xTaskGetTickCount() + pdMS_TO_TICKS(minute * 60000 + second * 1000) + pdMS_TO_TICKS(200);//10s + 200ms switch time
	
	while(health_monitor_flag == false && xTaskGetTickCount() <= target_time){
		TickType_t update_time = xTaskGetTickCount();
		if (update_time - Last_time >= 1000)
		{
			time = (target_time - xTaskGetTickCount()) / 1000;
			minute = time % 60;
			second = time / 60;
			//For Minute
			if(last_minute / 10 != minute / 10){
				str_buffer[0] = '0' + (minute / 10);
				str_buffer[1] = '\0';
				lcd_fill_rect(96+5, 50+3, 18, 24, ST7735_BLACK);
				lcd_draw_text(96+5, 50+3, str_buffer, ST7735_LIME_GREEN, ST7735_BLACK, 4);
			}
			if (last_minute % 10 != minute % 10)
			{
				str_buffer[0] = '0' + (minute % 10);
				str_buffer[1] = '\0';
				lcd_fill_rect(120+5, 50+3, 18, 24, ST7735_BLACK); 
				lcd_draw_text(120+5, 50+3, str_buffer, ST7735_LIME_GREEN, ST7735_BLACK, 4);
			}
			//For Second
			if (last_second / 10 != second / 10) {
				str_buffer[0] = '0' + (second / 10);
				str_buffer[1] = '\0';
				lcd_fill_rect(22+5, 50+3, 18, 24, ST7735_BLACK); 
				lcd_draw_text(22+5, 50+3, str_buffer, ST7735_LIME_GREEN, ST7735_BLACK, 4);
			}
			
			if (last_second % 10 != second % 10) {
				str_buffer[0] = '0' + (second % 10);
				str_buffer[1] = '\0';
				lcd_fill_rect(46+5, 50+3, 18, 24, ST7735_BLACK); 
				lcd_draw_text(46+5, 50+3, str_buffer, ST7735_LIME_GREEN, ST7735_BLACK, 4);
			}
			lcd_draw_text(74, 50+3, ":", ST7735_LIME_GREEN, ST7735_BLACK, 4);
			last_minute = minute;
			last_second = second;
			Last_time = update_time;
		}
	}
	if(!health_monitor_flag){
		//voice_control_flag = 0;
		//LCD real time
		

		//LED Show for Reminding that time is reached
		explosion_effect();
		meteor_effect(255, 0, 0);
		meteor_effect(0, 255, 0);
		meteor_effect(0, 0, 255);
		rainbow_swirl();
		SK6812_Clear();
		led_flag = 0;
		
		lcd_fill_rect_dma(0, 0, ST7735_WIDTH, ST7735_HEIGHT, ST7735_BLACK);
		lcd_update_time_display_one_time(&current_time);
		stop_rtc_show_flag = 0;
	}
}

//-----------------------------------------------------------------------------------
// Main LCD display task with RTC
//-----------------------------------------------------------------------------------
void rtc_lcd_display_task(void *pvParameters)
{
	char debug_buffer[64];
	
	snprintf(debug_buffer, sizeof(debug_buffer), "LCD RTC Task started - Free heap: %d\r\n",
	xPortGetFreeHeapSize());
	//SerialConsoleWriteString(debug_buffer);
	
	vTaskDelay(pdMS_TO_TICKS(500));
	
	SerialConsoleWriteString("Setting up SPI DMA...\r\n");
	setup_spi_dma();

	SerialConsoleWriteString("Initializing LCD...\r\n");
	lcd_init();
	
	vTaskDelay(pdMS_TO_TICKS(500));
	
	SerialConsoleWriteString("Initializing RTC...\r\n");
	rtc_initialize();
	
	rtc_set_callback(rtc_alarm_callback);
	rtc_set_alarm(0, 0, 0);
	
	rtc_enable_alarm_interrupt();
	lcd_fill_rect_dma(0, 0, ST7735_WIDTH, ST7735_HEIGHT, ST7735_BLACK);
	
	rtc_get_time(&current_time);
	lcd_update_time_display(&current_time);
	
	while (1) {
		if (rtc_alarm_triggered) {
			rtc_alarm_triggered = false;
			
			rtc_get_time(&current_time);
			
			if(stop_rtc_show_flag == 0){
				lcd_update_time_display(&current_time);
			}
			
			snprintf(debug_buffer, sizeof(debug_buffer), "Time: %02d:%02d:%02d, Date: %04d-%02d-%02d\r\n",
			current_time.hours, current_time.minutes, current_time.seconds,
			current_time.year, current_time.month, current_time.day);
			SerialConsoleWriteString(debug_buffer);
		}
		
		static uint32_t last_update = 0;
		uint32_t current_tick = xTaskGetTickCount();
		
		if ((current_tick - last_update) >= pdMS_TO_TICKS(1000)) {
			last_update = current_tick;
			
			rtc_get_time(&current_time);
			
			if (stop_rtc_show_flag == 0)
			{
				lcd_update_time_display(&current_time);
			}
			
		}
		
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}