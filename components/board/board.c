#include "board.h"
#include "config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdbool.h>


static const char *TAG = "BOARD";

void board_init(void);
void board_led_set(bool on);

void board_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << CONFIG_BOARD_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_config_t button_charge_request_conf = {
        .pin_bit_mask = 1ULL << CONFIG_BOARD_BUTTON_CHARGE_REQUEST_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&button_charge_request_conf));

    gpio_config_t az1_button_conf = {
        .pin_bit_mask = 1ULL << CONFIG_BOARD_AZ1_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&az1_button_conf));

    gpio_config_t inverter_on_off_led_conf = {
        .pin_bit_mask = 1ULL << CONFIG_BOARD_INVERTER_ON_OFF_LED_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&inverter_on_off_led_conf));

    gpio_set_level(CONFIG_BOARD_LED_GPIO, 0);

    gpio_config_t inverter_switch_conf = {
        .pin_bit_mask = 1ULL << CONFIG_BOARD_INVERTER_SWITCH_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&inverter_switch_conf));

    gpio_set_level(CONFIG_BOARD_INVERTER_SWITCH_GPIO, 1);

    ESP_LOGI(TAG, "Board initialized");

}

void board_led_set(bool on)
{
    gpio_set_level(CONFIG_BOARD_LED_GPIO, on);
}

bool board_button_charge_request_pressed(void)
{
    return gpio_get_level(CONFIG_BOARD_BUTTON_CHARGE_REQUEST_GPIO) == 0;
}

bool board_inverter_on_off_led_is_on(void)
{
    return gpio_get_level(CONFIG_BOARD_INVERTER_ON_OFF_LED_GPIO) == 0;
}

void board_inverter_switch_on(void)
{
    gpio_set_level(CONFIG_BOARD_INVERTER_SWITCH_GPIO, 0);
}

void board_inverter_switch_off(void)
{
    gpio_set_level(CONFIG_BOARD_INVERTER_SWITCH_GPIO, 1);
}  

bool board_az1_button_pressed(void)
{
    return gpio_get_level(CONFIG_BOARD_AZ1_BUTTON_GPIO) == 0;
}