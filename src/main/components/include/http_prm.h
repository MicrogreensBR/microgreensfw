/**
 * @file http_prm.h
 * @author Henrique Sander Lourenço
 * @brief
 * @version 0.1
 * @date 2023-05-03
 *
 * @copyright Copyright (c) 2023 MicrogreensBR
 *
 */

#ifndef HTTP_PRM_H
#define HTTP_PRM_H

#include "mqtt_prm.h"

// #define PRODUCT_ID 123
#define MAX_ID_DIGITS 3
#define HTTP_FIRST_TEST_HOST "microgreens.igbt.eesc.usp.br"
#define HTTP_ADD_ESP_BASE_PATH "/add/esp/"
#define HTTP_FIRST_TEST_PATH HTTP_ADD_ESP_BASE_PATH PRODUCT_ID
#define HTTP_MAX_PATH_LEN 128
#define HTTP_TASK_DELAY_MS 1000
#define HTTP_TEST_DELAY_MS 5000
#define HTTP_INIT_RETRY_DELAY_MS 3000
#define HTTP_TIMEOUT_MS 10000
#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

#endif