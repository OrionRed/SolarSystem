#include "demo.h"

#include "board.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void button_demo_task(void *pvParameters)
{
    while (1)
    {
        board_led_set(board_button_pressed());

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void demo_start(void)
{
    xTaskCreate(
        button_demo_task,  // Function to run
        "Button Demo",     // Task name (for debugging)
        2048,              // Stack size (in words, not bytes)
        NULL,              // Task input parameter
        5,                 // Priority
        NULL               // Task handle
    );
}