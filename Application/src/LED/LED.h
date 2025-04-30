/*
 * LED.h
 *
 * Created: 2025-04-26 15:31:04
 *  Author: Zeng Li
 */ 


#ifndef LED_H_
#define LED_H_

#include "port.h"
#include "asf.h"
#include "SerialConsole.h"
#include "Health_Reminder/Health_Reminder.h"
#include "flag.h"

#define SK6812_PIN PIN_PA22
#define SK6812_PIN_MASK (1 << 22)

#define SK6812_RIGHT PIN_PA21
#define SK6812_RIGHT_MASK (1 << 21)

#define SK6812_PORT PORT->Group[PIN_PA22 / 32]
#define SK6812_PIN_MASK (1 << (PIN_PA22 % 32))

#define SK6812_PORT_RIGHT PORT->Group[PIN_PA21 / 32]
#define SK6812_RIGHT_MASK (1 << (PIN_PA21 % 32))

#define LED_COUNT 8 

typedef struct {
	uint8_t green;
	uint8_t red;
	uint8_t blue;
} RGB_t;

void SK6812_Init(void);
static inline void send_bit(uint8_t bit);
static void send_byte(uint8_t byte);
void SK6812_Send(void);
void SK6812_SetLED(uint16_t index, uint8_t red, uint8_t green, uint8_t blue);
void SK6812_SetAll(uint8_t red, uint8_t green, uint8_t blue);
void SK6812_Clear(void);
void LED_Task(void *pvParameters);

void meteor_effect(uint8_t r, uint8_t g, uint8_t b);
void explosion_effect();
void rainbow_swirl();
void BLink_one_color();
void Hold_in_one_color();

#endif /* LED_H_ */