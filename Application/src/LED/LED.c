/*
 * LED.c
 *
 * Created: 2025-04-26 15:30:56
 *  Author: 13356
 */ 

#include "LED.h"


RGB_t ledBuffer[LED_COUNT];

// SK6812_initialization
void SK6812_Init(void)
{
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	
	pin_conf.direction = PORT_PIN_DIR_OUTPUT;
	pin_conf.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(SK6812_PIN, &pin_conf);
	port_pin_set_output_level(SK6812_PIN, false);
	
	port_pin_set_config(SK6812_RIGHT, &pin_conf);
	port_pin_set_output_level(SK6812_RIGHT, false);
}

static inline void send_bit(uint8_t bit)
{
	if (bit) {
		// Send Code 1: 0.6us high and 0.6us low
		SK6812_PORT.OUTSET.reg = SK6812_PIN_MASK; 
		SK6812_PORT_RIGHT.OUTSET.reg = SK6812_RIGHT_MASK;
		__asm__ volatile(
		"nop\n"
		"nop\n"
		"nop\n"
		"nop\n"
		"nop\n"
		"nop\n"
		"nop\n"
		"nop\n"
		"nop\n"
		"nop\n"
		);
		SK6812_PORT.OUTCLR.reg = SK6812_PIN_MASK;  
		SK6812_PORT_RIGHT.OUTCLR.reg = SK6812_RIGHT_MASK;
		} else {
		// Send Code 0: 0.3us high and 0.9us low
		SK6812_PORT.OUTSET.reg = SK6812_PIN_MASK;  
		SK6812_PORT_RIGHT.OUTSET.reg = SK6812_RIGHT_MASK;
		SK6812_PORT.OUTCLR.reg = SK6812_PIN_MASK;  
		SK6812_PORT_RIGHT.OUTCLR.reg = SK6812_RIGHT_MASK;
	}
}

// Send one Byte
static void send_byte(uint8_t byte)
{
	for (int i = 7; i >= 0; i--) {
		send_bit((byte >> i) & 0x01);
	}
}

// Send RGB Data to all LED
void SK6812_Send(void)
{
	cpu_irq_disable();
	for (int i = 0; i < LED_COUNT; i++) {
		send_byte(ledBuffer[i].green);
		send_byte(ledBuffer[i].red);
		send_byte(ledBuffer[i].blue);
	}
	
	// Send Reset Code£¨>80¦Ìs low level time£©
	port_pin_set_output_level(SK6812_PIN, false);//Left
	port_pin_set_output_level(SK6812_RIGHT, false);
	delay_cycles_us(90);
	
	Enable_global_interrupt();
	vTaskDelay(10);
}

// Single LED Color
void SK6812_SetLED(uint16_t index, uint8_t red, uint8_t green, uint8_t blue)
{
	if (index < LED_COUNT) {
		ledBuffer[index].red = red;
		ledBuffer[index].green = green;
		ledBuffer[index].blue = blue;
	}
}

// Set all LED to the same color
void SK6812_SetAll(uint8_t red, uint8_t green, uint8_t blue)
{
	for (int i = 0; i < LED_COUNT; i++) {
		ledBuffer[i].red = red;
		ledBuffer[i].green = green;
		ledBuffer[i].blue = blue;
	}
}

// Close all LED
void SK6812_Clear(void)
{
	SK6812_SetAll(0, 0, 0);
	SK6812_Send();
}

/**********************************************LED Show and Three Mode******************************************/
void meteor_effect(uint8_t r, uint8_t g, uint8_t b) {
	for (int i = 0; i < LED_COUNT + 5; i++) {
		SK6812_Clear();
		for (int j = 0; j < 5; j++) {
			if (i - j >= 0 && i - j < LED_COUNT) {
				uint8_t fade = 255 - j * 50;
				SK6812_SetLED(i - j, r * fade / 255, g * fade / 255, b * fade / 255);
			}
		}
		SK6812_Send();
		vTaskDelay(30);
	}
}


void explosion_effect() {
	uint8_t r = rand() % 55;
	uint8_t g = rand() % 55;
	uint8_t b = rand() % 55;
	for (int i = 0; i < 10; i++) {
		SK6812_SetAll(r, g, b);
		SK6812_Send();
		vTaskDelay(50);
		SK6812_Clear();
		SK6812_Send();
		vTaskDelay(50);
	}
}

void rainbow_swirl() {
	for (int j = 0; j < 255; j += 5) {
		for (int i = 0; i < LED_COUNT; i++) {
			uint8_t pos = (i * 255 / LED_COUNT + j) % 255;
			if (pos < 85) {
				SK6812_SetLED(i, 255 - pos * 3, pos * 3, 0);
				} else if (pos < 170) {
				pos -= 85;
				SK6812_SetLED(i, 0, 255 - pos * 3, pos * 3);
				} else {
				pos -= 170;
				SK6812_SetLED(i, pos * 3, 0, 255 - pos * 3);
			}
		}
		SK6812_Send();
		vTaskDelay(20);
	}
}

void BLink_one_color(){
	SK6812_SetAll(70,0,0);
	SK6812_Send();
	vTaskDelay(20);
	SK6812_Clear();
	vTaskDelay(20);
}

void Hold_in_one_color(){
	for (int i = 0; i<255; i+=5)
	{
		SK6812_SetAll(0,0,i);
		SK6812_Send();
		vTaskDelay(10);
	}
	for (int i = 255; i>0; i-=5)
	{
		SK6812_SetAll(0,0,i);
		SK6812_Send();
		vTaskDelay(10);
	}
}


/**************************************************************LED Task******************************************/
//flag = 0, No LED
//flag = 1, mode 1 blink
//flag = 2, mode 2 hold in one color
//flag = 3, mode 3 hold in multiple color
//flag = 4, mode 4 Stop LED
void LED_Task(void *pvParameters)
{
	SK6812_Clear();
	while(1){
		if (led_flag == 0){}//No State
		else if(led_flag == 1){
			BLink_one_color();
		}
		else if(led_flag == 2){
			Hold_in_one_color();
		}
		else if(led_flag == 3){
			rainbow_swirl();
		}
		else if(led_flag == 4){
			SK6812_Clear();
			led_flag = 0;
		}
	}
}