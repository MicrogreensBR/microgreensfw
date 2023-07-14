/**
 * @file stma.c
 * @author Henrique Sander Lourenco (henriquesander27@gmail.com)
 * @brief State machine functions.
 * @version 0.1
 * @date 2023-05-23
 *
 * @copyright Copyright (c) 2023 MicrogreensBR
 *
 */

#include <sys/time.h>
#include <time.h>
#include <sys/time.h>

#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "stma.h"
#include "my_wifi.h"
#include "web_server.h"
#include "tasks.h"
#include "outputs.h"
#include "my_mqtt.h"

#define STMA_TASK_DELAY_MS 2500

static char *TAG = "state_machine";

states_t st_prev;
states_t st_current;
states_t st_next;

int64_t timer_start_ts;
int64_t timer_start_ts_real;

int8_t timer_on = 0;
int32_t timer_time_s = 0;
int64_t last_check_timestamp_s = 0;
int32_t elaps_time = 0;
timer_mode_t timer_mode;
int8_t timer_on_real;
int32_t timer_time_s_real;
int64_t last_check_timestamp_s_real;
int32_t elaps_time_real;
timer_mode_t timer_mode_real;
time_t now;

void set_timer_real(void)
{
    timer_on_real = timer_on;
    timer_time_s_real = timer_time_s;
    last_check_timestamp_s_real = last_check_timestamp_s;
    elaps_time_real = elaps_time;
    timer_mode_real = timer_mode;
}

int64_t us_to_s(int64_t us)
{
    return (us / 1000000);
}

void Stma__InitiateTimer(int64_t time_s, timer_mode_t mode)
{
    if (time_s == 0)
        time_s = 1;
    timer_on = 1;
    // gettimeofday(&now, NULL);
    // last_check_timestamp_s = us_to_s(esp_timer_get_time());
    // last_check_timestamp_s = now.tv_sec;

    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    last_check_timestamp_s = now;

    timer_start_ts_real = now;
    timer_start_ts = timer_start_ts_real;

    timer_time_s = time_s;
    timer_mode = mode;
    elaps_time = 0;
    set_timer_real();
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    }
    else
    {
        printf("Done opening NVS handle\n");

        ret = nvs_set_i8(my_handle, "timer_on", timer_on_real);
        switch (ret)
        {
        case ESP_OK:
            printf("Success writing timer_on!\n");
            printf("Committing updates in NVS ... ");
            ret = nvs_commit(my_handle);
            printf((ret != ESP_OK) ? "Failed!\n" : "Done!\n");
            break;
        default:
            printf("Error (%s) writing timer_on!\n", esp_err_to_name(ret));
            break;
        }
        ret = nvs_set_i64(my_handle, "ts_start", timer_start_ts);
        switch (ret)
        {
        case ESP_OK:
            printf("Success writing timer_start_ts!\n");
            printf("Committing updates in NVS ... ");
            ret = nvs_commit(my_handle);
            printf((ret != ESP_OK) ? "Failed!\n" : "Done!\n");
            break;
        default:
            printf("Error (%s) writing timer timer_start_ts!\n", esp_err_to_name(ret));
            break;
        }
        ret = nvs_set_i32(my_handle, "timer_time_s", timer_time_s_real);
        switch (ret)
        {
        case ESP_OK:
            printf("Success writing timer_time_s!\n");
            printf("Committing updates in NVS ... ");
            ret = nvs_commit(my_handle);
            printf((ret != ESP_OK) ? "Failed!\n" : "Done!\n");
            break;
        default:
            printf("Error (%s) writing timer_time_s!\n", esp_err_to_name(ret));
            break;
        }
        ret = nvs_set_i8(my_handle, "timer_mode", timer_mode_real);
        switch (ret)
        {
        case ESP_OK:
            printf("Success writing timer_mode!\n");
            printf("Committing updates in NVS ... ");
            ret = nvs_commit(my_handle);
            printf((ret != ESP_OK) ? "Failed!\n" : "Done!\n");
            break;
        default:
            printf("Error (%s) writing timer_mode!\n", esp_err_to_name(ret));
            break;
        }
    }
    nvs_close(my_handle);
}

