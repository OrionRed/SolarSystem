#include "web.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "pzem.h"
#include "inverter.h"

static const char *TAG = "Web";

static const char *state_name(void)
{
    return inverter_is_on() ? "CHARGING / INVERTER ON" : "IDLE / INVERTER OFF";
}

static esp_err_t index_get_handler(httpd_req_t *req)
{
    pzem_data_t pzem = {0};
    bool pzem_valid = pzem_get_data(&pzem);

    int64_t uptime_s = esp_timer_get_time() / 1000000;
    int hours = (int)(uptime_s / 3600);
    int minutes = (int)((uptime_s % 3600) / 60);
    int seconds = (int)(uptime_s % 60);

    char response[2048];

    if (pzem_valid)
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
            "<h3>Battery</h3>"
            "<p>Voltage: %.2f V</p>"
            "<p>Current: %.2f A</p>"
            "<p>Power: %.1f W</p>"
            "<p>PZEM Energy: %lu Wh</p>"
            "<h3>System</h3>"
            "<p>Inverter: %s</p>"
            "<p>Uptime: %02d:%02d:%02d</p>"
            "</body></html>",
            state_name(),
            pzem.voltage,
            pzem.current,
            pzem.power,
            (unsigned long)pzem.energy,
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
            "<p>Uptime: %02d:%02d:%02d</p>"
            "</body></html>",
            state_name(), hours, minutes, seconds);
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

    ESP_LOGI(TAG, "Web server started on port %d", config.server_port);
}
