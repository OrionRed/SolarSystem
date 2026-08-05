#pragma once

#include <stddef.h>
#include <stdint.h>

void hal_uart_init(void);

int hal_uart_write(const uint8_t *data, size_t length);

int hal_uart_read(uint8_t *buffer, size_t max_length, uint32_t timeout_ms);