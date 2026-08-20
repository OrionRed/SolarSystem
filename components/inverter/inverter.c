#include "inverter.h"
#include "board.h "

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

bool inverter_set_enabled(bool enabled)
{
    if (enabled == inverter_enabled)
    {
        return true;
    }

    inverter_enabled = enabled;

    ESP_LOGI(TAG,
             "Inverter %s",
             enabled ? "ON" : "OFF");

    return true;
}
