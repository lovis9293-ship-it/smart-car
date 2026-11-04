#ifndef MOTOR_H
#define MOTOR_H
#include "driver/gpio.h"
#include "driver/ledc.h"

//----------------STRUCT----------
typedef struct 
{
    /* data */
    int ena_pin;
    int in1_pin;
    int in2_pin;
    ledc_channel_t pwm_channel;
    ledc_timer_t pwm_timer;

}Motor_typedef;



void motor_init(Motor_typedef *motor,int ena_pin1,int in1_pin1,int in2_pin2,ledc_channel_t pwm_channel1,ledc_timer_t pwm_timer1);
void motor_forward(Motor_typedef *motor,uint32_t speed);
void motor_backward(Motor_typedef *motor,uint32_t speed);
void motor_stop(Motor_typedef *motor);
#endif
