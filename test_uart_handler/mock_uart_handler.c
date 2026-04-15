// 文件路径: test_uart_handler/mock_uart_handler.c
#include "bsp_uart_handler.h"
#include <stdlib.h>
#include <string.h>

static uint32_t mock_item_count = 0;

static uint32_t mock_get_count(void) { return mock_item_count; }

static uart_handler_state_t mock_queue_create(void **queue_handle, uint32_t length, uint32_t item_size) {
    *queue_handle = (void*)0x12345678; 
    return UART_HANDLER_OK;
}

static uart_handler_state_t mock_queue_send(void *queue_handle, const void *item, uint32_t timeout_ms) {
    mock_item_count++;
    return UART_HANDLER_OK;
}

static uart_handler_state_t mock_queue_receive(void *queue_handle, void *buffer, uint32_t timeout_ms) {
    if (mock_item_count > 0) {
        mock_item_count--;
        int dummy_event = 1; 
        memcpy(buffer, &dummy_event, sizeof(int));
        return UART_HANDLER_OK;
    }
    return UART_HANDLER_TIMEOUT;
}

static uart_handler_state_t mock_queue_delete(void *queue_handle) {
    mock_item_count = 0;
    return UART_HANDLER_OK;
}

static uint32_t mock_get_counter(void) { return 0; }
static void mock_delay_ms(uint32_t ms) {}
static void mock_delay_us(uint32_t us) {}

// 暴露给 Handler 的接口实例
uart_queue_ops_t mock_queue_ops = {
    .pfget_count = mock_get_count,
    .pfqueue_create = mock_queue_create,
    .pfqueue_send = mock_queue_send,
    .pfqueue_receive = mock_queue_receive,
    .pfqueue_delete = mock_queue_delete
};

timebase_os_t mock_time_ops = {
    .pfget_count = mock_get_counter,
    .pfdelay_ms = mock_delay_ms
};