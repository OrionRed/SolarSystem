#include "system_info.h"

#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "System";

void system_info_print(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    ESP_LOGI(TAG, "========== System Information ==========");
    ESP_LOGI(TAG, "Chip: ESP32-S3");
    ESP_LOGI(TAG, "Revision: %d", chip_info.revision);
    ESP_LOGI(TAG, "CPU cores: %d", chip_info.cores);
    ESP_LOGI(TAG, "Flash size: %lu MB",
             (unsigned long)(flash_size / (1024 * 1024)));
    ESP_LOGI(TAG, "Free heap: %lu bytes",
             (unsigned long)esp_get_free_heap_size());
    ESP_LOGI(TAG, "========================================");
}