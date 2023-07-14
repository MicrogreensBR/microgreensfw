/**
 * @file hum_sensor.c
 * @author your name (you@domain.com)
 * @brief Functions to read humidity sensor data
 * @version 0.1
 * @date 2023-06-03
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

#include "hum_sensor_prm.h"
#include "http.h"
#include "my_mqtt.h"

#define AIR_VAL_MV 660
#define WATER_VAL_MV 385

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_3; // ADC_CHANNEL_6: GPIO34 if ADC1, GPIO14 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
static const adc_atten_t atten = ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;
const char *TAG = "humidity_sensor";

static void check_efuse(void)
{
    // Check if TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
    {
        printf("eFuse Two Point: Supported\n");
    }
    else
    {
        printf("eFuse Two Point: NOT supported\n"); // this
    }
    // Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
    {
        printf("eFuse Vref: Supported\n"); // this
    }
    else
    {
        printf("eFuse Vref: NOT supported\n");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        printf("Characterized using Two Point Value\n");
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        printf("Characterized using eFuse Vref\n"); // this
    }
    else
    {
        printf("Characterized using Default Vref\n");
    }
}

static void HumSensor__Initialize(void)
{
    // Check if Two Point or Vref are burned into eFuse
    check_efuse();

    // Configure ADC
    if (unit == ADC_UNIT_1)
    {
        adc1_config_width(width);
        adc1_config_channel_atten(channel, atten);
    }
    else
    {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    // Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);
}

void HumSensor__Task(void *arg)
{
    TickType_t last_wake_time;
    last_wake_time = xTaskGetTickCount();
    vTaskSuspend(NULL);
    HumSensor__Initialize();
    while (1)
    {
        ESP_LOGD(TAG, "Inside task");
        uint32_t adc_reading = 0;
        // Multisampling
        for (int i = 0; i < NO_OF_SAMPLES; i++)
        {
            ESP_LOGD(TAG, "Getting sample #%d", i);
            if (unit == ADC_UNIT_1)
            {
                adc_reading += adc1_get_raw((adc1_channel_t)channel);
            }
            else
            {
                int raw;
                adc2_get_raw((adc2_channel_t)channel, width, &raw);
                adc_reading += raw;
            }
        }
        adc_reading /= NO_OF_SAMPLES;
        // Convert adc_reading to voltage in mV
        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        ESP_LOGI(TAG, "Raw: %ld\tVoltage: %ldmV\n", adc_reading, voltage);
        sensors_vars.have_hum = 1;
        // sensors_vars.hum = (voltage * 100) / DEFAULT_VREF;
        if (voltage > AIR_VAL_MV)
            voltage = AIR_VAL_MV;
        else if (voltage < WATER_VAL_MV)
            voltage = WATER_VAL_MV;
        sensors_vars.hum = ((AIR_VAL_MV - voltage) * 100) / (AIR_VAL_MV - WATER_VAL_MV);
        ESP_LOGI(TAG, "Hum = %.1f", sensors_vars.hum);
        xTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(HUM_SENSOR_TASK_DELAY_MS));
    }
}