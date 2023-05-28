/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"

#include "http.h"
#include "http_prm.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

static const char *TAG = "http";

esp_http_client_handle_t client;

http_status_t http_status;

char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

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
            ESP_LOGI(TAG, "Data: %s", (char *)evt->data);
            // // If user_data buffer is configured, copy the response into the buffer
            // int copy_len = 0;
            // if (evt->user_data)
            // {
            //     copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
            //     if (copy_len)
            //     {
            //         memcpy(evt->user_data + output_len, evt->data, copy_len);
            //     }
            // }
            // else
            // {
            //     const int buffer_len = esp_http_client_get_content_length(evt->client);
            //     if (output_buffer == NULL)
            //     {
            //         output_buffer = (char *)malloc(buffer_len);
            //         output_len = 0;
            //         if (output_buffer == NULL)
            //         {
            //             ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
            //             return ESP_FAIL;
            //         }
            //     }
            //     copy_len = MIN(evt->data_len, (buffer_len - output_len));
            //     if (copy_len)
            //     {
            //         memcpy(output_buffer + output_len, evt->data, copy_len);
            //     }
            // }
            // output_len += copy_len;
        }
        else
        {
            ESP_LOGI(TAG, "Chunked response received");
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        // if (output_buffer != NULL)
        // {
        //     // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
        //     // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
        //     free(output_buffer);
        //     output_buffer = NULL;
        // }
        // output_len = 0;
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
        // esp_http_client_set_header(evt->client, "From", "user@example.com");
        // esp_http_client_set_header(evt->client, "Accept", "text/html");
        // esp_http_client_set_redirection(evt->client);
        break;
    }
    return ESP_OK;
}

