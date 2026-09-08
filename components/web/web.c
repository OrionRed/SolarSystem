#include "web.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "pzem.h"
#include "inverter.h"
#include "energy.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

static const char *TAG = "Web";

static int64_t baseline_start_us = 0;
static int64_t baseline_idle_us = 0;
static double baseline_energy_wh = 0.0;
static float baseline_min_w = 0.0f;
static float baseline_max_w = 0.0f;
static uint32_t baseline_samples = 0;

static const char *app_stage = "IDLE";

/* Keep the HTML response out of the HTTP server task's stack. */
static char response[4096];

#define LOG_LINE_COUNT 64
#define LOG_LINE_LENGTH 256

static char log_lines[LOG_LINE_COUNT][LOG_LINE_LENGTH];
static uint32_t log_next = 0;
static uint32_t log_count = 0;
static portMUX_TYPE log_lock = portMUX_INITIALIZER_UNLOCKED;
static vprintf_like_t original_vprintf = NULL;
static char log_response[LOG_LINE_COUNT * LOG_LINE_LENGTH + 1];

static int web_log_vprintf(const char *format, va_list args)
{
    char line[LOG_LINE_LENGTH];
    va_list copy;

    va_copy(copy, args);
    int length = vsnprintf(line, sizeof(line), format, copy);
    va_end(copy);

    if (length > 0)
    {
        line[sizeof(line) - 1] = '\0';

        portENTER_CRITICAL(&log_lock);

        strncpy(log_lines[log_next], line, LOG_LINE_LENGTH - 1);
        log_lines[log_next][LOG_LINE_LENGTH - 1] = '\0';

        log_next = (log_next + 1) % LOG_LINE_COUNT;
        if (log_count < LOG_LINE_COUNT)
        {
            log_count++;
        }

        portEXIT_CRITICAL(&log_lock);
    }

    /* Preserve the normal USB/UART log output. */
    if (original_vprintf)
    {
        return original_vprintf(format, args);
    }

    return 0;
}

static size_t copy_log_snapshot(void)
{
    size_t used = 0;

    portENTER_CRITICAL(&log_lock);

    uint32_t first = (log_next + LOG_LINE_COUNT - log_count) % LOG_LINE_COUNT;

    for (uint32_t i = 0; i < log_count; i++)
    {
        uint32_t index = (first + i) % LOG_LINE_COUNT;
        size_t remaining = sizeof(log_response) - used;

        if (remaining <= 1)
        {
            break;
        }

        int written = snprintf(log_response + used, remaining,
                               "%s", log_lines[index]);
        if (written < 0)
        {
            break;
        }

        if ((size_t)written >= remaining)
        {
            used = sizeof(log_response) - 1;
            break;
        }

        used += (size_t)written;
    }

    log_response[used] = '\0';

    portEXIT_CRITICAL(&log_lock);

    return used;
}

static esp_err_t logs_get_handler(httpd_req_t *req)
{
    size_t length = copy_log_snapshot();

    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    /* The log view refreshes independently of the main page. */
    const char *header =
        "<!DOCTYPE html>"
        "<html><head>"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<meta http-equiv=\"refresh\" content=\"2\">"
        "<style>"
        "body{margin:0;background:#111;color:#ddd;font-family:monospace;}"
        "pre{margin:0;padding:10px;white-space:pre-wrap;word-break:break-word;}"
        "</style>"
        "</head><body>"
        "<pre id=\"log\">";

    const char *footer =
        "</pre>"
        "<script>"
        "window.scrollTo(0,document.body.scrollHeight);"
        "</script>"
        "</body></html>";

    esp_err_t err = httpd_resp_send_chunk(req, header, HTTPD_RESP_USE_STRLEN);
    if (err != ESP_OK)
    {
        return err;
    }

    err = httpd_resp_send_chunk(req, log_response, length);
    if (err != ESP_OK)
    {
        return err;
    }

    err = httpd_resp_send_chunk(req, footer, HTTPD_RESP_USE_STRLEN);
    if (err != ESP_OK)
    {
        return err;
    }

    /* End the chunked HTTP response. Without this, the browser waits forever
       for the response to finish and the parent page remains "loading". */
    return httpd_resp_send_chunk(req, NULL, 0);
}

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

static const char *inverter_state_name(void)
{
    return inverter_is_on() ? "INVERTER ON" : "INVERTER OFF";
}

static const char *flow_name(energy_flow_t flow)
{
    return flow == ENERGY_FLOW_CHARGE ? "PV CHARGING" : "BATTERY DISCHARGING";
}

static void format_duration(uint64_t total_seconds, char *buffer, size_t buffer_size)
{
    uint64_t days = total_seconds / 86400;
    uint64_t hours = (total_seconds % 86400) / 3600;
    uint64_t minutes = (total_seconds % 3600) / 60;
    uint64_t seconds = total_seconds % 60;

    if (days > 0)
    {
        snprintf(buffer, buffer_size, "%llud %02llu:%02llu:%02llu",
                 (unsigned long long)days,
                 (unsigned long long)hours,
                 (unsigned long long)minutes,
                 (unsigned long long)seconds);
    }
    else
    {
        snprintf(buffer, buffer_size, "%02llu:%02llu:%02llu",
                 (unsigned long long)hours,
                 (unsigned long long)minutes,
                 (unsigned long long)seconds);
    }
}

