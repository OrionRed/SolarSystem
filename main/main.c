#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "system_info.h"

static const char *TAG = "SolarSystem";

void app_main(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, " SolarSystem Controller v0.1");
    ESP_LOGI(TAG, " ESP32-S3 online");
    ESP_LOGI(TAG, " First successful application");
    ESP_LOGI(TAG, "=================================");
    
    system_info_print();
    
    while (1)
    {
        ESP_LOGI(TAG, "Heartbeat");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}