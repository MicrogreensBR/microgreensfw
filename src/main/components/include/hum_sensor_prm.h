/**
 * @file hum_sensor_prm.h
 * @author Henrique Sander Lourenço
 * @brief
 * @version 0.1
 * @date 2023-06-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef HUM_SENSOR_PRM_H
#define HUM_SENSOR_PRM_H

#define DEFAULT_VREF 3300 // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES 64  // Multisampling
#define HUM_SENSOR_TASK_DELAY_MS 2000

#endif