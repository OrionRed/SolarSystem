#include <stdint.h>

#include "app.h"
#include "hal_uart.h"
#include "app_config.h"
#include "system_info.h"
#include "pzem.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board.h"
#include "demo.h"
#include "modbus.h"
static const char *TAG = "SolarSystem";

typedef struct {
    pzem_data_t pzem;
} app_state_t;

static app_state_t app;

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

    xTaskCreate(
    pzem_task,
    "pzem_task",
    4096,
    NULL,
    5,
    NULL);

    system_info_print();

    demo_start();
}

static void pzem_task(void *arg)
{
    while (1)
    {
        if (pzem_read(&app.pzem))
        {
            ESP_LOGI(TAG,
                     "Voltage: %.2f V, Current: %.2f A, Power: %.1f W, Energy: %lu Wh",
                     app.pzem.voltage,
                     app.pzem.current,
                     app.pzem.power,
                     app.pzem.energy);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_run(void)
{
    ESP_LOGI(TAG, "Heartbeat");
    vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_MS));
}