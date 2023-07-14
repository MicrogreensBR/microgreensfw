/**
 * @file temp_sensor.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-06-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include "esp_log.h"
#include "esp_err.h"

#include "dht11.h"

#include "gpio_nums.h"
#include "my_mqtt.h"

#define TEMP_SENSOR_TASK_DELAY_MS 2000

static const char *TAG = "temp_sensor";

static void TempSensor__Initialize(void)
{
    DHT11_init(TEMP_SENSOR_GPIO);
}

void TempSensor__Task(void *arg)
{
    TickType_t last_wake_time;
    last_wake_time = xTaskGetTickCount();
    vTaskSuspend(NULL);
    TempSensor__Initialize();
    struct dht11_reading sensor_reading;
    while (1)
    {
        ESP_LOGD(TAG, "Inside task");
        sensor_reading = DHT11_read();
        if (sensor_reading.status == DHT11_OK)
        {
            ESP_LOGI(TAG, "temp = %d celsius degrees", sensor_reading.temperature);
            sensors_vars.temp = sensor_reading.temperature;
            sensors_vars.have_temp = 1;
        }
        else
        {
            ESP_LOGW(TAG, "error reading temperature");
        }
        xTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(TEMP_SENSOR_TASK_DELAY_MS));
    }
}