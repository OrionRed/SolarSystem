#include "inverter.h"

#include "esp_log.h"

static const char *TAG = "Inverter";

static bool inverter_enabled;

void inverter_init(void)
{
    inverter_enabled = false;

    ESP_LOGI(TAG, "Inverter initialized");
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
