/**
 * @file http.c
 * @author Henrique Sander Lourenco (https://github.com/hsanderr)
 * @brief Funções e utilizadades para comunicação HTTP
 * @version 0.1
 * @date 2023-05-28
 *
 * @copyright Copyright (c) 2023 MicrogreensBR
 *
 */

#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "nvs_flash.h"
#include "esp_check.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"

#include "http.h"
#include "http_prm.h"
#include "mqtt_prm.h"

static const char *TAG = "http";

static esp_http_client_handle_t client;

static char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer; // Buffer to store response of http request from event handler
    static int output_len;      // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        /*
         *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
         *  However, event handler can also be used in case chunked encoding is used.
         */
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            ESP_LOGI(TAG, "Data: %.*s", evt->data_len, (char *)evt->data);
            // If user_data buffer is configured, copy the response into the buffer
            int copy_len = 0;
            if (evt->user_data)
            {
                copy_len = (int)fmin((double)evt->data_len, (double)(MAX_HTTP_OUTPUT_BUFFER - output_len));
                if (copy_len)
                {
                    memcpy(evt->user_data + output_len, evt->data, copy_len);
                }
            }
            else
            {
                const int buffer_len = esp_http_client_get_content_length(evt->client);
                if (output_buffer == NULL)
                {
                    output_buffer = (char *)malloc(buffer_len);
                    output_len = 0;
                    if (output_buffer == NULL)
                    {
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                copy_len = (int)fmin((double)evt->data_len, (double)(buffer_len - output_len));
                if (copy_len)
                {
                    memcpy(output_buffer + output_len, evt->data, copy_len);
                }
            }
            output_len += copy_len;
        }
        else
        {
            ESP_LOGI(TAG, "Chunked response received");
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL)
        {
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        if (output_buffer != NULL)
        {
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

static esp_err_t Http__Perform(esp_http_client_handle_t http_client)
{
    esp_err_t err;
    err = esp_http_client_perform(http_client);
    if (err == ESP_OK)
    {
        int status = esp_http_client_get_status_code(http_client);
        ESP_LOGI(TAG, "HTTP request Status = %d, content_length = %" PRIu64,
                 status,
                 esp_http_client_get_content_length(http_client));
        if (status != 204 && status != 200)
            return ESP_FAIL;
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_close(http_client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP request request failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t Http__Initialize(void)
{
    char path[] = HTTP_FIRST_TEST_PATH;
    esp_http_client_config_t config = {
        .host = HTTP_FIRST_TEST_HOST,
        .path = HTTP_FIRST_TEST_PATH,
        .transport_type = HTTP_TRANSPORT_OVER_TCP,
        .method = HTTP_METHOD_GET,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer, // Pass address of local buffer to get response
        .disable_auto_redirect = false,
        .timeout_ms = HTTP_TIMEOUT_MS,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    client = esp_http_client_init(&config);

    if (client == NULL)
    {
        ESP_LOGE(TAG, "Error initializing HTTP client");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "HTTP client initialized, host: %s", config.host);

    ESP_LOGI(TAG, "Performing first HTTP test...");

    return Http__Perform(client);
}