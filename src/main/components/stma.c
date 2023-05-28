/**
 * @file stma.c
 * @author Henrique Sander Lourenco (henriquesander27@gmail.com)
 * @brief State machine functions.
 * @version 0.1
 * @date 2023-05-23
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static char *TAG = "state_machine";

typedef enum
{
    st_initial,
    st_wait
} states_t;

states_t st_current;
states_t st_next;

void Stma__Task(void *arg)
{
    TickType_t last_wake_time;
    last_wake_time = xTaskGetTickCount();
    while (1)
    {
        switch (st_current)
        {
        default:
            ESP_LOGI(TAG, "Inside Stma__Task");
            break;
        }
        st_current = st_next;
        xTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(10));
    }
}