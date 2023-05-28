/**
 * @file tasks.c
 * @author Henrique Sander Lourenço
 * @brief Funções para criação e controle das tasks
 * @version 0.1
 * @date 2023-05-23
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tasks.h"
#include "stma.h"
#include "wifi.h"
#include "http.h"

TaskHandle_t tasks_handles[th_size];

void Tasks__CreateAll(void)
{
    // memset(tasks_handles, NULL, th_size * sizeof(tasks_handles));
    xTaskCreate(Wifi__Task, "Wifi__Task", 4096, NULL, 10, &tasks_handles[th_wifi]);
    xTaskCreate(Http__Task, "Http__Task", 4096, NULL, 10, &tasks_handles[th_http]);
    vTaskResume(tasks_handles[th_wifi]);
}