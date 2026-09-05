#include "energy.h"

#include <string.h>

#include "esp_timer.h"

#define ENERGY_THRESHOLD_A 0.5f

typedef struct
{
    energy_stats_t stats;
    int64_t previous_us;
    float previous_power_w;
    bool have_previous;
} energy_state_t;

static energy_state_t energy;

void energy_init(void)
{
    memset(&energy, 0, sizeof(energy));
    energy.stats.current_flow = ENERGY_FLOW_DISCHARGE;
}

void energy_update(const pzem_data_t *pzem, bool inverter_on, bool charging_state)
{
    if (pzem == NULL)
    {
        return;
    }

    int64_t now_us = esp_timer_get_time();
    energy_flow_t flow;

    /*
     * The PZEM current is unsigned, so direction is assigned by the
     * application's operating-state heuristic rather than inferred from
     * the PZEM reading.
     */
    if (pzem->current <= ENERGY_THRESHOLD_A)
    {
        flow = ENERGY_FLOW_DISCHARGE;
    }
    else if (!inverter_on)
    {
        flow = ENERGY_FLOW_CHARGE;
    }
    else if (charging_state)
    {
        flow = ENERGY_FLOW_DISCHARGE;
    }
    else
    {
        flow = ENERGY_FLOW_CHARGE;
    }

    energy.stats.current_flow = flow;

    if (!energy.have_previous)
    {
        energy.previous_us = now_us;
        energy.previous_power_w = pzem->power;
        energy.have_previous = true;
        return;
    }

    int64_t interval_us = now_us - energy.previous_us;

    /* Ignore an unexpectedly large interval rather than assigning a large
     * amount of energy to a stale reading after a long interruption. */
    if (interval_us <= 0 || interval_us > 30000000)
    {
        energy.previous_us = now_us;
        energy.previous_power_w = pzem->power;
        return;
    }

    double interval_s = (double)interval_us / 1000000.0;
    double interval_wh =
        ((double)energy.previous_power_w + (double)pzem->power) *
        0.5 * interval_s / 3600.0;

    if (flow == ENERGY_FLOW_CHARGE)
    {
        energy.stats.charge_wh += interval_wh;
        energy.stats.charge_time_s += (uint64_t)(interval_s + 0.5);
    }
    else
    {
        energy.stats.discharge_wh += interval_wh;
        energy.stats.discharge_time_s += (uint64_t)(interval_s + 0.5);
    }

    energy.stats.net_change_wh =
        energy.stats.charge_wh - energy.stats.discharge_wh;
    energy.stats.samples++;

    energy.previous_us = now_us;
    energy.previous_power_w = pzem->power;
}

void energy_get_stats(energy_stats_t *stats)
{
    if (stats == NULL)
    {
        return;
    }

    *stats = energy.stats;
}
