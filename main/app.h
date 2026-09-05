#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    APP_STATE_IDLE,
    APP_STATE_WAITING_FOR_BATTERY,
    APP_STATE_CHARGING,
    APP_STATE_CHARGE_COMPLETE,
    APP_STATE_COOLDOWN,
    APP_STATE_FAULT
} app_state_id_t;

void app_init(void);
void app_run(void);
void app_request_charge(void);
void app_request_charge_abort(void);
app_state_id_t app_get_state(void);

