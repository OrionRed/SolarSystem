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

void modbus_self_test(void)
{
    uint8_t frame[8];

    size_t length =
    modbus_build_read_request(
        1,
        0,
        6,
        frame);

    ESP_LOGI(TAG, "Sending Modbus frame:");

    dump_hex(frame, length);    

    hal_uart_write(frame, length);

    ESP_LOGI(TAG, "Frame sent");

    uint8_t response[32];

    int response_length =
        hal_uart_read(response, sizeof(response), 1000);

    ESP_LOGI(TAG, "Received %d bytes:", response_length);

    dump_hex(response, response_length);

    uint16_t voltage_raw = ((uint16_t)response[3] << 8) | response[4];
    uint16_t current_raw = ((uint16_t)response[5] << 8) | response [6];

    float voltage = voltage_raw / 100.0f;
    float current = current_raw / 100.0f;

    uint16_t power_low  = ((uint16_t)response[7] << 8) | response[8];
    uint16_t power_high = ((uint16_t)response[9] << 8) | response[10];

    uint32_t power_raw = ((uint32_t)power_high << 16) | power_low;
    float power = power_raw / 10.0f;

    uint16_t energy_low  = ((uint16_t)response[11] << 8) | response[12];
    uint16_t energy_high = ((uint16_t)response[13] << 8) | response[14];

    uint32_t energy_raw = ((uint32_t)energy_high << 16) | energy_low;
    float energy = (float)energy_raw;

    ESP_LOGI(TAG, "Voltage: %.2f V, Current: %.2f A, Power: %.1f W, Energy: %.0f Wh",
             voltage, current, power, energy);
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