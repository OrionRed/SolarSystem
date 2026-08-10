#include "pzem.h"

#include "modbus.h"

bool pzem_read(pzem_data_t *data)
{
    uint8_t response[32];

    int response_length =
        modbus_read_registers(
            1,
            0,
            6,
            response,
            sizeof(response));

    if (response_length != 17)
    {
        return false;
    }

    uint16_t voltage_raw =
        ((uint16_t)response[3] << 8) | response[4];

    uint16_t current_raw =
        ((uint16_t)response[5] << 8) | response[6];

    uint16_t power_low =
        ((uint16_t)response[7] << 8) | response[8];

    uint16_t power_high =
        ((uint16_t)response[9] << 8) | response[10];

    uint16_t energy_low =
        ((uint16_t)response[11] << 8) | response[12];

    uint16_t energy_high =
        ((uint16_t)response[13] << 8) | response[14];

    uint32_t power_raw =
        ((uint32_t)power_high << 16) | power_low;

    uint32_t energy_raw =
        ((uint32_t)energy_high << 16) | energy_low;

    data->voltage = voltage_raw / 100.0f;
    data->current = current_raw / 100.0f;
    data->power   = power_raw / 10.0f;
    data->energy  = energy_raw;

    return true;
}