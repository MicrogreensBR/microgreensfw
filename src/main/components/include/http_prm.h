/**
 * @file http_prm.h
 * @author Henrique Sander Lourenço
 * @brief
 * @version 0.1
 * @date 2023-05-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef HTTP_PRM_H
#define HTTP_PRM_H

#define PRODUCT_ID 789
#define MAX_ID_DIGITS 3
#define HTTP_CONFIG_HOST "igbt.eesc.usp.br"
#define HTTP_DEFAUlT_BASE_PATH "/app-15/add/esp/"
#define HTTP_FIRST_TEST_URL "https://httpbin.org/get"
// #define HTTP_APPLICATION_JSON "application/json"
#define HTTP_SEND_DATA_BASE_URL "http://igbt.eesc.usp.br/app-15/get/"
#define HTTP_ADD_ID_BASE_URL "http://igbt.eesc.usp.br/app-15/add/esp/"
#define HTTP_MAX_PATH_LEN 128
#define HTTP_TASK_DELAY_MS 1000
#define HTTP_TEST_DELAY_MS 5000
#define HTTP_INIT_RETRY_DELAY_MS 5000
#define HTTP_TIMEOUT_MS 5000

#endif