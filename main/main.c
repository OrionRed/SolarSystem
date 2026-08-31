#include "app.h"
#include "wifi.h"

void app_main(void)
{
    wifi_init();
    app_init();

    while (1)
    {
        app_run();
    }
}
