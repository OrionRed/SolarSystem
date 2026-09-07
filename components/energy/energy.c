#include "energy.h"

#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"

#define ENERGY_THRESHOLD_A 0.5f
#define ENERGY_DEFAULT_BATTERY_CAPACITY_WH 1280.0
#define ENERGY_NVS_NAMESPACE "energy"
#define ENERGY_NVS_KEY "state"
#define ENERGY_STATE_MAGIC 0x454E4731u
#define ENERGY_STATE_VERSION 1u
#define ENERGY_SAVE_INTERVAL_US (15LL * 60LL * 1000000LL)
#define ENERGY_SAVE_DELTA_WH 10.0

typedef struct
{
    uint32_t magic;
    uint32_t version;
    double charge_wh;
    double discharge_wh;
    uint64_t charge_time_s;
    uint64_t discharge_time_s;
    double battery_energy_wh;
    double battery_capacity_wh;
    uint8_t battery_calibrated;
} energy_persist_t;

typedef struct
{
    energy_stats_t stats;
    int64_t previous_us;
    float previous_power_w;
    int64_t last_save_us;
    double energy_at_last_save_wh;
    bool have_previous;
    bool persist_dirty;
} energy_state_t;

static const char *TAG = "Energy";
static energy_state_t energy;

static esp_err_t energy_save(void)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(ENERGY_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS for save: %s", esp_err_to_name(err));
        return err;
    }

    energy_persist_t saved = {
        .magic = ENERGY_STATE_MAGIC,
        .version = ENERGY_STATE_VERSION,
        .charge_wh = energy.stats.charge_wh,
        .discharge_wh = energy.stats.discharge_wh,
        .charge_time_s = energy.stats.charge_time_s,
        .discharge_time_s = energy.stats.discharge_time_s,
        .battery_energy_wh = energy.stats.battery_energy_wh,
        .battery_capacity_wh = energy.stats.battery_capacity_wh,
        .battery_calibrated = energy.stats.battery_calibrated ? 1 : 0,
    };

    err = nvs_set_blob(nvs, ENERGY_NVS_KEY, &saved, sizeof(saved));
    if (err == ESP_OK)
    {
        err = nvs_commit(nvs);
    }

    nvs_close(nvs);

    if (err == ESP_OK)
    {
        energy.last_save_us = esp_timer_get_time();
        energy.energy_at_last_save_wh = energy.stats.net_change_wh;
        energy.persist_dirty = false;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to save energy state: %s", esp_err_to_name(err));
    }

    return err;
}

static void energy_load(void)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(ENERGY_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open NVS for load: %s", esp_err_to_name(err));
        return;
    }

    energy_persist_t saved = {0};
    size_t size = sizeof(saved);
    err = nvs_get_blob(nvs, ENERGY_NVS_KEY, &saved, &size);
    nvs_close(nvs);

    if (err == ESP_OK && size == sizeof(saved) &&
        saved.magic == ENERGY_STATE_MAGIC &&
        saved.version == ENERGY_STATE_VERSION)
    {
        energy.stats.charge_wh = saved.charge_wh;
        energy.stats.discharge_wh = saved.discharge_wh;
        energy.stats.charge_time_s = saved.charge_time_s;
        energy.stats.discharge_time_s = saved.discharge_time_s;
        energy.stats.battery_energy_wh = saved.battery_energy_wh;
        energy.stats.battery_capacity_wh = saved.battery_capacity_wh;
        energy.stats.battery_calibrated = saved.battery_calibrated != 0;

        ESP_LOGI(TAG, "Loaded persistent energy state: charge %.3f Wh, discharge %.3f Wh",
                 energy.stats.charge_wh, energy.stats.discharge_wh);
    }
    else if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ESP_LOGI(TAG, "No saved energy state; starting fresh");
    }
    else if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "No valid saved energy state: %s", esp_err_to_name(err));
    }

    energy.stats.net_change_wh =
        energy.stats.charge_wh - energy.stats.discharge_wh;
    energy.energy_at_last_save_wh = energy.stats.net_change_wh;
}

void energy_init(void)
{
    memset(&energy, 0, sizeof(energy));
    energy.stats.current_flow = ENERGY_FLOW_DISCHARGE;
    energy.stats.battery_capacity_wh = ENERGY_DEFAULT_BATTERY_CAPACITY_WH;
    energy_load();
    energy.last_save_us = esp_timer_get_time();
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

        if (energy.stats.battery_calibrated)
        {
            energy.stats.battery_energy_wh += interval_wh;
        }
    }
    else
    {
        energy.stats.discharge_wh += interval_wh;
        energy.stats.discharge_time_s += (uint64_t)(interval_s + 0.5);

        if (energy.stats.battery_calibrated)
        {
            energy.stats.battery_energy_wh -= interval_wh;
            if (energy.stats.battery_energy_wh < 0.0)
            {
                energy.stats.battery_energy_wh = 0.0;
            }
        }
    }

    energy.stats.net_change_wh =
        energy.stats.charge_wh - energy.stats.discharge_wh;
    energy.stats.samples++;
    energy.persist_dirty = true;

    /* Persist infrequently to avoid unnecessary flash writes. A maximum
     * 10 Wh / 15 minute gap is small compared with this 1.28 kWh battery. */
    if (energy.persist_dirty &&
        ((energy.stats.net_change_wh - energy.energy_at_last_save_wh >= ENERGY_SAVE_DELTA_WH) ||
         (energy.energy_at_last_save_wh - energy.stats.net_change_wh >= ENERGY_SAVE_DELTA_WH) ||
         (now_us - energy.last_save_us >= ENERGY_SAVE_INTERVAL_US)))
    {
        energy_save();
    }

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

void energy_calibrate_full(void)
{
    energy.stats.battery_energy_wh = energy.stats.battery_capacity_wh;
    energy.stats.battery_calibrated = true;
    energy.persist_dirty = true;

    ESP_LOGI(TAG, "Battery calibrated FULL at %.1f Wh", energy.stats.battery_capacity_wh);
    energy_save();
}
