#include "safety.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

static void safety_task(void *arg)
{
    Safety_typedef *safety = (Safety_typedef *)arg;
    Car_typedef *car = safety->car;

    while (1)
    {
        // Dang khoa: task ngu tai day, khong phat TRIG.
        // Mo khoa: bit duoc set, task tiep tuc chay.
        xEventGroupWaitBits(
            car->state_event,
            CAR_UNLOCKED_BIT,
            pdFALSE,       // Khong xoa bit sau khi cho
            pdTRUE,
            portMAX_DELAY
        );

        // Ghi nhan lan mo khoa truoc khi do.
        xSemaphoreTake(car->cmd_mutex, portMAX_DELAY);

        bool unlocked = car->unlocked;
        uint32_t generation = car->unlock_generation;

        xSemaphoreGive(car->cmd_mutex);

        // Co the xe vua bi khoa sau khi task duoc danh thuc.
        if (!unlocked)
        {
            continue;
        }

        float distance_cm;

        // Khong giu mutex trong luc cho cam bien.
        esp_err_t ret = hcsr05_read(
            &safety->sensor,
            &distance_cm
        );

        // Loi do hoac vat can gan: chan xe.
        bool blocked =
            (ret != ESP_OK) ||
            (distance_cm <= SAFETY_STOP_CM);

        bool updated = false;

        xSemaphoreTake(car->cmd_mutex, portMAX_DELAY);

        // Chi dung ket qua neu van trong cung lan mo khoa.
        if (car->unlocked &&
            car->unlock_generation == generation)
        {
            // Xoa lenh cu khi bi chan hoac vua het chan.
            if (blocked || car->obstacle)
            {
                car->command = 'x';
            }

            car->obstacle = blocked;
            updated = true;
        }

        xSemaphoreGive(car->cmd_mutex);

        if (updated)
        {
            xTaskNotifyGive(car->task_handle);
        }

        vTaskDelay(pdMS_TO_TICKS(SAFETY_PERIOD_MS));
    }
}

void safety_init(Safety_typedef *safety,
                 Car_typedef *car,
                 int trig_pin,
                 int echo_pin)
{
    safety->car = car;

    ESP_ERROR_CHECK(
        hcsr05_init(&safety->sensor, trig_pin, echo_pin)
    );

    BaseType_t ret = xTaskCreate(
        safety_task,
        "SafetyTask",
        3072,
        safety,
        7,
        NULL
    );

    configASSERT(ret == pdPASS);
}