#include "inverter.h"
#include "board.h "

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

static const char *TAG = "Inverter";

static bool inverter_enabled;

void inverter_init(void)
{
    inverter_enabled = false;

    ESP_LOGI(TAG, "Inverter initialized");
}

bool inverter_is_on(void)
{
    return board_inverter_on_off_led_is_on();
}

bool inverter_turn_on(void)
{
    if (inverter_is_on())
    {
        return true;
    }

    board_inverter_switch_on();

    const int timeout_ms = 5000;
    const int check_interval_ms = 100;

    for (int elapsed_ms = 0; elapsed_ms < timeout_ms; elapsed_ms += check_interval_ms)
    {
        if (inverter_is_on())
        {
            return true;
        }

        vTaskDelay(pdMS_TO_TICKS(check_interval_ms));
    }

    return false;
}

bool inverter_turn_off(void)
{
    if (!inverter_is_on())
    {
        return true;
    }
    
    board_inverter_switch_off();

    const int timeout_ms = 5000;
    const int check_interval_ms = 100;

    for (int elapsed_ms = 0; elapsed_ms < timeout_ms; elapsed_ms += check_interval_ms)
    {
        if (!inverter_is_on())
        {
            return true;
        }

        vTaskDelay(pdMS_TO_TICKS(check_interval_ms));
    }

    return false;
}