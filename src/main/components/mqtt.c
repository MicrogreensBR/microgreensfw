/**
 * @file mqtt.c
 * @author Henrique Sander Lourenco (https://github.com/hsanderr)
 * @brief Funções e utilizadades para comunicação MQTT
 * @version 0.1
 * @date 2023-05-28
 *
 * @copyright Copyright (c) 2023 MicrogreensBR
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> //necessário p/ função time()
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_check.h"
#include "mqtt_client.h"
#include "esp_crt_bundle.h"
#include "esp_random.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "my_mqtt.h"
#include "mqtt_prm.h"
#include "outputs.h"
#include "stma.h"

#define TEST_DATA 0
#define SEND_DATA_MQTT 1

static const char *TAG = "mqtt";

static esp_mqtt_client_handle_t client;

int64_t time_to_count_s = 0;

sensors_vars_t sensors_vars = {
#if TEST_DATA
    .have_distance = 1,
    .have_hum = 1,
    .have_temp = 1,
#else
    .have_distance = 0,
    .have_hum = 0,
    .have_temp = 0,
#endif
    .dist = 0,
    .hum = 0,
    .temp = 0,
};

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    char topic[event->topic_len + 1];
    memcpy(topic, event->topic, event->topic_len);
    topic[event->topic_len] = '\0';
    char data[event->data_len + 1];
    memcpy(data, event->data, event->data_len);
    data[event->data_len] = '\0';

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        msg_id = esp_mqtt_client_subscribe(client, MQTT_TOPIC_DOWN, 0);
        if (msg_id != -1)
            ESP_LOGI(TAG, "Subscribing to %s topic, msg_id=%d", MQTT_TOPIC_DOWN, msg_id);
        else
            ESP_LOGE(TAG, "Error subscribing to %s topic", MQTT_TOPIC_DOWN);

        msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC_UP, MQTT_DATA_ESP_ON, 0, 1, 0);
        if (msg_id != -1)
            ESP_LOGI(TAG, "Publishing to %s topic, data=%s, msg_id=%d", MQTT_TOPIC_UP, MQTT_DATA_ESP_ON, msg_id);
        else
            ESP_LOGE(TAG, "Error publishing %s to %s topic", MQTT_DATA_ESP_ON, MQTT_TOPIC_UP);

        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA, msg_id=%d", event->msg_id);
        ESP_LOGI(TAG, "TOPIC=%.*s (TOPIC_LEN=%d)\r\n", event->topic_len, event->topic, event->topic_len);
        ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
        if (!strcmp(topic, MQTT_TOPIC_DOWN))
        {
            ESP_LOGI(TAG, "Received data in expected topic");
            if (!strcmp(data, MQTT_DATA_ILUMINAR))
            {
                ESP_LOGI(TAG, "Iluminar");
                Outputs__ChangeLedState();
            }
            else if (!strcmp(data, MQTT_DATA_IRRIGAR))
            {
                ESP_LOGI(TAG, "Irrigar");
                Outputs__ChangeWaterState();
            }
            else if (!strcmp(data, MQTT_DATA_VENTILAR))
            {
                ESP_LOGI(TAG, "Ventilar");
                Outputs__ChangeCoolerState();
            }
            else if (!strcmp(data, MQTT_DATA_START))
            {
                ESP_LOGI(TAG, "Iniciar recebido!");
                // esp_mqtt_client_publish(client, MQTT_TOPIC_UP, "321/finish", 10, 1, 0);
            }
            else if ((time_to_count_s = atoi(&data[4])))
            {
                if (time_to_count_s > 59)
                {
                    Stma__InitiateTimer(time_to_count_s, SOAKING_TIMER);
                    ESP_LOGI(TAG, "Tempo recebido: %" PRId64 ", iniciando contagem de tempo de embebição", time_to_count_s);
                    // esp_mqtt_client_publish(client, MQTT_TOPIC_UP, "321/finish", 10, 1, 0);
                }
                else
                {
                    if (time_to_count_s != 2)
                        Stma__InitiateTimer(time_to_count_s * 24 * 60 * 60, GROWTH_TIMER);
                    else
                        Stma__InitiateTimer(120, GROWTH_TIMER);
                    ESP_LOGI(TAG, "Tempo recebido: %" PRId64 ", iniciando contagem de tempo de cultivo", time_to_count_s);
                    // esp_mqtt_client_publish(client, MQTT_TOPIC_UP, "321/finish", 10, 1, 0);
                }
            }
            else
            {
                ESP_LOGW(TAG, "Dado inválido");
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id: %d", event->event_id);
        break;
    }
}

esp_err_t Mqtt__Initialize(void)
{
    ESP_LOGI(TAG, "Initializing MQTT");
    esp_mqtt_client_config_t mqtt_cfg = {
        // .broker.address.uri = MQTT_TEST_BROKER_URI,
        .broker.address.hostname = MQTT_TEST_BROKER_HOST,
        .broker.address.transport = MQTT_TRANSPORT_OVER_TCP,
        .broker.address.port = 1883,
        // .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
        .network.timeout_ms = 7500,
        .credentials.set_null_client_id = true,
        .buffer.out_size = 2048,
        .buffer.size = 2048,
        // .broker.address.port = 1883,
        .credentials.username = "",
        .credentials.authentication.password = "",
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL)
        return ESP_FAIL;
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    ESP_RETURN_ON_ERROR(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL), TAG, "error registering mqtt event handler");
    ESP_RETURN_ON_ERROR(esp_mqtt_client_start(client), TAG, "error starting mqtt client");
    return ESP_OK;
}

esp_err_t Mqtt__PublishVars(sensors_vars_t *vars)
{
    char payload[32] = {0};
    srand(time(NULL));
#if TEST_DATA
    sensors_vars.temp = 20.0 + ((rand() % 20) / 10.0);
    sensors_vars.dist = 1.0 + ((rand() % 3) / 100.0);
    sensors_vars.hum = 50.0 + ((rand() % 10) / 10.0);
#else
    sensors_vars.have_temp = 0;
    sensors_vars.have_distance = 0;
    sensors_vars.have_hum = 0;
#endif
    // sprintf(payload, PRODUCT_ID "/%d&%d&%d", (int)sensors_vars.temp, (int)sensors_vars.dist, (int)sensors_vars.hum);
    sprintf(payload, PRODUCT_ID "/%.1f&2&%.1f", sensors_vars.temp, sensors_vars.hum);

    ESP_LOGI(TAG, "payload=%s", payload);
// temp&hum
#if SEND_DATA_MQTT
    if (esp_mqtt_client_publish(client, MQTT_TOPIC_DATA, payload, 0, 1, 0) == -1)
        return ESP_FAIL;
    else
        return ESP_OK;
#else
    return ESP_OK;
#endif
}

esp_err_t Mqtt__PublishFinish(timer_mode_t mode)
{
    if (mode == SOAKING_TIMER)
        esp_mqtt_client_publish(client, MQTT_TOPIC_UP, PRODUCT_ID "/finish", 10, 1, 0);
    else if (mode == GROWTH_TIMER)
        esp_mqtt_client_publish(client, MQTT_TOPIC_UP, PRODUCT_ID "/colheita", 12, 1, 0);
    return ESP_OK;
}