/*
 * Count_timeeIncFile1.h
 *
 * Created: 2025-04-25 14:06:15
 *  Author: 13356
 */ 


#ifndef HEALTH_REMINDER_H_
#define HEALTH_REMINDER_H_

#include "tcc.h"
#include "port.h"
#include "tcc_callback.h"
#include "LED/LED.h"
#include "LCD/LCD.h"
#include "flag.h"
#include "RTC_LCD/rtc_lcd.h"

typedef enum{
	Waiting_for_flag,
	counting_down,
	executing_task,
}SystemState;

void configure_tcc0(void);
static void tcc0_callback(struct tcc_module *const module_inst);
void vHealthMonitorTask(void *pvParameters);



#endif /* HEALTH_REMINDER_H_ */