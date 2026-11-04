#ifndef HCSR05_H
#define HCSR05_H

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct
{
    gpio_num_t trig_pin;
    gpio_num_t echo_pin;
    QueueHandle_t echo_queue;
    int64_t last_trigger_us;
} HCSR05_typedef;

// Khoi tao 1 lan, struct phai ton tai suot chuong trinh.
esp_err_t hcsr05_init(HCSR05_typedef *sensor, int trig_pin, int echo_pin);
// Chi 1 task goi ham nay. Cho Queue, khong quay vong doi ECHO.
esp_err_t hcsr05_read(HCSR05_typedef *sensor, float *distance_cm);

#endif
