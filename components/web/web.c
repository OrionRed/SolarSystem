#include "web.h"

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "Web";

static esp_err_t index_get_handler(httpd_req_t *req)
{
    int64_t uptime_us = esp_timer_get_time();
    int64_t uptime_s = uptime_us / 1000000;

    int hours = (int)(uptime_s / 3600);
    int minutes = (int)((uptime_s % 3600) / 60);
    int seconds = (int)(uptime_s % 60);

    const char *page =
        "<!DOCTYPE html>"
        "<html><head>"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<meta http-equiv=\"refresh\" content=\"5\">"
        "<title>SolarSystem</title>"
        "</head><body>"
        "<h1>SolarSystem</h1>"
        "<p>Web server is running.</p>"
        "<p>Uptime: %02d:%02d:%02d</p>"
        "</body></html>";

    char response[512];
    snprintf(response, sizeof(response), page, hours, minutes, seconds);

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
}

void web_init(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) != ESP_OK) {
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
