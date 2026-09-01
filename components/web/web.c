#include "web.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "pzem.h"
#include "inverter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "Web";

static int64_t baseline_start_us = 0;
static int64_t baseline_idle_us = 0;
static double baseline_energy_wh = 0.0;
static float baseline_min_w = 0.0f;
static float baseline_max_w = 0.0f;
static uint32_t baseline_samples = 0;

/* Keep the HTML response out of the HTTP server task's stack. */
static char response[3072];

static void baseline_task(void *arg)
{
    int64_t previous_us = 0;
    float previous_power_w = 0.0f;
    bool have_previous = false;

    baseline_start_us = esp_timer_get_time();

    while (1)
    {
        pzem_data_t pzem = {0};

        if (!inverter_is_on() && pzem_get_data(&pzem))
        {
            int64_t now_us = esp_timer_get_time();

            if (!have_previous)
            {
                previous_us = now_us;
                previous_power_w = pzem.power;
                baseline_min_w = pzem.power;
                baseline_max_w = pzem.power;
                have_previous = true;
            }
            else
            {
                int64_t interval_us = now_us - previous_us;
                double interval_s = (double)interval_us / 1000000.0;

                /* Trapezoidal integration of the idle power. */
                baseline_energy_wh +=
                    ((double)previous_power_w + (double)pzem.power) *
                    0.5 * interval_s / 3600.0;

                baseline_idle_us += interval_us;
                previous_us = now_us;
                previous_power_w = pzem.power;
                baseline_samples++;

                if (pzem.power < baseline_min_w)
                {
                    baseline_min_w = pzem.power;
                }

                if (pzem.power > baseline_max_w)
                {
                    baseline_max_w = pzem.power;
                }
            }
        }
        else
        {
            /* A non-idle period is not part of the baseline. */
            have_previous = false;
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

static const char *state_name(void)
{
    return inverter_is_on() ? "CHARGING / INVERTER ON" : "IDLE / INVERTER OFF";
}

static esp_err_t index_get_handler(httpd_req_t *req)
{
    pzem_data_t pzem = {0};
    bool pzem_valid = pzem_get_data(&pzem);

    double average_w = baseline_idle_us > 0 ?
                       baseline_energy_wh /
                       ((double)baseline_idle_us / 3600000000.0) : 0.0;

    if (pzem_valid)
    {
        int64_t uptime_s = esp_timer_get_time() / 1000000;
        int hours = (int)(uptime_s / 3600);
        int minutes = (int)((uptime_s % 3600) / 60);
        int seconds = (int)(uptime_s % 60);

        snprintf(response, sizeof(response),
            "<!DOCTYPE html>"
            "<html><head>"
            "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
            "<meta http-equiv=\"refresh\" content=\"5\">"
            "<title>SolarSystem</title>"
            "</head><body>"
            "<h1>SolarSystem</h1>"
            "<h2>%s</h2>"
            "<h3>Battery</h3>"
            "<p>Voltage: %.2f V</p>"
            "<p>Current: %.2f A</p>"
            "<p>Power: %.1f W</p>"
            "<p>PZEM Energy: %lu Wh</p>"
            "<h3>Baseline (inverter OFF)</h3>"
            "<p>Elapsed idle time: %lld s</p>"
            "<p>Average power: %.3f W</p>"
            "<p>Energy: %.3f Wh</p>"
            "<p>Minimum: %.2f W</p>"
            "<p>Maximum: %.2f W</p>"
            "<p>Samples: %lu</p>"
            "<h3>System</h3>"
            "<p>Inverter: %s</p>"
            "<p>Uptime: %02d:%02d:%02d</p>"
            "</body></html>",
            state_name(),
            pzem.voltage,
            pzem.current,
            pzem.power,
            (unsigned long)pzem.energy,
            (long long)(baseline_idle_us / 1000000),
            average_w,
            baseline_energy_wh,
            baseline_min_w,
            baseline_max_w,
            (unsigned long)baseline_samples,
            inverter_is_on() ? "ON" : "OFF",
            hours, minutes, seconds);
    }
    else
    {
        snprintf(response, sizeof(response),
            "<!DOCTYPE html>"
            "<html><head>"
            "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
            "<meta http-equiv=\"refresh\" content=\"5\">"
            "<title>SolarSystem</title>"
            "</head><body>"
            "<h1>SolarSystem</h1>"
            "<h2>%s</h2>"
            "<p>PZEM: No valid reading</p>"
            "</body></html>",
            state_name());
    }

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
}

void web_init(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return;
    }

    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_get_handler,
        .user_ctx = NULL,
    };

    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &index_uri));

    xTaskCreate(
        baseline_task,
        "baseline_task",
        4096,
        NULL,
        4,
        NULL);

    ESP_LOGI(TAG, "Web server started on port %d", config.server_port);
}
