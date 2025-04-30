/*
 * Motor.h
 *
 * Created: 2025-04-22 16:30:37
 *  Author: Zeng Li
 */ 


#ifndef MOTOR_H_
#define MOTOR_H_

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <asf.h>



typedef struct {
	uint8_t tcc_channel;  // TCC1 W[0/1]
	uint32_t pin_pwm;     // PA06/PA17
	uint32_t pin_feedback; //PB02/PB03
	int16_t targetSpeed;  //-120????120
	int16_t currentSpeed;
	int16_t currentAngle;
} MotorController;

extern MotorController Motor_L, Motor_R;


// Global Variable
extern SemaphoreHandle_t xAngleMutex;


// Function Definitions
void set_control_port(uint32_t port, uint32_t channel);
void Set_TCC_GLCK(void);
void servo_control_init(void);
void set_servo_speed(MotorController *Motor, int16_t speed);
void set_motors_speeds(int16_t left, int16_t right);
void servo_control_task(void *pvParameters);

void servo_feedback_init(void);

void move_forward(void);
void move_backward(void);
void move_left(void);
void move_right(void);
void stop(void);



void set(int16_t right);
#endif /* MOTOR_H_ */