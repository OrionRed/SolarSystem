#include "pzem.h"
#include "modbus.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static pzem_data_t pzem_data;
static SemaphoreHandle_t pzem_data_mutex;

static const char *TAG = "PZEM";
static bool pzem_data_valid = false;

static bool pzem_read(pzem_data_t *data)
{
    uint8_t response[32];

    int response_length =
        modbus_read_registers(
            1,
            0,
            6,
            response,
            sizeof(response));

    if (response_length != 17)
    {
        return false;
    }

    uint16_t voltage_raw =
        ((uint16_t)response[3] << 8) | response[4];

    uint16_t current_raw =
        ((uint16_t)response[5] << 8) | response[6];

    uint16_t power_low =
        ((uint16_t)response[7] << 8) | response[8];

    uint16_t power_high =
        ((uint16_t)response[9] << 8) | response[10];

    uint16_t energy_low =
        ((uint16_t)response[11] << 8) | response[12];

    uint16_t energy_high =
        ((uint16_t)response[13] << 8) | response[14];

    uint32_t power_raw =
        ((uint32_t)power_high << 16) | power_low;

    uint32_t energy_raw =
        ((uint32_t)energy_high << 16) | energy_low;

    data->voltage = voltage_raw / 100.0f;
    data->current = current_raw / 100.0f;
    data->power   = power_raw / 10.0f;
    data->energy  = energy_raw;

    return true;
}

static void pzem_task(void *arg)
{
    while (1)
    {
        pzem_data_t new_data;

        if (pzem_read(&new_data))
        {
            if (xSemaphoreTake(pzem_data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                pzem_data = new_data;
                pzem_data_valid = true;                
                xSemaphoreGive(pzem_data_mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void pzem_init(void)
{
    pzem_data_mutex = xSemaphoreCreateMutex();

    if (pzem_data_mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create data mutex");
        return;
    }

    xTaskCreate(
        pzem_task,
        "pzem_task",
        4096,
        NULL,
        5,
        NULL);
}

bool pzem_get_data(pzem_data_t *data)
{
    if (data == NULL)
    {
        return false;
    }

    if (xSemaphoreTake(pzem_data_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
    {
        return false;
    }

    if (!pzem_data_valid)
    {
        xSemaphoreGive(pzem_data_mutex);
        return false;
    }

    *data = pzem_data;

    xSemaphoreGive(pzem_data_mutex);

    return true;
}