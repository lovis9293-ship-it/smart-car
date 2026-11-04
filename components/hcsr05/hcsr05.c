#include "hcsr05.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "freertos/task.h"

#define ECHO_TIMEOUT_US 40000
#define TRIGGER_INTERVAL_US 60000

typedef struct
{
    int level;
    int64_t time_us;
} Echo_typedef;

// ISR chi ghi nhan canh ECHO va gui Queue.
// Dang ky ISR service voi flags = 0 (khong yeu cau IRAM).
static void echo_isr(void *arg)
{
    HCSR05_typedef *sensor = (HCSR05_typedef *)arg;
    Echo_typedef event = {
        .time_us = esp_timer_get_time(),
        .level = gpio_get_level(sensor->echo_pin),
    };
    BaseType_t wake = pdFALSE;
    xQueueSendFromISR(sensor->echo_queue, &event, &wake);
    if (wake == pdTRUE) portYIELD_FROM_ISR();
}

esp_err_t hcsr05_init(HCSR05_typedef *sensor, int trig_pin, int echo_pin)
{
    if (!sensor || !GPIO_IS_VALID_OUTPUT_GPIO(trig_pin) ||
        !GPIO_IS_VALID_GPIO(echo_pin) || trig_pin == echo_pin)
        return ESP_ERR_INVALID_ARG;

    sensor->trig_pin = trig_pin;
    sensor->echo_pin = echo_pin;
    sensor->last_trigger_us = 0;
    sensor->echo_queue = xQueueCreate(8, sizeof(Echo_typedef));
    if (!sensor->echo_queue) return ESP_ERR_NO_MEM;

    gpio_config_t trig = {
        .pin_bit_mask = 1ULL << trig_pin,
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config_t echo = {
        .pin_bit_mask = 1ULL << echo_pin,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    esp_err_t ret = gpio_config(&trig);
    if (ret != ESP_OK) goto fail;
    ret = gpio_set_level(trig_pin, 0);
    if (ret != ESP_OK) goto fail;
    ret = gpio_config(&echo);
    if (ret != ESP_OK) goto fail;
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) goto fail;
    ret = gpio_isr_handler_add(echo_pin, echo_isr, sensor);
    if (ret != ESP_OK) goto fail;
    return ESP_OK;

fail:
    vQueueDelete(sensor->echo_queue);
    sensor->echo_queue = NULL;
    return ret;
}

esp_err_t hcsr05_read(HCSR05_typedef *sensor, float *distance_cm)
{
    if (!sensor || !distance_cm || !sensor->echo_queue)
        return ESP_ERR_INVALID_ARG;
    *distance_cm = -1.0f;

    // Bao dam 2 lan phat cach nhau it nhat 60 ms.
    int64_t remaining = TRIGGER_INTERVAL_US -
                        (esp_timer_get_time() - sensor->last_trigger_us);
    if (remaining > 0)
        vTaskDelay(pdMS_TO_TICKS((remaining + 999) / 1000) + 1);

    Echo_typedef event;
    while (xQueueReceive(sensor->echo_queue, &event, 0) == pdTRUE) {}
    if (gpio_get_level(sensor->echo_pin)) return ESP_ERR_INVALID_STATE;

    sensor->last_trigger_us = esp_timer_get_time();
    gpio_set_level(sensor->trig_pin, 1);
    esp_rom_delay_us(10);
    gpio_set_level(sensor->trig_pin, 0);

    int64_t rise_us = -1;
    int64_t deadline = sensor->last_trigger_us + ECHO_TIMEOUT_US;
    while ((remaining = deadline - esp_timer_get_time()) > 0)
    {
        TickType_t wait = pdMS_TO_TICKS((remaining + 999) / 1000) + 1;
        if (xQueueReceive(sensor->echo_queue, &event, wait) != pdTRUE)
            break;
        if (event.time_us < sensor->last_trigger_us) continue;
        if (event.time_us > deadline) break;
        if (event.level)
        {
            if (rise_us >= 0) return ESP_ERR_INVALID_RESPONSE;
            rise_us = event.time_us;
        }
        else if (rise_us >= 0)
        {
            int64_t width_us = event.time_us - rise_us;
            // Ngoai dai do hoac xung nhieu: coi la loi, khong coi la duong trong.
            if (width_us < 116 || width_us > 23200)
                return ESP_ERR_INVALID_RESPONSE;
            *distance_cm = width_us / 58.0f;
            return ESP_OK;
        }
    }
    return ESP_ERR_TIMEOUT;
}