static esp_err_t calibrate_full_handler(httpd_req_t *req)
{
    if (inverter_is_on())
    {
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_set_type(req, "text/plain");
        return httpd_resp_sendstr(req,
                                  "Turn the inverter OFF before calibrating FULL.\n");
    }

    energy_calibrate_full();

    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t index_get_handler(httpd_req_t *req)
{
    pzem_data_t pzem = {0};
    energy_stats_t energy_stats = {0};
    bool pzem_valid = pzem_get_data(&pzem);

    energy_get_stats(&energy_stats);

    double average_w = baseline_idle_us > 0 ?
                       baseline_energy_wh /
                       ((double)baseline_idle_us / 3600000000.0) : 0.0;

    char charge_time[32];
    char discharge_time[32];
    char battery_energy_text[32];
    format_duration(energy_stats.charge_time_s, charge_time, sizeof(charge_time));
    format_duration(energy_stats.discharge_time_s, discharge_time, sizeof(discharge_time));

    if (energy_stats.battery_calibrated)
    {
        snprintf(battery_energy_text, sizeof(battery_energy_text),
                 "%.1f Wh", energy_stats.battery_energy_wh);
    }
    else
    {
        snprintf(battery_energy_text, sizeof(battery_energy_text),
                 "NOT CALIBRATED");
    }

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
            "<h3>Energy Accounting</h3>"
            "<p>Current flow: <strong>%s</strong></p>"
            "<p>PV charge: +%.3f Wh</p>"
            "<p>Battery discharge: -%.3f Wh</p>"
            "<p>Net battery change: %.3f Wh</p>"
            "<p>Charge time: %s</p>"
            "<p>Discharge time: %s</p>"
            "<p>Accounting samples: %lu</p>"
            "<p>Battery energy: %s</p>"
            "<p>Battery capacity: %.0f Wh</p>"
            "<p><a href=\"/calibrate-full\">Calibrate battery FULL</a></p>"
            "<h3>Baseline (inverter OFF)</h3>"
            "<p>Elapsed idle time: %lld s</p>"
            "<p>Average power: %.3f W</p>"
            "<p>Energy: %.3f Wh</p>"
            "<p>Minimum: %.2f W</p>"
            "<p>Maximum: %.2f W</p>"
            "<p>Samples: %lu</p>"
            "<h3>System</h3>"
            "<p>Stage: <strong>%s</strong></p>"
            "<p>Inverter: %s</p>"
            "<p>Uptime: %02d:%02d:%02d</p>"
            "<h3>Live Log</h3>"
            "<p><a href=\"/logs\" target=\"_blank\">Open live log in new window</a></p>"
            "<iframe src=\"/logs\" style=\"width:100%%;height:400px;border:1px solid #888;\"></iframe>"
            "</body></html>",
            inverter_state_name(),
            pzem.voltage,
            pzem.current,
            pzem.power,
            (unsigned long)pzem.energy,
            flow_name(energy_stats.current_flow),
            energy_stats.charge_wh,
            energy_stats.discharge_wh,
            energy_stats.net_change_wh,
            charge_time,
            discharge_time,
            (unsigned long)energy_stats.samples,
            battery_energy_text,
            energy_stats.battery_capacity_wh,
            (long long)(baseline_idle_us / 1000000),
            average_w,
            baseline_energy_wh,
            baseline_min_w,
            baseline_max_w,
            (unsigned long)baseline_samples,
            app_stage,
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
            "<h3>Energy Accounting</h3>"
            "<p>Current flow: <strong>%s</strong></p>"
            "<p>PV charge: +%.3f Wh</p>"
            "<p>Battery discharge: -%.3f Wh</p>"
            "<p>Net battery change: %.3f Wh</p>"
            "<p>Battery energy: %s</p>"
            "<h3>System</h3>"
            "<p>Stage: <strong>%s</strong></p>"
            "<p>Inverter: %s</p>"
            "<h3>Live Log</h3>"
            "<p><a href=\"/logs\" target=\"_blank\">Open live log in new window</a></p>"
            "<iframe src=\"/logs\" style=\"width:100%%;height:400px;border:1px solid #888;\"></iframe>"
            "</body></html>",
            inverter_state_name(),
            flow_name(energy_stats.current_flow),
            energy_stats.charge_wh,
            energy_stats.discharge_wh,
            energy_stats.net_change_wh,
            battery_energy_text,
            app_stage,
            inverter_is_on() ? "ON" : "OFF");
    }

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
}

void web_set_stage(const char *stage)
{
    app_stage = stage ? stage : "UNKNOWN";
}

void web_init(void)
{
    /* Capture the same formatted log stream that normally goes to USB/UART. */
    original_vprintf = esp_log_set_vprintf(web_log_vprintf);

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

    httpd_uri_t calibrate_full_uri = {
        .uri = "/calibrate-full",
        .method = HTTP_GET,
        .handler = calibrate_full_handler,
        .user_ctx = NULL,
    };

    httpd_uri_t logs_uri = {
        .uri = "/logs",
        .method = HTTP_GET,
        .handler = logs_get_handler,
        .user_ctx = NULL,
    };

    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &index_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &calibrate_full_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &logs_uri));

    xTaskCreate(
        baseline_task,
        "baseline_task",
        4096,
        NULL,
        4,
        NULL);

    ESP_LOGI(TAG, "Web server started on port %d", config.server_port);
}
