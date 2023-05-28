/**
 * @file wifi.h
 * @author Henrique Sander Lourenço
 * @brief
 * @version 0.1
 * @date 2023-04-19
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef WIFI_H
#define WIFI_H

typedef enum
{
    wifi_uninitialized,
    wifi_initializing,
    wifi_disconnected,
    wifi_connecting,
    wifi_connected,
    wifi_idle,
    wifi_waiting,
} wifi_status_t;

extern wifi_status_t wifi_status;

void Wifi__ChangeWiFi(char *ssid, char *pwd);
esp_err_t Wifi__Initialize(void);
void Wifi__Task(void *arg);

#endif