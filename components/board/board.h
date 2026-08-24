#pragma once

#include <stdbool.h>

void board_init(void);
void board_led_set(bool on);
bool board_inverter_on_off_led_is_on(void);

void board_inverter_switch_on(void);
void board_inverter_switch_off(void);
bool board_button_charge_request_pressed(void);
