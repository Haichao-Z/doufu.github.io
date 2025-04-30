/*
 * voice_control.c
 *
 * Created: 2025-04-26 23:32:51
 *  Author: 13356
 */ 
#include "voice_control.h"

#define VC02_I2C_ADDRESS 0x28
#define VC02_CMD_REG     0x01
#define I2C_TIMEOUT      pdMS_TO_TICKS(200)

#define PORT1 PIN_PA03 //CTRL M1      Voice Module A25
#define PORT2 PIN_PA02 //CTRL M2	  Voice Module A26
#define PORT3 PIN_PA20 // FB 2		  Voice Module A27
#define PORT4 PIN_PA10 //VBatin		  Voice Module B8

void set_output(uint32_t port){
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	pin_conf.direction = PORT_PIN_DIR_INPUT;
	port_pin_set_config(port, &pin_conf);
}
	

void Command_Detect(void){
	set_output(PORT1);
	set_output(PORT2);
	set_output(PORT3);
	set_output(PORT4);

}

/*
flag = 0 -> No State(Idle Task)
flag = 4 -> Count down time
flag = 1 -> LED show(Blink)
flag = 2 -> LED show(Hold in one color)
flag = 3 -> LED show(Hold in multiple color)
flag = 5 -> Motor(Move forward)
flag = 6 -> Motor(Move backward)
flag = 7 -> Motor(Move leftward)
flag = 8 -> Motor(Move rightward)
flag = 9 -> Motor(Stop)
flag = 10-> CLear all state and return to no state
***********************************************************************/
void vVoiceControlTask(void *pvParameters){
	while(1){
		static int pre_voice_flag = 50;
		uint8_t a = port_pin_get_input_level(PORT1);
		uint8_t b = port_pin_get_input_level(PORT2);
		uint8_t c = port_pin_get_input_level(PORT3);
		uint8_t d = port_pin_get_input_level(PORT4);
		voice_control_flag = 8*d + 4*c + 2*b + a;
		if(voice_control_flag != pre_voice_flag){		
			if (voice_control_flag == 0){}
			else if (voice_control_flag == 1 ){
				//BLink_one_color();
				led_flag = 1;
			}
			else if (voice_control_flag == 2){
				//Hold_in_one_color();
				led_flag = 2;
			}
			else if (voice_control_flag == 3){
				//rainbow_swirl();
				led_flag = 3;
			}		
			else if (voice_control_flag == 4){
				//SK6812_Clear();
				led_flag = 4;
			}
			else if (voice_control_flag == 5){
				counting_down_time();
			}
			else if (voice_control_flag == 6){
				move_forward();
			}
			else if (voice_control_flag == 7){
				move_backward();
			}
			else if (voice_control_flag == 8){
				move_left();
			}
			else if (voice_control_flag == 9){
				move_right();
			}										
			else if (voice_control_flag == 10){
				stop();
			}
			else if (voice_control_flag == 11){//Clear All state and return
				//stop_all_flag = true;
				stop();
				SK6812_Clear();
				voice_control_flag = 0;
			}
			else if (voice_control_flag == 12){//For Test£©
				vTaskDelay(5000);
				voice_control_flag = 6;
			}
			//else if (voice_control_flag == 15){
				//PM->RCAUSE.reg = PM_RCAUSE_SYST;
			//}
		pre_voice_flag = voice_control_flag;
		vTaskDelay(pdMS_TO_TICKS(1000)); 
		}
	
	}

}
//
