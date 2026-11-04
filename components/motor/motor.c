#include "motor.h"
void motor_init(Motor_typedef *motor,int ena_pin1,int in1_pin1,int in2_pin2,ledc_channel_t pwm_channel1,ledc_timer_t pwm_timer1){
    motor->ena_pin =ena_pin1;
    motor->in1_pin =in1_pin1;
    motor->in2_pin=in2_pin2;
    motor->pwm_channel=pwm_channel1;
    motor->pwm_timer=pwm_timer1;
    gpio_set_direction(motor->in1_pin,GPIO_MODE_OUTPUT);
    gpio_set_direction(motor->in2_pin,GPIO_MODE_OUTPUT);
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = motor->pwm_timer,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .freq_hz          = 5000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = motor->pwm_channel,
        .timer_sel      = motor->pwm_timer,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = motor->ena_pin,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

}
void motor_forward(Motor_typedef *motor,uint32_t speed){
    gpio_set_level(motor->in1_pin,1);
    gpio_set_level(motor->in2_pin,0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE,motor->pwm_channel,speed);
    ledc_update_duty(LEDC_LOW_SPEED_MODE,motor->pwm_channel);

}
void motor_backward(Motor_typedef *motor,uint32_t speed){
    gpio_set_level(motor->in1_pin,0);
    gpio_set_level(motor->in2_pin,1);
    ledc_set_duty(LEDC_LOW_SPEED_MODE,motor->pwm_channel,speed);
    ledc_update_duty(LEDC_LOW_SPEED_MODE,motor->pwm_channel);
}
void motor_stop(Motor_typedef *motor)
{
    gpio_set_level(motor->in1_pin, 0);
    gpio_set_level(motor->in2_pin, 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, motor->pwm_channel, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, motor->pwm_channel);
}