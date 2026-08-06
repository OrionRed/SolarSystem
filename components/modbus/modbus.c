#include "modbus.h"

#include "hal_uart.h"

#include "esp_log.h"

static const char *TAG = "Modbus";

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

    frame[1] = 0x03;

    frame[2] = start_register >> 8;
    frame[3] = start_register & 0xFF;

    frame[4] = register_count >> 8;
    frame[5] = register_count & 0xFF;

    return 6;
}

void modbus_self_test(void)
{
    uint8_t frame[8];

    size_t length =
    modbus_build_read_request(
        1,
        0,
        2,
        frame);

    hal_uart_write(frame, length);

    ESP_LOGI(TAG, "Frame sent");
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
            if (crc & 1)
            {
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}
