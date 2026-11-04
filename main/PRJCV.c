#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
#include "car.h"
#include "blue.h"
#include "rc.h"
#include "safety.h"
#include "freertos/queue.h"

QueueHandle_t rfid_queue;
Car_typedef car;
RC522_Typedef rc522;
Safety_typedef safety;

#define HCSR05_TRIG_PIN 25
#define HCSR05_ECHO_PIN 26

// Bluetooth Callback
//moc khoa UID:  59 F8 8F 29
//the UID:  1A 93 6A E7
static void system_task(void *sys)
{
    uint8_t signal;
    bool bluetooth_fault = false;
    bool unlocked = false;

    while (1)
    {
        if (xQueueReceive(rfid_queue, &signal,
                          portMAX_DELAY) == pdTRUE)
        {
            if (!unlocked)
            {
                // Loi tat Bluetooth truoc do: giu khoa, can reset de khoi phuc.
                if (bluetooth_fault) continue;

                // Mo khoa
                bluetooth_init();
                car_set_unlocked(&car, true);
                unlocked = true;

                ESP_LOGI("SYSTEM", "MO KHOA XE");
            }
            else
            {
                // Yeu cau dung xe
                car_set_unlocked(&car, false);
                unlocked = false;

                // Tat Bluetooth
                if (bluetooth_deinit() == ESP_OK)
                {
                    unlocked = false;
                    ESP_LOGI("SYSTEM", "KHOA XE");
                }
                else
                {
                    bluetooth_fault = true;
                    ESP_LOGE("SYSTEM", "LOI TAT BLUETOOTH - XE VAN KHOA, HAY RESET");
                }
            }
        }
    }
}
void app_main(void)
{
    // Khoi tao xe 1 lan ngay luc boot, mac dinh khoa va dung.
    car_init(&car);
    safety_init(&safety, &car, HCSR05_TRIG_PIN, HCSR05_ECHO_PIN);
    rfid_queue = xQueueCreate(5, sizeof(uint8_t));
    configASSERT(rfid_queue != NULL);
    BaseType_t ret = xTaskCreate(system_task,"SystemTask",4096,NULL,6,NULL);

    configASSERT(ret == pdPASS);
    rc522_init(&rc522,
        16,  // MISO
        13,  // MOSI
        4,  // SCLK
        15,   // SDA / SS
        2   // RST
    );
    
}