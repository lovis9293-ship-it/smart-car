#ifndef BLUETOOTH_H
#define BLUETOOTH_H
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"

void bluetooth_init(void);
esp_err_t bluetooth_deinit(void);
#endif