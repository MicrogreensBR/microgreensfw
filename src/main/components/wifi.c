/**
 * @file wifi.c
 * @author Henrique Sander Lourenço
 * @brief
 * @version 0.1
 * @date 2023-04-19
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_check.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi.h"
#include "wifi_prm.h"
#include "tasks.h"

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_UNSPECIFIED
#define H2E_IDENTIFIER ""

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

static const char *TAG = "wifi";
static int retry_num = 0;
static EventGroupHandle_t s_wifi_event_group; /* FreeRTOS event group to signal when we are connected */

wifi_status_t wifi_status;

wifi_config_t wifi_config_ap = {
    .ap = {
        .ssid = ESP_WIFI_SSID,
        .ssid_len = strlen(ESP_WIFI_SSID),
        .channel = ESP_WIFI_CHANNEL,
        .password = ESP_WIFI_PASS,
        .max_connection = MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
        .authmode = WIFI_AUTH_WPA3_PSK,
        .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
        .authmode = WIFI_AUTH_WPA2_PSK,
#endif
        .pmf_cfg = {
            .required = true,
        },
    },
};

wifi_config_t wifi_config_sta = {
    .sta = {
        /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
         * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
         * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
         * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
         */
        .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        .ssid = DEFAULT_STA_SSID,
        .password = DEFAULT_STA_PWD,
    },
};

static void evt_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "WiFi station starting, trying to connect.");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG, "Connected to the AP");
        wifi_status = wifi_connected;
        vTaskResume(tasks_handles[th_wifi]);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Disconnected from AP");
        if (retry_num < MAX_RETRIES)
        {
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(TAG, "Retrying to connect to the AP (#%d)", retry_num);
        }
        else
        {
            ESP_LOGW(TAG, "Exceeded maximum # of attempts to connect to AP");
            wifi_status = wifi_idle;
            retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " joined, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " left, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void Wifi__ChangeWiFi(char *ssid, char *pwd)
{
    ESP_LOGI(TAG, "Changing WiFi configuration, new ssid = %s, new pwd = %s", ssid, pwd);
    // wifi_config_t wifi_config = {
    //     .sta = {
    //         /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
    //          * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
    //          * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
    //          * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
    //          */
    //         .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
    //     },
    // };
    memcpy(wifi_config_sta.sta.ssid, ssid, strlen(ssid));
    memcpy(wifi_config_sta.sta.password, pwd, strlen(pwd));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta));
    retry_num = 0;
    esp_wifi_connect();
}

esp_err_t Wifi__Initialize(void)
{
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");

    s_wifi_event_group = xEventGroupCreate();

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "esp_netif_init failed");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "esp_netif_init failed");

    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "esp_wifi_init failed");

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &evt_handler,
                                                            NULL,
                                                            &instance_any_id),
                        TAG, "esp_event_handler_instance_register failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &evt_handler,
                                                            NULL,
                                                            &instance_got_ip),
                        TAG, "esp_event_handler_instance_register failed");

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "esp_wifi_set_mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap), TAG, "esp_wifi_set_config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta), TAG, "esp_wifi_set_config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wsp_wifi_start failed");

    ESP_LOGI(TAG, "Wifi__Initialize finished.");
    return ESP_OK;
}

void Wifi__Task(void *arg)
{
    vTaskSuspend(NULL);
    TickType_t last_wake_time;
    last_wake_time = xTaskGetTickCount();
    wifi_status = wifi_uninitialized;
    while (1)
    {
        switch (wifi_status)
        {
        case wifi_uninitialized:
            ESP_LOGI(TAG, "Initializing WiFi");
            if (Wifi__Initialize() != ESP_OK)
            {
                ESP_LOGE(TAG, "Error initializing WiFi");
                wifi_status = wifi_idle;
            }
            else
            {
                wifi_status = wifi_waiting;
            }
            break;

        case wifi_connected:
            ESP_LOGI(TAG, "WiFi connected to AP");
            vTaskResume(tasks_handles[th_mqtt]);
            vTaskResume(tasks_handles[th_http]);
            wifi_status = wifi_idle;
            break;

        case wifi_idle:
            ESP_LOGI(TAG, "Suspending WiFi task");
            vTaskSuspend(NULL);
            break;

        case wifi_waiting:
            ESP_LOGI(TAG, "WiFi waiting");
            break;

        default:
            ESP_LOGI(TAG, "Wrong wifi_status");
            break;
        }
        xTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(WIFI_TASK_DELAY_MS));
    }
}
