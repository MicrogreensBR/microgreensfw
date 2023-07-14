/**
 * @file http.h
 * @author Henrique Sander Lourenço
 * @brief
 * @version 0.1
 * @date 2023-05-03
 *
 * @copyright Copyright (c) 2023 MicrogreensBR
 *
 */

#ifndef HTTP_H
#define HTTP_H

esp_err_t Http__Initialize(void);
void Http__Task(void *arg);

#endif