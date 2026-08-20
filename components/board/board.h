#pragma once

#include <stdbool.h>

void board_init(void);
void board_led_set(bool on);
bool board_button_pressed(void);
bool board_inverter_on_off_led_is_on(void);
