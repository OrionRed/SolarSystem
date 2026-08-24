#include <stdint.h>
#include <stdbool.h>

#include "app.h"
#include "hal_uart.h"
#include "app_config.h"
#include "system_info.h"
#include "pzem.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board.h"
#include "modbus.h"
#include "inverter.h"
#include "button_charge_request.h"
#include "charge_request_iface.h"

static const char *TAG = "SolarSystem";
static const char *app_state_name(app_state_id_t state);
static void app_process_state(void);

typedef struct
{
    app_state_id_t state;
    pzem_data_t pzem;
    bool charge_requested;
    uint32_t low_current_start_ms;
    bool pzem_valid;
    bool charge_active;
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
    app.charge_active = false;
    app.low_current_start_ms = 0;
        
    board_init();
 
    system_info_print();
        
    hal_uart_init();
    
    modbus_init();

    modbus_self_test();

    pzem_init();

    button_charge_request_init();

    inverter_init();

    ESP_LOGI(TAG, "Inverter initial state: %s",
         inverter_is_on() ? "ON" : "OFF");
}

void app_run(void)
{
    ESP_LOGI(TAG, "App State: %s", app_state_name(app.state));
    app.pzem_valid = pzem_get_data(&app.pzem);
    if (app.pzem_valid)
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
    bool inverter_led_on = inverter_is_on();
    ESP_LOGI(TAG, "Heartbeat - Inverter LED: %s", inverter_led_on ? "ON" : "OFF");
    vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_MS));
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
        {
            if (app.charge_requested)
            {
                ESP_LOGI(TAG, "Charge requested");

                app.charge_requested = false;
                app.charge_active = false;
                app.low_current_start_ms = 0;

                if (inverter_turn_on())
                {
                    app.state = APP_STATE_CHARGING;
                }
                else
                {
                    ESP_LOGE(TAG, "Failed to turn inverter on");
                    app.state = APP_STATE_FAULT;
                }   
            }
            break;
        } //case APP_STATE_IDLE

        case APP_STATE_CHARGING:
        {
            if (!app.pzem_valid)
            {
                break;
            }

            if (!app.charge_active)
            {
                if (app.pzem.current >= CHARGE_START_CURRENT_A)
                {
                    app.charge_active = true;

                    ESP_LOGI(TAG,
                            "Charging started: %.2f A",
                            app.pzem.current);
                }
                break;
            }

            /*
            * Charger is known to be active.
            * Now watch for sustained low current.
            */
            if (app.pzem.current < CHARGE_COMPLETE_CURRENT_A)
            {
                uint32_t now_ms = esp_timer_get_time() / 1000;

                if (app.low_current_start_ms == 0)
                {
                    app.low_current_start_ms = now_ms;

                    ESP_LOGI(TAG,
                            "Charging current below %.2f A",
                            CHARGE_COMPLETE_CURRENT_A);
                }
                else if ((now_ms - app.low_current_start_ms) >=
                        CHARGE_COMPLETE_TIME_MS)
                {
                    ESP_LOGI(TAG, "Charge complete");
                    app.low_current_start_ms = 0;
                    app.charge_active = false;

                    if (!inverter_turn_off())
                    {
                        ESP_LOGE(TAG, "Failed to turn inverter off");
                        app.state = APP_STATE_FAULT;
                    }
                    else
                    {
                        app.state = APP_STATE_IDLE;
                    }
                }
            }
            else
            {
                if (app.low_current_start_ms != 0)
                {
                    ESP_LOGI(TAG, "Charging current recovered");
                    app.low_current_start_ms = 0;
                }
            }
            break;
        } //case APP_STATE_CHARGING

        case APP_STATE_FAULT:
        {
            ESP_LOGE(TAG, "Application is in FAULT state");
            break;
        } //case APP_STATE_FAULT

        default:
            ESP_LOGE(TAG, "Unknown application state: %d", app.state);
            app.state = APP_STATE_IDLE;
            break;

    } //switch (app.state)
}

void app_request_charge(void)
{
    if (app.state == APP_STATE_IDLE)
    {
        ESP_LOGI(TAG, "Charge request received");
        app.charge_requested = true;
    }
    else
    {
        ESP_LOGI(TAG,
                 "Charge request ignored; app state is %s",
                 app_state_name(app.state));
    }
}