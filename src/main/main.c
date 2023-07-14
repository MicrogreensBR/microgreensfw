/**
 * @file main.c
 * @author Henrique Sander Lourenço
 * @brief
 * @version 0.1
 * @date 2023-04-19
 *
 * @copyright Copyright (c) 2023 MicrogreensBR
 *
 */

#include "esp_log.h"
#include "esp_netif.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_check.h"

#include "stma.h"
#include "my_wifi.h"
#include "web_server.h"
#include "tasks.h"
#include "http.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Olá, este é o firmware do projeto MicrogreensBR.");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    Tasks__CreateAll();
}
