#include "app.h"
#include "hal_uart.h"
#include "app_config.h"
#include "system_info.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board.h"
#include "demo.h"
#include "modbus.h"
static const char *TAG = "SolarSystem";

void app_init(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, " %s", APP_NAME);
    ESP_LOGI(TAG, " Version %s", APP_VERSION);
    ESP_LOGI(TAG, " ESP32-S3 online");
    ESP_LOGI(TAG, "=================================");

    board_init();
    
    hal_uart_init();
    
    modbus_init();

    modbus_self_test();

    system_info_print();

    demo_start();
}

void app_run(void)
{
    ESP_LOGI(TAG, "Heartbeat");
    vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_MS));
}
