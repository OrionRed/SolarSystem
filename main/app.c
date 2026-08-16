#include <stdint.h>
#include <stdbool.h>

#include "app.h"
#include "hal_uart.h"
#include "app_config.h"
#include "system_info.h"
#include "pzem.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board.h"
#include "demo.h"
#include "modbus.h"
static const char *TAG = "SolarSystem";
static const char *app_state_name(app_state_id_t state);
static void app_process_state(void);

typedef struct
{
    app_state_id_t state;
    pzem_data_t pzem;
    bool charge_requested;
    uint32_t charge_ticks;
} app_state_t;

static app_state_t app;

void app_init(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, " %s", APP_NAME);
    ESP_LOGI(TAG, " Version %s", APP_VERSION);
    ESP_LOGI(TAG, " ESP32-S3 online");
    ESP_LOGI(TAG, "=================================");

    app.state = APP_STATE_IDLE;
    app.charge_requested = false;
    app.charge_ticks = 0;
    
    board_init();
 
    system_info_print();
        
    hal_uart_init();
    
    modbus_init();

    modbus_self_test();

    pzem_init();

    demo_start();
}

void app_run(void)
{
    pzem_data_t data;

    ESP_LOGI(TAG, "App State: %s", app_state_name(app.state));
    
    if (pzem_get_data(&app.pzem))
    {
        ESP_LOGI(TAG,
                 "PZEM: %.2f V, %.2f A, %.1f W, %lu Wh",
                 app.pzem.voltage,
                 app.pzem.current,
                 app.pzem.power,
                 app.pzem.energy);
    }
    else
    {
        ESP_LOGI(TAG, "PZEM: No valid reading yet");
    }

    app_process_state();

    ESP_LOGI(TAG, "Heartbeat");
    vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_MS));
    app.charge_requested = true;
}

static const char *app_state_name(app_state_id_t state)
{
    switch (state)
    {
        case APP_STATE_IDLE:
            return "IDLE";
        case APP_STATE_WAITING_FOR_BATTERY:
            return "WAITING_FOR_BATTERY";
        case APP_STATE_CHARGING:
            return "CHARGING";
        case APP_STATE_CHARGE_COMPLETE:
            return "CHARGE_COMPLETE";
        case APP_STATE_FAULT:
            return "FAULT";

        default:
            return "UNKNOWN";
    }
}

static void app_process_state(void)
{
    switch (app.state)
    {
        case APP_STATE_IDLE:
            if (app.charge_requested)
            {
                ESP_LOGI(TAG, "Charge requested");
                app.charge_requested = false;
                app.state = APP_STATE_CHARGING;
            }
            break;

        case APP_STATE_CHARGING:
            app.charge_ticks++;
                if (app.charge_ticks >= 5)
            {
                ESP_LOGI(TAG, "Test charge complete");

                app.charge_ticks = 0;
                app.state = APP_STATE_IDLE;
            }

            break;

        default:
            ESP_LOGE(TAG, "Unknown application state: %d", app.state);
            app.state = APP_STATE_IDLE;
            break;
    }
}