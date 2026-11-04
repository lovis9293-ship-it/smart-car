#include "rc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
extern QueueHandle_t rfid_queue;

static const char *TAG = "rc522-basic-example";
static const uint8_t valid_uid[4] = {
    0x59,0xF8,0x8F,0x29
};
bool check_uid(rc522_picc_t *picc){
    if (picc->uid.length != 4)
        return false;
    
    for (int i = 0; i < sizeof(valid_uid); i++)
    {
        if(picc->uid.value[i] != valid_uid[i]){
            return false;
        }    
    }
    return true;
}
static void on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;

    if (picc->state == RC522_PICC_STATE_ACTIVE) {

        if (check_uid(picc))
        {
            ESP_LOGI("RFID", "UID dung!");
            uint8_t signal = 1;
            // Sau này gửi tín hiệu cho SystemTask
            if (xQueueSend(rfid_queue, &signal, 0) != pdTRUE)
                ESP_LOGW(TAG, "RFID queue day, hay quet lai");
            // để bật hoặc tắt Bluetooth
            
        }
        else
        {
            ESP_LOGI("RFID", "UID sai!");
            return;
        }
    }
    else if (picc->state == RC522_PICC_STATE_IDLE && event->old_state >= RC522_PICC_STATE_ACTIVE) {
        ESP_LOGI(TAG, "Card has been removed");
    }
}


void rc522_init(RC522_Typedef *rc,int RC522_SPI_BUS_GPIO_MISO,int RC522_SPI_BUS_GPIO_MOSI,int RC522_SPI_BUS_GPIO_SCLK,int RC522_SPI_SCANNER_GPIO_SDA,int RC522_SCANNER_GPIO_RST){
   
   
    rc->RC522_SPI_BUS_GPIO_MISO=RC522_SPI_BUS_GPIO_MISO;
    rc->RC522_SPI_BUS_GPIO_MOSI=RC522_SPI_BUS_GPIO_MOSI;
    rc->RC522_SPI_BUS_GPIO_SCLK=RC522_SPI_BUS_GPIO_SCLK;
    rc->RC522_SPI_SCANNER_GPIO_SDA=RC522_SPI_SCANNER_GPIO_SDA;
    rc->RC522_SCANNER_GPIO_RST=RC522_SCANNER_GPIO_RST;


    static spi_bus_config_t bus_config;

bus_config = (spi_bus_config_t){
    .miso_io_num = rc->RC522_SPI_BUS_GPIO_MISO,
    .mosi_io_num = rc->RC522_SPI_BUS_GPIO_MOSI,
    .sclk_io_num = rc->RC522_SPI_BUS_GPIO_SCLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
};

rc522_spi_config_t driver_config = {
    .host_id = SPI3_HOST,
    .bus_config = &bus_config,
    .dev_config = {
        .spics_io_num = rc->RC522_SPI_SCANNER_GPIO_SDA,
    },
    .rst_io_num = rc->RC522_SCANNER_GPIO_RST,
};

    ESP_ERROR_CHECK(rc522_spi_create(&driver_config, &rc->driver));
    
    ESP_ERROR_CHECK(rc522_driver_install(rc->driver));

    rc522_config_t scanner_config = {
        .driver = rc->driver,
    };

    ESP_ERROR_CHECK(rc522_create(&scanner_config, &rc->scanner));
    ESP_ERROR_CHECK(rc522_register_events(rc->scanner, RC522_EVENT_PICC_STATE_CHANGED, on_picc_state_changed, NULL));
    ESP_ERROR_CHECK(rc522_start(rc->scanner));
}