static esp_err_t Http__Test(esp_http_client_handle_t http_client)
{
    esp_err_t err;
    // GET
    err = esp_http_client_perform(http_client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %" PRIu64,
                 esp_http_client_get_status_code(http_client),
                 esp_http_client_get_content_length(http_client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    return err;
    // ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, strlen(local_response_buffer));

    // esp_http_client_set_url(http_client, "http://igbt.eesc.usp.br/app-15/get/789/25&50&10");
    // err = esp_http_client_perform(http_client);
    // if (err == ESP_OK)
    // {
    //     ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %" PRIu64,
    //              esp_http_client_get_status_code(http_client),
    //              esp_http_client_get_content_length(http_client));
    // }
    // else
    // {
    //     ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    // }

    // POST
    // const char *post_data = "{\"field1\":\"value1\"}";
    // esp_http_client_set_url(http_client, "http://httpbin.org/post");
    // esp_http_client_set_method(http_client, HTTP_METHOD_POST);
    // esp_http_client_set_header(http_client, "Content-Type", "application/json");
    // esp_http_client_set_post_field(http_client, post_data, strlen(post_data));
    // err = esp_http_client_perform(http_client);
    // if (err == ESP_OK)
    // {
    //     ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %" PRIu64,
    //              esp_http_client_get_status_code(http_client),
    //              esp_http_client_get_content_length(http_client));
    // }
    // else
    // {
    //     ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    // }

    // // PUT
    // esp_http_client_set_url(http_client, "http://httpbin.org/put");
    // esp_http_client_set_method(http_client, HTTP_METHOD_PUT);
    // err = esp_http_client_perform(http_client);
    // if (err == ESP_OK)
    // {
    //     ESP_LOGI(TAG, "HTTP PUT Status = %d, content_length = %" PRIu64,
    //              esp_http_client_get_status_code(http_client),
    //              esp_http_client_get_content_length(http_client));
    // }
    // else
    // {
    //     ESP_LOGE(TAG, "HTTP PUT request failed: %s", esp_err_to_name(err));
    // }

    // // PATCH
    // esp_http_client_set_url(http_client, "http://httpbin.org/patch");
    // esp_http_client_set_method(http_client, HTTP_METHOD_PATCH);
    // esp_http_client_set_post_field(http_client, NULL, 0);
    // err = esp_http_client_perform(http_client);
    // if (err == ESP_OK)
    // {
    //     ESP_LOGI(TAG, "HTTP PATCH Status = %d, content_length = %" PRIu64,
    //              esp_http_client_get_status_code(http_client),
    //              esp_http_client_get_content_length(http_client));
    // }
    // else
    // {
    //     ESP_LOGE(TAG, "HTTP PATCH request failed: %s", esp_err_to_name(err));
    // }

    // // DELETE
    // esp_http_client_set_url(http_client, "http://httpbin.org/delete");
    // esp_http_client_set_method(http_client, HTTP_METHOD_DELETE);
    // err = esp_http_client_perform(client);
    // if (err == ESP_OK)
    // {
    //     ESP_LOGI(TAG, "HTTP DELETE Status = %d, content_length = %" PRIu64,
    //              esp_http_client_get_status_code(http_client),
    //              esp_http_client_get_content_length(http_client));
    // }
    // else
    // {
    //     ESP_LOGE(TAG, "HTTP DELETE request failed: %s", esp_err_to_name(err));
    // }

    // // HEAD
    // esp_http_client_set_url(http_client, "http://httpbin.org/get");
    // esp_http_client_set_method(http_client, HTTP_METHOD_HEAD);
    // err = esp_http_client_perform(http_client);
    // if (err == ESP_OK)
    // {
    //     ESP_LOGI(TAG, "HTTP HEAD Status = %d, content_length = %" PRIu64,
    //              esp_http_client_get_status_code(http_client),
    //              esp_http_client_get_content_length(http_client));
    // }
    // else
    // {
    //     ESP_LOGE(TAG, "HTTP HEAD request failed: %s", esp_err_to_name(err));
    // }

    esp_http_client_cleanup(http_client);
}

static esp_err_t Http__Initialize(void)
{
    char id_str[MAX_ID_DIGITS + 1] = {0};
    sprintf(id_str, "%d", PRODUCT_ID);
    char path[HTTP_MAX_PATH_LEN] = HTTP_DEFAUlT_BASE_PATH;

    strcat(path, id_str);
    ESP_LOGI(TAG, "Resulting path: %s", path);

    esp_http_client_config_t config = {
        .url = HTTP_FIRST_TEST_URL,
        // .host = HTTP_CONFIG_HOST,
        // .path = path,
        // .path = "/app-15/add/esp/789",
        // .query = "esp",
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer, // Pass address of local buffer to get response
        .disable_auto_redirect = false,
        .timeout_ms = HTTP_TIMEOUT_MS,
    };

    client = esp_http_client_init(&config);

    if (client == NULL)
    {
        ESP_LOGE(TAG, "Error initializing HTTP client");
        return;
    }
    else
    {
        ESP_LOGI(TAG, "HTTP client initialized, host: %s", config.host);
    }

    vTaskDelay(pdMS_TO_TICKS(HTTP_TEST_DELAY_MS));

    Http__Test(client);
}

void Http__Task(void *arg)
{
    vTaskSuspend(NULL);
    TickType_t last_wake_time;
    last_wake_time = xTaskGetTickCount();
    http_status = http_uninitialized;
    while (1)
    {
        switch (http_status)
        {
        case http_uninitialized:
            ESP_LOGI(TAG, "Initializing HTTP");
            if (Http__Initialize() == ESP_OK)
            {
                http_status = http_idle;
            }
            else
            {
                esp_http_client_close(client);
                http_status = http_uninitialized;
            }
            break;
        case http_idle:
            ESP_LOGI(TAG, "Suspending HTTP task");
            vTaskSuspend(NULL);
            break;
        default:
            ESP_LOGI(TAG, "Wrong http_status");
            break;
        }
        xTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(HTTP_TASK_DELAY_MS));
    }
}