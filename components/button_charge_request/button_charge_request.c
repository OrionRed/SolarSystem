#include "button_charge_request.h"

#include "board.h"
#include "charge_request_iface.h"
#include "charge_abort_iface.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON_SAMPLE_MS       20
#define BUTTON_DEBOUNCE_COUNT  3

static void button_task(void *pvParameters)
{
    bool charge_last_sample = false;
    bool charge_stable_state = false;
    int charge_stable_count = 0;

    bool az1_last_sample = false;
    bool az1_stable_state = false;
    int az1_stable_count = 0;

    while (1)
    {
        bool charge_sample = board_button_charge_request_pressed();
        bool az1_sample = board_az1_button_pressed();

        /* Charge request button */

        if (charge_sample == charge_last_sample)
        {
            if (charge_stable_count < BUTTON_DEBOUNCE_COUNT)
            {
                charge_stable_count++;
            }
        }
        else
        {
            charge_stable_count = 0;
        }

        if (charge_stable_count >= BUTTON_DEBOUNCE_COUNT &&
            charge_sample != charge_stable_state)
        {
            charge_stable_state = charge_sample;

            if (charge_stable_state)
            {
                app_request_charge();
            }
        }

        charge_last_sample = charge_sample;


        /* AZ-1 button */

        if (az1_sample == az1_last_sample)
        {
            if (az1_stable_count < BUTTON_DEBOUNCE_COUNT)
            {
                az1_stable_count++;
            }
        }
        else
        {
            az1_stable_count = 0;
        }

        if (az1_stable_count >= BUTTON_DEBOUNCE_COUNT &&
            az1_sample != az1_stable_state)
        {
            az1_stable_state = az1_sample;

            if (az1_stable_state)
            {
                app_request_charge_abort();
            }
        }

        az1_last_sample = az1_sample;

        vTaskDelay(pdMS_TO_TICKS(BUTTON_SAMPLE_MS));
    }
}

void button_charge_request_init(void)
{
    xTaskCreate(
        button_task,
        "Button Task",
        2048,
        NULL,
        5,
        NULL
    );
}