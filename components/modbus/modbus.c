#include "modbus.h"

#include "hal_uart.h"

#include "esp_log.h"

#include <stdio.h>

static const char *TAG = "Modbus";
static void dump_hex(const uint8_t *data, size_t length);

void modbus_init(void)
{
    ESP_LOGI(TAG, "Modbus initialized");
}

size_t modbus_build_read_request(
    uint8_t address,
    uint16_t start_register,
    uint16_t register_count,
    uint8_t *frame)
{
    frame[0] = address;

    frame[1] = 0x04;

    frame[2] = start_register >> 8;
    frame[3] = start_register & 0xFF;

    frame[4] = register_count >> 8;
    frame[5] = register_count & 0xFF;

    uint16_t crc = modbus_crc(frame, 6);

    frame[6] = crc & 0xFF;
    frame[7] = crc >> 8;

    return 8;
}

static bool modbus_validate_response(
    const uint8_t *response,
    size_t length)
{
    if (length < 5)
        return false;

    uint16_t received_crc =
        ((uint16_t)response[length - 1] << 8) |
        response[length - 2];

    uint16_t calculated_crc =
        modbus_crc(response, length - 2);

    if (received_crc != calculated_crc)
    {
        ESP_LOGE(TAG,
                 "CRC error: received %04X, calculated %04X",
                 received_crc,
                 calculated_crc);
        return false;
    }

    return true;
}

int modbus_read_registers(
    uint8_t address,
    uint16_t start_register,
    uint16_t register_count,
    uint8_t *response,
    size_t response_size)
{
    uint8_t frame[8];

    size_t length =
        modbus_build_read_request(
            address,
            start_register,
            register_count,
            frame);

    ESP_LOGI(TAG, "Sending Modbus frame:");
    dump_hex(frame, length);

    hal_uart_write(frame, length);

    ESP_LOGI(TAG, "Frame sent");

    int response_length =
        hal_uart_read(response, response_size, 1000);

    ESP_LOGI(TAG, "Received %d bytes:", response_length);

    if (response_length <= 0)
    {
        ESP_LOGE(TAG, "No Modbus response");
        return response_length;
    }

    if (!modbus_validate_response(response, response_length))
    {
        ESP_LOGE(TAG, "Invalid Modbus response");
        return -1;
    }
    
    dump_hex(response, response_length);

    return response_length;
}

void modbus_self_test(void)
{
    uint8_t response[32];

    int response_length =
        modbus_read_registers(
            1,
            0,
            6,
            response,
            sizeof(response));

    if (response_length < 0)
    {
        ESP_LOGE(TAG, "Modbus self-test failed");
        return;
    }

    ESP_LOGI(TAG,
             "Modbus self-test received %d bytes",
             response_length);
}

uint16_t modbus_crc(
    const uint8_t *data,
    size_t length)
{    
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++)
        {
            bool lsb = crc & 1;

            crc >>= 1;

            if (lsb)
            {
                crc ^= 0xA001;
            }
        }
    }

    return crc;
}

static void dump_hex(const uint8_t *data, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        printf("%02X ", data[i]);
    }

    printf("\n");
}