#include "button_charge_request.h"

#include "board.h"
#include "charge_request_iface.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON_SAMPLE_MS       20
#define BUTTON_DEBOUNCE_COUNT  3

static void button_charge_request_task(void *pvParameters)
{
    bool last_sample = false;
    bool stable_state = false;
    int stable_count = 0;

    while (1)
    {
        bool sample = board_button_charge_request_pressed();

        if (sample == last_sample)
        {
            if (stable_count < BUTTON_DEBOUNCE_COUNT)
            {
                stable_count++;
            }
        }
        else
        {
            stable_count = 0;
        }

        if (stable_count >= BUTTON_DEBOUNCE_COUNT &&
            sample != stable_state)
        {
            stable_state = sample;

            if (stable_state)
            {
                app_request_charge();
            }
        }

        last_sample = sample;

        vTaskDelay(pdMS_TO_TICKS(BUTTON_SAMPLE_MS));
    }
}

void button_charge_request_init(void)
{
    xTaskCreate(
        button_charge_request_task,
        "Button Charge Request Task",
        2048,
        NULL,
        5,
        NULL
    );
}