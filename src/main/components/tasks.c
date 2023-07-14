/**
 * @file tasks.c
 * @author Henrique Sander Lourenço
 * @brief Funções para criação e controle das tasks
 * @version 0.1
 * @date 2023-05-23
 *
 * @copyright Copyright (c) 2023 MicrogreensBR
 *
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tasks.h"
#include "stma.h"
#include "my_wifi.h"
#include "http.h"
#include "my_mqtt.h"
#include "hum_sensor.h"
#include "temp_sensor.h"

TaskHandle_t tasks_handles[th_size];

void Tasks__CreateAll(void)
{
    xTaskCreate(Stma__Task, "Stma__Task", 1024 * 64, NULL, 10, &tasks_handles[th_stma]);
    xTaskCreate(HumSensor__Task, "HumSensor__Task", 1024 * 4, NULL, 10, &tasks_handles[th_hum_sens]);
    xTaskCreate(TempSensor__Task, "TempSensor__Task", 1024 * 4, NULL, 10, &tasks_handles[th_temp_sens]);
}