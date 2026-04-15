#include "bsp_os_layer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* --- 时间/延时适配实现 --- */
static uint32_t os_get_counter(void) {
    return (uint32_t)xTaskGetTickCount();
}

static void os_delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

timebase_os_t os_time_ops = {
    .pfget_count = os_get_counter,
    .pfdelay_ms    = os_delay_ms
};

/* --- 队列适配实现 --- */
static uart_handler_state_t os_queue_create(void **queue_handle, uint32_t len, uint32_t size) {
    *queue_handle = xQueueCreate(len, size);
    return (*queue_handle != NULL) ? UART_HANDLER_OK : UART_HANDLER_ERROR;
}

static uart_handler_state_t os_queue_put(void *handle, const void *item, uint32_t timeout) {
    if (xQueueSend((QueueHandle_t)handle, item, pdMS_TO_TICKS(timeout)) == pdTRUE) {
        return UART_HANDLER_OK;
    }
    return UART_HANDLER_TIMEOUT;
}

static uart_handler_state_t os_queue_pop(void *handle, void *buffer, uint32_t timeout) {
    if (xQueueReceive((QueueHandle_t)handle, buffer, pdMS_TO_TICKS(timeout)) == pdTRUE) {
        return UART_HANDLER_OK;
    }
    return UART_HANDLER_TIMEOUT;
}

uart_queue_ops_t os_queue_ops = {
    .pfqueue_create  = os_queue_create,
    .pfqueue_send    = os_queue_put,
    .pfqueue_receive = os_queue_pop
};