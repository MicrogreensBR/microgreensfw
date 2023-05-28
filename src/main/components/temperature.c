/**
 * @file temperature.c
 * @author Henrique Sander Lourenco
 * @brief Functions related to temperature control.
 * @version 0.1
 * @date 2023-05-24
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "temperature.h"

void Temp__Task(void *arg)
{
    TickType_t last_wake_time;
    last_wake_time = xTaskGetTickCount();
    while (1)
    {
        xTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(10));
    }
}