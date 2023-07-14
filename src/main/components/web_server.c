/**
 * @file web_server.c
 * @author Henrique Sander Lourenço
 * @brief
 * @version 0.1
 * @date 2023-05-07
 *
 * @copyright Copyright (c) 2023 MicrogreensBR
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> //Requires by memset
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <esp_http_server.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>

#include "my_wifi.h"

char web_page[] = "<!DOCTYPE html><html lang=\"pt-BR\"> <head> <meta charset=\"UTF-8\" /> <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\" /> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" /> <title>Babygreens web page</title> </head> <body style=\"padding: 20px\"> <h1>Conecte o Babygreens à internet</h1> <form action=\"/\" method=\"post\"> <label for=\"ssid\">SSID:</label> <br /> <input style=\"margin-bottom: 20px\" type=\"text\" id=\"ssid\" name=\"ssid\" /> <br /> <label for=\"pwd\">Senha:</label> <br /> <input style=\"margin-bottom: 20px\" type=\"password\" id=\"pwd\" name=\"pwd\" /> <br /> <button type=\"submit\">Conectar</button> </form> </body></html>";

static const char *TAG = "web_server";

esp_err_t send_web_page(httpd_req_t *req)
{
    return httpd_resp_send(req, web_page, HTTPD_RESP_USE_STRLEN);
}

esp_err_t get_req_handler(httpd_req_t *req)
{
    return send_web_page(req);
}

esp_err_t new_wifi_config_handler(httpd_req_t *req)
{
    char ssid[128] = {0}, pwd[128] = {0}, buf[1024] = {0};
    int16_t ssid_start_index = 0, ssid_end_index = 0, pwd_start_index = 0, pwd_end_index;
    ESP_LOGI(TAG, "POST request received.");
    httpd_req_recv(req, buf, 1024);
    pwd_end_index = strlen(buf);
    ESP_LOGI(TAG, "Content received (strlen = %d): %s", strlen(buf), buf);

    for (uint16_t i = 0; i < strlen(buf); i++)
    {
        buf[i] = (buf[i] == '+') ? ' ' : buf[i];

        if (buf[i] == '=' && buf[i - 1] == 'd' && buf[i - 2] == 'i' && buf[i - 3] == 's' && buf[i - 4] == 's')
        {
            ssid_start_index = i + 1;
        }
        if (buf[i] == '=' && buf[i - 1] == 'd' && buf[i - 2] == 'w' && buf[i - 3] == 'p')
        {
            ssid_end_index = i - 4;
            pwd_start_index = i + 1;
            break;
        }
    }

    if (ssid_start_index && ssid_end_index && pwd_start_index && pwd_end_index)
    {
        for (uint16_t i = ssid_start_index; i < ssid_end_index; i++)
        {
            ssid[i - ssid_start_index] = buf[i];
        }
        for (uint16_t i = pwd_start_index; i < pwd_end_index; i++)
        {
            pwd[i - pwd_start_index] = buf[i];
        }
    }

    ESP_LOGI(TAG, "ssid: %s, pwd: %s", ssid, pwd);

    esp_err_t ret = send_web_page(req);

    vTaskDelay(pdMS_TO_TICKS(2500));

    Wifi__ChangeWiFi(ssid, pwd);

    return ret;
}

httpd_uri_t uri_post = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = new_wifi_config_handler,
    .user_ctx = NULL};

httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_req_handler,
    .user_ctx = NULL};

httpd_handle_t WebServer__Initialize(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.send_wait_timeout = 10;
    config.recv_wait_timeout = 10;
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
    }

    return server;
}