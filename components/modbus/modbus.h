#pragma once

#include <stddef.h>
#include <stdint.h>

void modbus_init(void);

size_t modbus_build_read_request(
    uint8_t address,
    uint16_t start_register,
    uint16_t register_count,
    uint8_t *frame);

void modbus_self_test(void);

uint16_t modbus_crc(
    const uint8_t *data,
    size_t length);

int modbus_read_registers(
    uint8_t address,
    uint16_t start_register,
    uint16_t register_count,
    uint8_t *response,
    size_t response_size);
        