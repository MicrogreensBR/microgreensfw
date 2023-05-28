/**
 * @file web_server.h
 * @author Henrique Sander Lourenço
 * @brief
 * @version 0.1
 * @date 2023-05-07
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_http_server.h"

httpd_handle_t WebServer__Initialize(void);

#endif