#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "pzem.h"

typedef enum
{
    ENERGY_FLOW_DISCHARGE,
    ENERGY_FLOW_CHARGE
} energy_flow_t;

typedef struct
{
    double charge_wh;
    double discharge_wh;
    double net_change_wh;
    uint64_t charge_time_s;
    uint64_t discharge_time_s;
    uint32_t samples;
    energy_flow_t current_flow;
} energy_stats_t;

void energy_init(void);
void energy_update(const pzem_data_t *pzem, bool inverter_on, bool charging_state);
void energy_get_stats(energy_stats_t *stats);
