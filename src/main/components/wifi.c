/**
 * @file wifi.c
 * @author Henrique Sander Lourenço
 * @brief
 * @version 0.1
 * @date 2023-04-19
 *
 * @copyright Copyright (c) 2023 MicrogreensBR
 *
 */

#include <time.h>
#include <sys/time.h>
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
#include "nvs.h"
#include "esp_sntp.h"
#include "sntp.h"
#include "esp_system.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "my_wifi.h"
#include "wifi_prm.h"
#include "http.h"
#include "my_mqtt.h"
#include "tasks.h"
#include "stma.h"

#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_UNSPECIFIED
#define H2E_IDENTIFIER ""

static const char *TAG = "wifi";
static int retry_num = 0;
static EventGroupHandle_t s_wifi_event_group; /* FreeRTOS event group to signal when we are connected */

wifi_status_t wifi_status = wifi_uninitialized;

uint8_t first_conn_since_boot = 1;
uint8_t http_initialized = 0;
uint8_t mqtt_initialized = 0;

struct timeval tv_now;

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
        // .pmf_cfg = {
        //     .required = true,
        // },
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
        nvs_handle_t my_handle;
        int32_t first_conn = 1;
        esp_err_t ret = nvs_open("storage", NVS_READWRITE, &my_handle);
        if (ret != ESP_OK)
        {
            printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
        }
        else
        {
            printf("Done\n");

            // Read
            printf("Checking if first connection has already been made ... ");
            ret = nvs_get_i32(my_handle, "first_conn", &first_conn);
            switch (ret)
            {
            case ESP_OK:
                printf("first_conn=%" PRId32 "\n", first_conn);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet, itintializing it right now...\n");
                // Write
                printf("Writing first connection (1) to NVS");
                ret = nvs_set_i32(my_handle, "first_conn", first_conn);
                if (ret != ESP_OK)
                {
                    printf("Failed with code %d!\n", ret);
                }
                printf("Done\n");

                // Commit written value.
                // After setting any values, nvs_commit() must be called to ensure changes are written
                // to flash storage. Implementations may write to storage at other times,
                // but this is not guaranteed.
                printf("Committing updates in NVS ... ");
                ret = nvs_commit(my_handle);
                printf((ret != ESP_OK) ? "Failed!\n" : "Done\n");
                break;
            default:
                printf("Error (%s) reading!\n", esp_err_to_name(ret));
                esp_restart();
            }
        }
        if (first_conn_since_boot)
        {
            vTaskDelay(pdMS_TO_TICKS(DELAY_AFTER_CONNECT_MS));
            first_conn_since_boot = 0;
            esp_err_t ret;
            uint8_t retry = 0;

            if (first_conn)
            {
                do
                {
                    if (retry > 5)
                    {
                        ESP_LOGE(TAG, "HTTP failed to initialize multiple times");
                        break;
                    }
                    ret = Http__Initialize();
                    retry++;
                    vTaskDelay(pdMS_TO_TICKS(HTTP_MQTT_INIT_RETRY_DELAY_MS));
                } while (ret != ESP_OK);

                if (ret == ESP_OK)
                {
                    printf("Writing first connection (0) to NVS");
                    first_conn = 0;
                    ret = nvs_set_i32(my_handle, "first_conn", first_conn);
                    if (ret != ESP_OK)
                    {
                        printf("Failed with code %d!\n", ret);
                    }
                    printf("Done\n");
                }
            }

            do
            {
                if (retry > 5)
                {
                    ESP_LOGE(TAG, "MQTT failed to initialize multiple times");
                    break;
                }
                ret = Mqtt__Initialize();
                retry++;
                vTaskDelay(pdMS_TO_TICKS(HTTP_MQTT_INIT_RETRY_DELAY_MS));
            } while (ret != ESP_OK);

            if (ret == ESP_OK)
                mqtt_initialized = 1;

            if ((http_initialized && mqtt_initialized) ||
                (mqtt_initialized && !first_conn))
            // if (mqtt_initialized)
            {
                ESP_LOGI(TAG, "communication protocols initialized");
                st_next = st_working;
            }
            else
            {
                ESP_LOGI(TAG, "Error initializing communication protocols");
                st_next = st_broken;
            }
            vTaskResume(tasks_handles[th_stma]);
        }

        // Close
        nvs_close(my_handle);
        wifi_status = wifi_connected;
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
            retry_num = 0;
        }
        wifi_status = wifi_disconnected;
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        retry_num = 0;
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
    esp_wifi_stop();
    // esp_wifi_deauth_sta(0);
    strcpy((char *)&wifi_config_sta.sta.ssid, ssid);
    strcpy((char *)&wifi_config_sta.sta.password, pwd);

    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    }
    else
    {
        printf("Done\n");

        // Read
        printf("Writing ssid to NVS ... ");

        ret = (nvs_set_str(my_handle, "ssid", ssid));
        switch (ret)
        {
        case ESP_OK:
            printf("success");
            break;
        default:
            printf("Error (%s) writing!\n", esp_err_to_name(ret));
            break;
        }

        printf("Writing pwd to NVS ... ");

        ret = (nvs_set_str(my_handle, "pwd", pwd));
        switch (ret)
        {
        case ESP_OK:
            printf("success");
            break;
        default:
            printf("Error (%s) writing!\n", esp_err_to_name(ret));
            break;
        }

        // Close
        nvs_close(my_handle);
    }
    // memcpy(wifi_config_sta.sta.ssid, ssid, strlen(ssid));
    // memcpy(wifi_config_sta.sta.password, pwd, strlen(pwd));
    ESP_LOGI(TAG, "Changing WiFi configuration, new ssid = %s, new pwd = %s", wifi_config_sta.sta.ssid, wifi_config_sta.sta.password);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta));
    retry_num = 0;
    esp_wifi_start();
    esp_wifi_connect();
}

esp_err_t Wifi__Initialize(void)
{
    ESP_LOGI(TAG, "ESP_WIFI_MODE_APSTA");

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

    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &my_handle);
    char ssid[64] = {0};
    char pwd[64] = {0};
    if (ret != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    }
    else
    {
        printf("Done\n");

        // Read
        printf("Reading ssid from NVS ... ");
        size_t *ssid_len;
        size_t *pwd_len;
        ret = (nvs_get_str(my_handle, "ssid", ssid, &ssid_len));
        switch (ret)
        {
        case ESP_OK:
            printf("Done\n");
            printf("ssid=%s\n", ssid);

            // Read
            printf("Reading pwd from NVS ... ");
            ret = (nvs_get_str(my_handle, "pwd", pwd, &pwd_len));
            switch (ret)
            {
            case ESP_OK:
                printf("Done\n");
                printf("pwd=%s\n", pwd);
                if (ssid_len && pwd_len)
                {
                    strcpy(&wifi_config_sta.sta.ssid, ssid);
                    strcpy(&wifi_config_sta.sta.password, pwd);
                }
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default:
                printf("Error (%s) reading!\n", esp_err_to_name(ret));
                esp_restart();
            }
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            printf("The value is not initialized yet!\n");
            break;
        default:
            printf("Error (%s) reading!\n", esp_err_to_name(ret));
            esp_restart();
        }

        // Close
        nvs_close(my_handle);
    }

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "esp_wifi_set_mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap), TAG, "esp_wifi_set_config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta), TAG, "esp_wifi_set_config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "esp_wifi_start failed");

    ESP_LOGI(TAG, "WiFi initialized");
    return ESP_OK;
}