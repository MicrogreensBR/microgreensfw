/**
 * @file mqtt_prm.h
 * @author Henrique Sander Lourenco (https://github.com/hsanderr)
 * @brief Parâmetros para comunicação MQTT.
 * @version 0.1
 * @date 2023-05-28
 *
 * @copyright Copyright (c) 2023 MicrogreensBR
 *
 */

// /microgreens/mqtt/usp/up
// /microgreens/mqtt/usp/down
// /microgreens/mqtt/usp/data xx&xx&xx

#ifndef MQTT_PRM_H
#define MQTT_PRM_H

#define PRODUCT_ID "999"
#define MQTT_TASK_DELAY_MS 1000
#define MQTT_TEST_BROKER_HOST "broker.emqx.io"
#define MQTT_TOPIC_UP "/microgreens/usp/mqtt/up"
#define MQTT_TOPIC_DOWN "/microgreens/usp/mqtt/down"
#define MQTT_TOPIC_DATA "/microgreens/usp/mqtt/data"
#define MQTT_DATA_ESP_ON PRODUCT_ID "/esp-on"
#define MQTT_DATA_IRRIGAR PRODUCT_ID "/irrigar"
#define MQTT_DATA_ILUMINAR PRODUCT_ID "/iluminar"
#define MQTT_DATA_VENTILAR PRODUCT_ID "/ventilar"
#define MQTT_DATA_START PRODUCT_ID "/start"
#define MQTT_DATA

#endif