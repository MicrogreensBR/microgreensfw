/**
 * @file http.h
 * @author Henrique Sander Lourenço
 * @brief
 * @version 0.1
 * @date 2023-05-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef HTTP_H
#define HTTP_H

typedef enum
{
    http_uninitialized,
    http_idle,
} http_status_t;

void Http__Task(void *arg);

#endif