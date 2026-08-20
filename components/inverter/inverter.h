#ifndef INVERTER_H
#define INVERTER_H

#include <stdbool.h>

void inverter_init(void);
bool inverter_set_enabled(bool enabled);
bool inverter_is_on(void);

#endif