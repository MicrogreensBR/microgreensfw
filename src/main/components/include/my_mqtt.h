/**
 * @file mqtt.h
 * @author Henrique Sander Lourenco (https://github.com/hsanderr)
 * @brief Cabeçalho para o arquivo fonte mqtt.c
 * @version 0.1
 * @date 2023-05-28
 *
 * @copyright Copyright (c) 2023 MicrogreensBR
 *
 */

#ifndef MQTT_H
#define MQTT_H

#include "stma.h"

typedef struct
{
    uint8_t have_hum;
    uint8_t have_temp;
    uint8_t have_distance;
    float hum;
    float temp;
    float dist;
} sensors_vars_t;

extern sensors_vars_t sensors_vars;

esp_err_t Mqtt__PublishFinish(timer_mode_t mode);
esp_err_t Mqtt__Initialize(void);
esp_err_t Mqtt__PublishVars(sensors_vars_t *vars);

#endif