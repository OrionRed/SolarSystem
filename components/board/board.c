#include "board.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define BOARD_LED_GPIO    GPIO_NUM_4

static const char *TAG = "BOARD";

static void blink_task(void *pvParameters);
void board_init(void);

void board_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BOARD_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_set_level(BOARD_LED_GPIO, 0);

    ESP_LOGI(TAG, "Board initialized");

    xTaskCreate(
        blink_task,        // Function to run
        "Blink",           // Task name (for debugging)
        2048,              // Stack size (bytes)
        NULL,              // Parameter passed to the task
        1,                 // Priority
        NULL               // Don't keep the task handle
    );
}

static void blink_task(void *pvParameters)
{
    while (1)
    {
        gpio_set_level(BOARD_LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(500));

        gpio_set_level(BOARD_LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
