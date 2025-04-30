/*
 * flag.h
 *
 * Created: 2025-04-26 17:47:32
 *  Author: 13356
 */ 


#ifndef FLAG_H_
#define FLAG_H_

extern SemaphoreHandle_t xHealthMonitorSemaphore; // For Health Reminder
extern volatile int stop_rtc_show_flag; //stop show time when health monitor is working
extern volatile int led_flag; //four led mode
extern volatile bool health_monitor_flag; //Control the function of health monitor
extern volatile int rtc_mode; //Real time mode && Count down time mode
extern volatile int voice_control_flag;
extern volatile bool stop_all_flag;
extern volatile bool lift_flag;

#endif /* FLAG_H_ */