void Stma__Task(void *arg)
{
    TickType_t last_wake_time;
    last_wake_time = xTaskGetTickCount();

    st_prev = st_initialize;
    st_current = st_initialize;
    st_next = st_initialize;
    char states[][32] = {"st_initialize", "st_working", "st_broken", "st_wait"};
    while (1)
    {
        // if (st_current != st_prev)
        ESP_LOGI(TAG, "current state: %s", states[st_current]);
        switch (st_current)
        {
        case st_initialize:
        {
            Wifi__Initialize();
            WebServer__Initialize();
            Outputs__Initialize();
            vTaskSuspend(NULL);

            setenv("TZ", "GMT-3", 0);
            tzset();

            sntp_setoperatingmode(SNTP_OPMODE_POLL);
            sntp_setservername(0, "pool.ntp.org");
            sntp_init();

            // wait for time to be set
            time_t now = 0;
            struct tm timeinfo = {0};
            int retry = 0;
            const int retry_count = 15;
            while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
            {
                ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
                vTaskDelay(2000 / portTICK_PERIOD_MS);
            }
            time(&now);
            localtime_r(&now, &timeinfo);
            last_check_timestamp_s = now;

            nvs_handle_t my_handle;
            esp_err_t ret = nvs_open("storage", NVS_READWRITE, &my_handle);
            if (ret != ESP_OK)
            {
                printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
            }
            else
            {
                ret = nvs_get_i8(my_handle, "timer_on", &timer_on);
                switch (ret)
                {
                case ESP_OK:
                    printf("timer_on=%d\n", timer_on);
                    if (timer_on == 1)
                    {
                        ret = nvs_get_i64(my_handle, "ts_start", &timer_start_ts);
                        switch (ret)
                        {
                        case ESP_OK:
                            printf("timer_start_ts=%" PRId64 "\n", timer_start_ts);
                            ret = nvs_get_i32(my_handle, "timer_time_s", &timer_time_s);
                            switch (ret)
                            {
                            case ESP_OK:
                                printf("timer_time_s=%" PRId32 "\n", timer_time_s);
                                ret = nvs_get_i8(my_handle, "timer_mode", &timer_mode);
                                switch (ret)
                                {
                                case ESP_OK:
                                    printf("timer_mode=%d\n", timer_mode);
                                    timer_time_s_real = timer_time_s;
                                    timer_start_ts_real = timer_start_ts;
                                    timer_on_real = timer_on;
                                    timer_mode_real = timer_mode;
                                    break;
                                case ESP_ERR_NVS_NOT_FOUND:
                                    printf("timer_mode not initialized yet!\n");
                                    break;
                                default:
                                    printf("Error (%s) reading!\n", esp_err_to_name(ret));
                                    break;
                                }
                                break;
                            case ESP_ERR_NVS_NOT_FOUND:
                                printf("timer_time_s not initialized yet!\n");
                                break;
                            default:
                                printf("Error (%s) reading!\n", esp_err_to_name(ret));
                                break;
                            }
                            break;
                        case ESP_ERR_NVS_NOT_FOUND:
                            printf("timer_mode not initialized yet!\n");
                            break;
                        default:
                            printf("Error (%s) reading!\n", esp_err_to_name(ret));
                            break;
                        }
                    }
                    break;
                case ESP_ERR_NVS_NOT_FOUND:
                    printf("timer_on not initialized yet!\n");
                    break;
                default:
                    printf("Error (%s) reading!\n", esp_err_to_name(ret));
                    break;
                }
            }
            nvs_close(&my_handle);
            break;
        }
        case st_working:
            vTaskResume(tasks_handles[th_hum_sens]);
            vTaskResume(tasks_handles[th_temp_sens]);
            st_next = st_wait;
            break;
        case st_broken:
            ESP_LOGE(TAG, "Application is broken, restarting CPU");
            esp_restart();
            break;
        case st_wait:
        {
            nvs_handle_t my_handle;
            if (timer_on_real == 1)
            {
                // gettimeofday(&now, NULL);
                // int64_t current_time_s = now.tv_sec;
                char strftime_buf[64];
                struct tm timeinfo;
                time(&now);
                localtime_r(&now, &timeinfo);
                last_check_timestamp_s = now;
                int64_t current_time_s = now;
                strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
                ESP_LOGI(TAG, "The current date/time in Brasília is: %s", strftime_buf);
                // int64_t current_time_s = us_to_s(esp_timer_get_time());
                if (now - timer_start_ts_real > timer_time_s)
                {
                    Mqtt__PublishFinish(timer_mode_real);
                    // ESP_LOGI(TAG, "Timer estourado!!! current=%" PRId64 ", start=%" PRId64 ", total=%" PRId32, current_time_s, last_check_timestamp_s_real, timer_time_s_real);
                    ESP_LOGI(TAG, "Timer estourado!!!");
                    timer_on = 0;
                    timer_on_real = timer_on;
                    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &my_handle);
                    if (ret != ESP_OK)
                    {
                        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
                    }
                    else
                    {
                        printf("Done opening NVS handle, writing timer_on\n");

                        ret = nvs_set_i8(my_handle, "timer_on", timer_on_real);
                        if (ret != ESP_OK)
                        {
                            printf("Failed with code %d!\n", ret);
                        }

                        // Commit written value.
                        // After setting any values, nvs_commit() must be called to ensure changes are written
                        // to flash storage. Implementations may write to storage at other times,
                        // but this is not guaranteed.
                        printf("Committing updates in NVS ... ");
                        ret = nvs_commit(my_handle);
                        printf((ret != ESP_OK) ? "Failed!\n" : "Done\n");
                        nvs_close(my_handle);
                    }
                }
            }
#if TEST_DATA
            if (1)
#else
            if (sensors_vars.have_hum && sensors_vars.have_temp)
#endif
            {
                ESP_LOGI(TAG, "Sending humidity and temperature to app");
                Mqtt__PublishVars(&sensors_vars);
            }
            break;
        }
        default:
            break;
        }
        st_prev = st_current;
        st_current = st_next;
        xTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(STMA_TASK_DELAY_MS));
    }
}