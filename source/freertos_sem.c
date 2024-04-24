/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define TASK_PRIO          (configMAX_PRIORITIES - 1)
#define CONSUMER_LINE_SIZE 3

#define SIZE_RING_BUFF 5
#define SIZE_RING_BUFF_DATA 11

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void producer_task(void);
static void consumer_task(void);
void ring_buff_write(void);
void ring_buff_read(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/
char ring_buff[SIZE_RING_BUFF];
uint8_t consumer_counter;
uint8_t producer_counter;
bool data_complete = false;
char data_array[] = {'I', 'T', 'E', 'S', 'O', ' ', 'R', 'U', 'L', 'E', 'S'};
uint8_t index_data_array;

SemaphoreHandle_t xSemaphore_producer;
SemaphoreHandle_t xSemaphore_consumer;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Main function
 */
int main(void)
{
    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    
    if (xTaskCreate(producer_task, "PRODUCER_TASK", configMINIMAL_STACK_SIZE + 128, NULL, TASK_PRIO, NULL) != pdPASS)
    {
        PRINTF("Task creation failed!.\r\n");
        while (1)
            ;
    }
    /* Start scheduling. */
    vTaskStartScheduler();
    for (;;)
        ;
}

/*!
 * @brief Task producer_task.
 */
static void producer_task(void)
{
    PRINTF("Producer_task created.\r\n");

    xSemaphore_producer = xSemaphoreCreateCounting(SIZE_RING_BUFF, SIZE_RING_BUFF);
    if (xSemaphore_producer == NULL)
    {
        PRINTF("xSemaphore_producer creation failed.\r\n");
        vTaskSuspend(NULL);
    }

    xSemaphore_consumer = xSemaphoreCreateCounting(SIZE_RING_BUFF,0);
    if (xSemaphore_consumer == NULL)
    {
        PRINTF("xSemaphore_consumer creation failed.\r\n");
        vTaskSuspend(NULL);
    }

    if (xTaskCreate(consumer_task, "CONSUMER_TASK", configMINIMAL_STACK_SIZE + 128, /*(void *)i*/ 0, TASK_PRIO, NULL) !=
        pdPASS)
    {
        PRINTF("Task creation failed!.\r\n");
        vTaskSuspend(NULL);
    }
    else
    {
        PRINTF("Consumer_task created.\r\n"/*, i*/);
    }

    while (!data_complete) {
        xSemaphoreGive(xSemaphore_consumer);
        if (xSemaphoreTake(xSemaphore_producer, portMAX_DELAY) == pdTRUE) {
            ring_buff_write();
        }
    }
    PRINTF("Producer has completed writing data.\r\n");
    vTaskDelete(NULL);
}

/*!
 * @brief Task consumer_task.
 */
static void consumer_task(void )
{
    while (!data_complete) {
        xSemaphoreGive(xSemaphore_producer);
        if (xSemaphoreTake(xSemaphore_consumer, portMAX_DELAY) == pdTRUE) {
            ring_buff_read();
        }
    }
    PRINTF("\r\nConsumer has completed reading data.\r\n");
    vTaskDelete(NULL);
}

void ring_buff_write() {
    if (index_data_array < SIZE_RING_BUFF_DATA) {

        //data array to ring buff
        ring_buff[producer_counter] = data_array[index_data_array];

        //update the prducer counter
        producer_counter = (producer_counter + 1) % SIZE_RING_BUFF;

        //update data array index
        index_data_array++;
    }
}

void ring_buff_read() {
    
    PRINTF("%c", ring_buff[consumer_counter]);

    // Update the consumer index
    consumer_counter = (consumer_counter + 1) % SIZE_RING_BUFF;

    // Check if it's the end of the data array
    if ( (SIZE_RING_BUFF_DATA == index_data_array) & (SIZE_RING_BUFF - 1 == consumer_counter)) {
        data_complete = true;
    }
}
