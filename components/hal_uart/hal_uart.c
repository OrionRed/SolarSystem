#include "hal_uart.h"
#include "config.h"

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "UART";

void hal_uart_init(void);
static void uart_configure(void);
static void uart_self_test(void);

void hal_uart_init(void)
{
    uart_configure();
    ESP_LOGI(TAG, "UART initialized");
    // You need to cross wire rx/tx to make a loopback test. This is just for testing the UART driver.
    //uart_self_test();  
}

int hal_uart_write(const uint8_t *data, size_t length)
{
    return uart_write_bytes(
        CONFIG_UART_PORT,
        data,
        length);
}

int hal_uart_read(uint8_t *buffer, size_t buffer_size, uint32_t timeout_ms)
{
    return uart_read_bytes(
        UART_NUM_1,
        buffer,
        buffer_size,
        pdMS_TO_TICKS(timeout_ms));
}

static void uart_configure(void)
{
    uart_config_t uart_config = {
        .baud_rate = CONFIG_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(
        uart_driver_install(
            CONFIG_UART_PORT,
            CONFIG_UART_BUFFER_SIZE,
            CONFIG_UART_BUFFER_SIZE,
            0,
            NULL,
            0));

    ESP_ERROR_CHECK(
        uart_param_config(
            CONFIG_UART_PORT,
            &uart_config));

    ESP_ERROR_CHECK(
        uart_set_pin(
            CONFIG_UART_PORT,
            CONFIG_UART_TX_GPIO,
            CONFIG_UART_RX_GPIO,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE));
}

static void uart_self_test(void)
{
    const char test[] = "Hello UART";

    uint8_t buffer[33];

    int written = hal_uart_write((const uint8_t *)test, sizeof(test));

    int received = hal_uart_read(buffer, sizeof(buffer), 100);

    ESP_LOGI(TAG, "Wrote %d bytes", written);
    ESP_LOGI(TAG, "Read %d bytes", received);

    if (received > 0)
    {
        buffer[received] = '\0';

        ESP_LOGI(TAG, "Received: %s", buffer);
    }
}