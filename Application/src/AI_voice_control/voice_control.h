/*
 * voice_control.h
 *
 * Created: 2025-04-26 23:33:05
 *  Author: 13356
 */ 


#ifndef VOICE_CONTROL_H_
#define VOICE_CONTROL_H_

#include "Motor/Motor.h"
#include "RTC_LCD/rtc_lcd.h"
#include "LCD/LCD.h"
#include "LED/LED.h"
#include "flag.h"
#include "Health_Reminder/Health_Reminder.h"
#include "I2cDriver/I2cDriver.h"

void set_output(uint32_t port);
void Command_Detect(void);
void vVoiceControlTask(void *pvParameters);
//void counting_down_time(void);


#endif /* VOICE_CONTROL_H_ */