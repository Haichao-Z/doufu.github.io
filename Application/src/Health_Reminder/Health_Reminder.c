/*
 * Count_time.c
 *
 * Created: 2025-04-25 14:06:04
 *  Author: Zeng Li
 */ 
#include "Health_Reminder/Health_Reminder.h"

// Preset Time 60s
#define COUNTDOWN_TIME 60
#define WORK_TIME 10

// Global Variable



volatile uint32_t time_counter = COUNTDOWN_TIME; //After adding flag, it will be 0. And in the function of flag, it will be set to 10s.
volatile SystemState system_state;
volatile bool timer_expired = false;
volatile TickType_t blink_end_time = 0;  
struct tcc_module tcc_instance;

void configure_tcc0(void)
{
	struct tcc_config config_tcc;
	tcc_get_config_defaults(&config_tcc, TCC0);
	
	// 48MHz
	config_tcc.counter.clock_prescaler = TCC_CLOCK_PRESCALER_DIV1024; // 46.875kHz
	config_tcc.counter.period = 46874; // 1s
	tcc_init(&tcc_instance, TCC0, &config_tcc);
	
	tcc_enable(&tcc_instance);//It should be put in the flag function
	
	// Callback register and enable
	tcc_register_callback(&tcc_instance, tcc0_callback, TCC_CALLBACK_OVERFLOW);
	tcc_enable_callback(&tcc_instance, TCC_CALLBACK_OVERFLOW);
}

//At first, it will enter counting down state, after 10s, it will switch to executing task state(2s).
//After 2s, task will be finished and stop tcc0 and switch to waiting for flag state.
static void tcc0_callback(struct tcc_module *const module_inst)
{
	switch(system_state){
		case counting_down:
		if(--time_counter == 0){
			system_state = executing_task;
			time_counter = WORK_TIME;//10s executing task
			
			//Time is reached
			health_monitor_flag = true;
			blink_end_time = xTaskGetTickCount() + pdMS_TO_TICKS(10000);  
			xSemaphoreGiveFromISR(xHealthMonitorSemaphore, NULL); //Begin Health Monitor
		}
		break;
		case executing_task:
		if(--time_counter == 0){
			//tcc_disable(&tcc_instance);
			system_state = counting_down; 
			time_counter = COUNTDOWN_TIME; 

			//tcc_disable(&tcc_instance);
		}
		break;
		default: break;
	}
}

void vHealthMonitorTask(void *pvParameters){
	while(1){
		if (xSemaphoreTake(xHealthMonitorSemaphore, portMAX_DELAY) == pdTRUE) {
			SerialConsoleWriteString("Health Monitor: Received signal from TCC callback!\r\n");
			stop_rtc_show_flag = 1; //Stop RTC screen update
			voice_control_flag = 11; //If voice control is working, stop it
			lcd_fill_rect_dma(0, 0, ST7735_WIDTH, ST7735_HEIGHT, ST7735_WHITE);
			//TickType_t start_time = xTaskGetTickCount();
			while (health_monitor_flag && xTaskGetTickCount() < blink_end_time) //10s
			{	
				lcd_fill_circle(ST7735_WIDTH/4, ST7735_HEIGHT/2, 18, ST7735_BLACK);
				lcd_fill_circle(3*ST7735_WIDTH/4, ST7735_HEIGHT/2, 18, ST7735_BLACK);
				lcd_fill_rect(ST7735_WIDTH/4, ST7735_HEIGHT/2, ST7735_WIDTH/2, 4, ST7735_BLACK);
				
				explosion_effect();				
				meteor_effect(255, 0, 0); 
				meteor_effect(0, 255, 0);   
				meteor_effect(0, 0, 255);   				
				rainbow_swirl();

				
			}
			//lcd_fill_rect_dma(0, 0, ST7735_WIDTH, ST7735_HEIGHT, ST7735_WHITE);
			lcd_fill_rect_dma(0, 0, ST7735_WIDTH, ST7735_HEIGHT, ST7735_BLACK);
			lcd_update_time_display_one_time(&current_time);
			SK6812_Clear();
			health_monitor_flag = false;
			led_flag = 0;
			stop_rtc_show_flag = 0;
		}		
		//vTaskDelay(pdMS_TO_TICKS(300));
	}
			
}