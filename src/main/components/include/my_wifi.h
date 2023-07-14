/**
 * @file my_wifi.h
 * @author Henrique Sander Lourenço
 * @brief
 * @version 0.1
 * @date 2023-04-19
 *
 * @copyright Copyright (c) 2023 MicrogreensBR
 *
 */

#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"

extern int32_t first_conn;

typedef enum
{
    wifi_uninitialized,
    wifi_disconnected,
    wifi_connected,
} wifi_status_t;

extern wifi_status_t wifi_status;

void Wifi__ChangeWiFi(char *ssid, char *pwd);
esp_err_t Wifi__Initialize(void);
void Wifi__Task(void *arg);

#endif