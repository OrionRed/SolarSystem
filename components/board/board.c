#include "board.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdbool.h>

#define BOARD_BUTTON_GPIO    GPIO_NUM_0
#define BOARD_LED_GPIO    GPIO_NUM_4

static const char *TAG = "BOARD";

void board_init(void);
void board_led_set(bool on);
bool board_button_pressed(void);

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

    gpio_config_t button_conf = {
        .pin_bit_mask = 1ULL << BOARD_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&button_conf));

    gpio_set_level(BOARD_LED_GPIO, 0);

    ESP_LOGI(TAG, "Board initialized");

}

void board_led_set(bool on)
{
    gpio_set_level(BOARD_LED_GPIO, on);
}

bool board_button_pressed(void)
{
    return gpio_get_level(BOARD_BUTTON_GPIO) == 0;
}
