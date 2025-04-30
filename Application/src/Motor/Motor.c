/*
 * Motor.c
 *
 * Created: 2025-04-22 16:26:08
 *  Author: Zeng Li
 */ 

#include "Motor.h"

struct tcc_config config_tcc;
struct tcc_module tcc0_instance;
struct tcc_module tcc1_instance;
struct tcc_module tcc2_instance;

struct tc_config confic_tc;

MotorController Motor_L = {
	.tcc_channel = 0, //TCC1 WO[0]
	.pin_pwm = PIN_PA06,  
	.pin_feedback = PIN_PB02,//TC6
	.targetSpeed = 0
};

MotorController Motor_R = {
	.tcc_channel = 1,   //TCC2 WO[1]
	.pin_pwm = PIN_PA17,  
	.pin_feedback = PIN_PB03,//TC6
	.targetSpeed = 0
};


/*******************************************Control Line**********************************/
void set_control_port(uint32_t port, uint32_t channel){
	struct system_pinmux_config pin_conf;
	system_pinmux_get_config_defaults(&pin_conf);
	pin_conf.mux_position = channel;
	pin_conf.direction = SYSTEM_PINMUX_PIN_DIR_OUTPUT;
	system_pinmux_pin_set_config(port, &pin_conf);
}

void Set_TCC_GLCK(void){
    struct system_gclk_chan_config gclk_chan_cfg;
    system_gclk_chan_get_config_defaults(&gclk_chan_cfg);

    // TCC1£¨48MHz£©
    gclk_chan_cfg.source_generator = GCLK_GENERATOR_0; // GCLK0
    system_gclk_chan_set_config(TCC1_GCLK_ID, &gclk_chan_cfg);
    system_gclk_chan_enable(TCC1_GCLK_ID);

    // TCC2£¨48MHz£©
    system_gclk_chan_set_config(TCC2_GCLK_ID, &gclk_chan_cfg);
    system_gclk_chan_enable(TCC2_GCLK_ID);
}

//Motor_L(PA06)   Motor_R(PA17)
void servo_control_init(void){
	
	//Set_TCC_GLCK();
	
	//Use Tcc1 to control servo(50Hz PWM)
	tcc_get_config_defaults(&config_tcc, TCC1);
	//// 48MHz/64/50-1=14999
	config_tcc.counter.clock_prescaler = TCC_CLOCK_PRESCALER_DIV64;
	config_tcc.counter.period = 14999; //20ms
	config_tcc.compare.wave_generation = TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
	config_tcc.compare.match[0] = (uint32_t)(1500 * (48000000 / 64) / 1000000);//1500us(Stop)
	tcc_init(&tcc1_instance, TCC1, &config_tcc);
	tcc_enable(&tcc1_instance);
	//
	//Use Tcc2 to control servo(50Hz PWM)
    tcc_get_config_defaults(&config_tcc, TCC2);
    config_tcc.counter.clock_prescaler = TCC_CLOCK_PRESCALER_DIV64;
    config_tcc.counter.period = 14999;
    config_tcc.compare.wave_generation = TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
	config_tcc.compare.match[1] = (uint32_t)(1500 * (48000000 / 64) / 1000000);//1500us(Stop)
    tcc_init(&tcc2_instance, TCC2, &config_tcc);
    tcc_enable(&tcc2_instance);
	
	////Motor_Left
	set_control_port(Motor_L.pin_pwm, MUX_PA06E_TCC1_WO0);
	////Motor_Right
	set_control_port(Motor_R.pin_pwm, MUX_PA17E_TCC2_WO1);
	
}

void set_servo_speed(MotorController *Motor, int16_t speed){
	//-120RPM¡ª¡ª120RPM
	speed = (speed < -120) ? -120 : (speed > 120) ? 120 : speed;
	//Map Speed
	uint16_t pulse_width;
	if (speed == 0){
		pulse_width = 1500; //Stop
	}
	else{
		pulse_width = 1500 - (220 * speed / 120);//Clockwise(speed > 0)  Counterclockwise(speed < 0)	
	}
	
	//Transform pulse_width into match value (48Mhz / 64 / 1000000 = 0.75) 0.75 tick/us
	uint32_t match_value = (uint32_t)(pulse_width * 0.75f);
    if (Motor == &Motor_L) {
	    tcc_set_compare_value(&tcc1_instance, Motor->tcc_channel, match_value); // TCC1
	    } else {
	    tcc_set_compare_value(&tcc2_instance, Motor->tcc_channel, match_value); // TCC2
    }
}

void set_motors_speeds(int16_t left, int16_t right) {
	set_servo_speed(&Motor_L, left);
	set_servo_speed(&Motor_R, right);
}

void set(int16_t right){
	set_servo_speed(&Motor_R,right);
}



/***********************************************Easy Control*********************************/
void move_forward(void){
	set_motors_speeds(-27, 25);
}

void move_backward(void){
	set_motors_speeds(21, -25);
}

void move_left(void){
	set_motors_speeds(-15,35);
}

void move_right(void){
	set_motors_speeds(-35, 15);
}

void stop(void){
	set_motors_speeds(0, 0);
}


/***********************************************Control Task**********************************/
void servo_control_task(void *pvParameters) {
	MotorController *Motor = (MotorController *)pvParameters;
	const TickType_t xDelay = pdMS_TO_TICKS(20);  // 50Hz
	
	for(;;) {
		set_servo_speed(Motor, Motor->targetSpeed);
		vTaskDelay(xDelay);
	}
}