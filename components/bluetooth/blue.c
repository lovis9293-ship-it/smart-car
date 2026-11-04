#include "blue.h"
#include "car.h"
#include "esp_spp_api.h"
#include <stdatomic.h>
#include "freertos/task.h"
static const char *TAG = "BLUETOOTH";
extern Car_typedef car;
static atomic_bool spp_uninit_done = false;
static bool ble_memory_released = false;
//xu ly callback
static void bluetooth_callback(esp_spp_cb_event_t event,esp_spp_cb_param_t *param){
    switch (event)
    {
    case ESP_SPP_INIT_EVT:
        ESP_LOGI(TAG, "SPP INIT");

        esp_bt_gap_set_device_name("ESP32_BT");

        esp_bt_gap_set_scan_mode(
            ESP_BT_CONNECTABLE,
            ESP_BT_GENERAL_DISCOVERABLE
        );

        esp_spp_start_srv(
            ESP_SPP_SEC_NONE,
            ESP_SPP_ROLE_SLAVE,
            0,
            "SPP_SERVER"
        );
        break;

    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(TAG, "Client Connected!");
        break;

    case ESP_SPP_DATA_IND_EVT:
    {
        for (int i = 0; i < param->data_ind.len; i++)
        {
            char d = (char)param->data_ind.data[i];

            car_set_command(&car, d);
        }
        break;
    }
    case ESP_SPP_UNINIT_EVT:
        atomic_store(&spp_uninit_done, true);
         ESP_LOGI("BLUE", "SPP DA TAT");
        break;

    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(TAG, "Client Disconnected");
        car_set_command(&car, 'x');
        break;

    default:
        break;
    }

}

// Khoi tao Bluetooth
void bluetooth_init(void){
    ESP_ERROR_CHECK(nvs_flash_init());
    // Chi giai phong vung BLE mot lan, ke ca khi quet RFID nhieu lan.
    if (!ble_memory_released)
    {
        ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
        ble_memory_released = true;
    }

    esp_bt_controller_config_t bt_cfg =
        BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_bt_controller_init(&bt_cfg)
    );

    ESP_ERROR_CHECK(
        esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)
    );

    ESP_ERROR_CHECK(esp_bluedroid_init());

    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // Dang ky Callback
    ESP_ERROR_CHECK(
        esp_spp_register_callback(bluetooth_callback)
    );

    // Cau hinh SPP
    esp_spp_cfg_t spp_cfg = {
        .mode = ESP_SPP_MODE_CB,
        .enable_l2cap_ertm = false,
        .tx_buffer_size = 0,
    };

    ESP_ERROR_CHECK(
        esp_spp_enhanced_init(&spp_cfg)
    );

    ESP_LOGI(TAG, "System Ready!");
}
esp_err_t bluetooth_deinit(void)
{
    atomic_store(&spp_uninit_done, false);

    // Tat SPP
    esp_err_t ret = esp_spp_deinit();
    if (ret != ESP_OK) 
    return ret;
    // Cho SPP tat xong (toi da 2 giay)
    for (int i = 0; i < 200 && !atomic_load(&spp_uninit_done); i++)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (!atomic_load(&spp_uninit_done)) return ESP_ERR_TIMEOUT;

    // Tat Bluedroid
    ret = esp_bluedroid_disable();
    if (ret != ESP_OK) return ret;

    ret = esp_bluedroid_deinit();
    if (ret != ESP_OK) return ret;

    // Tat Bluetooth Controller
    ret = esp_bt_controller_disable();
    if (ret != ESP_OK) return ret;

    ret = esp_bt_controller_deinit();
    if (ret != ESP_OK) return ret;

    ESP_LOGI("BLUE", "BLUETOOTH DA TAT HOAN TOAN");
    return ESP_OK;
}