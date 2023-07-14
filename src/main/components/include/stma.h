/**
 * @file stma.h
 * @author Henrique Sander Lourenco
 * @brief Header file for stma.c
 * @version 0.1
 * @date 2023-05-23
 *
 * @copyright Copyright (c) 2023 MicrogreensBR
 *
 */

#ifndef STMA_H
#define STMA_H

#include "esp_system.h"

typedef enum
{
    st_initialize,
    st_working,
    st_broken,
    st_wait,
} states_t;

typedef enum
{
    SOAKING_TIMER,
    GROWTH_TIMER,
} timer_mode_t;

extern states_t st_next;

void Stma__InitiateTimer(int64_t time_s, timer_mode_t mode);
void Stma__Task(void *arg);

#endif