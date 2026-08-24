#ifndef INVERTER_H
#define INVERTER_H

#include <stdbool.h>

void inverter_init(void);

bool inverter_is_on(void);

bool inverter_turn_on(void);
bool inverter_turn_off(void);

#endif