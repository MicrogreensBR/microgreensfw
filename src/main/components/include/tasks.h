/**
 * @file tasks.h
 * @author Henrique Sander Lourenco (henriquesander27@gmail.com)
 * @brief Header file for tasks.c
 * @version 0.1
 * @date 2023-05-23
 *
 * @copyright Copyright (c) 2023 MicrogreensBR
 *
 */

#ifndef TASKS_H
#define TASKS_H

typedef enum
{
    th_stma,
    th_hum_sens,
    th_temp_sens,

    th_size,
} task_handles_t;

extern TaskHandle_t tasks_handles[th_size];

void Tasks__CreateAll(void);

#endif