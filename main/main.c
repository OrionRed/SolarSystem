#include "app.h"
#include "wifi.h"
#include "web.h"

#include "esp_err.h"
#include "nvs_flash.h"

static void nvs_init(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    else
    {
        ESP_ERROR_CHECK(ret);
    }
}

void app_main(void)
{
    nvs_init();
    wifi_init();
    web_init();
    app_init();

    while (1)
    {
        app_run();
    }
}
