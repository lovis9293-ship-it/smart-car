#ifndef CAR_H
#define CAR_H

#include "motor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdbool.h>
#include "freertos/task.h"
#include <stdint.h>
#include "freertos/event_groups.h"

#define CAR_UNLOCKED_BIT BIT0

typedef struct
{
    Motor_typedef left;
    Motor_typedef right;
    SemaphoreHandle_t cmd_mutex;
    TaskHandle_t task_handle;
    bool unlocked;
    bool obstacle;
    char command;
    int speed_level;

    EventGroupHandle_t state_event;
    uint32_t unlock_generation;

} Car_typedef;
// Khoi tao xe
void car_init(Car_typedef *car);
// Nhan lenh dieu khien
void car_set_command(Car_typedef *car, char cmd);

void car_set_unlocked(Car_typedef *car, bool unlocked);

#endif