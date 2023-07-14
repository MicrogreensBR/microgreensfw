/**
 * @file lights.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-06-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "gpio_nums.h"

static const char *TAG = "Saídas";

// #define LED_GPIO 2
// #define WATER_GPIO 19
// #define COOLER_GPIO 21

static uint8_t s_led_state = 1;
static uint8_t s_water_state = 1;
static uint8_t s_cooler_state = 0;

void Outputs__ChangeWaterState(void)
{
    s_water_state = !s_water_state;
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(WATER_GPIO, s_water_state);
}

void Outputs__ChangeCoolerState(void)
{
    s_cooler_state = !s_cooler_state;
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(COOLER_GPIO, s_cooler_state);
}

void Outputs__ChangeLedState(void)
{
    s_led_state = !s_led_state;
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(LED_GPIO, s_led_state);
}

void Outputs__Initialize(void)
{
    ESP_LOGI(TAG, "Configurando saidas...");
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, s_led_state);
    gpio_reset_pin(WATER_GPIO);
    gpio_set_direction(WATER_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(WATER_GPIO, s_water_state);
    gpio_reset_pin(COOLER_GPIO);
    gpio_set_direction(COOLER_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(COOLER_GPIO, s_cooler_state);
    ESP_LOGI(TAG, "Saidas configuradas.");
}
