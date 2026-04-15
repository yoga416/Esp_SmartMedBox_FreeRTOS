#ifndef _BSP_UART_HANDLER_H_
#define _BSP_UART_HANDLER_H_

// 1. 包含必要的头文件
#include "bsp_uart_driver.h"
#include "bsp_uart_reg.h"

#include <stdio.h>
#include <stdint.h>     
#include <string.h>


//define 
#define HANDLER_DEBUG_ENABLE /*printf是否使能*/

//uarthandler 状态枚举
typedef enum {
    UART_HANDLER_OK = 0,                  // 操作成功
    UART_HANDLER_ERROR = -1,              // 通用错误
    UART_HANDLER_TIMEOUT = -2,            // 操作超时
    UART_HANDLER_PARSE_ERR = -3,          // 帧解析错误
    UART_ERR_NULL_PTR = -4,               // 空指针错误
    UART_HANDLER_FRAME_INCOMPLETE = -5,   // 帧数据不完整
    ERR_INVALID_CALLBACK = -6,            // 回调函数无效或未注册
    ERR_NOT_IMPLEMENTED = -7              // 功能未实现
} uart_handler_state_t;   

typedef struct {
    uint8_t sensor_id;        // 设备类型标识
    uint16_t data_len;  // 有效载荷(Payload)长度
    uint8_t *payload;   // 指向有效载荷的指针
} uart_parsed_frame_t;

typedef void (*uart_frame_parsed_cb_t)(const uart_parsed_frame_t *frame);

typedef struct {
   uint32_t (*pfget_count)(void);

   uart_handler_state_t (*pfqueue_create)(void **queue_handle,
                                           uint32_t queue_length,
                                            uint32_t item_size);

   uart_handler_state_t (*pfqueue_send)(void *queue_handle,
                                           const void *item,
                                            uint32_t timeout_ms);

   uart_handler_state_t (*pfqueue_receive)(void *queue_handle,
                                                 void *buffer,
                                                  uint32_t timeout_ms);

   uart_handler_state_t (*pfqueue_delete)(void *queue_handle);
   
} uart_queue_ops_t;

typedef struct bsp_uart_handler_t bsp_uart_handler_t;

typedef struct{
      timebase_os_t *time_ops;  // OS 接口
      bsp_uart_driver_t *uart_driver; // UART 驱动实例
      uart_queue_ops_t *queue_ops;   // 队列操作接口
}uart_handler_input_instance_t;

struct bsp_uart_handler_t{
        bsp_uart_driver_t *uart_driver; // UART 驱动实例
      uart_queue_ops_t *queue_ops;   // 队列操作接口
      timebase_os_t *time_ops;  // OS 接口

      void *event_queue_handle; // 事件队列句柄

      uart_frame_parsed_cb_t frame_parsed_callback; // 帧解析完成回调函数

      uint8_t rx_buffer[256]; 
      uint16_t rx_index; 
      uint16_t parse_state; 

        uart_handler_state_t (*init)(bsp_uart_handler_t *self, uart_handler_input_instance_t *input_instance);
        uart_handler_state_t (*deinit)(bsp_uart_handler_t *self);
        uart_handler_state_t (*register_rx_callback)(bsp_uart_handler_t *self, uart_frame_parsed_cb_t callback);
        uart_handler_state_t (*send_frame)(bsp_uart_handler_t *self, uint8_t sensor_id, const uint8_t *data, uint16_t data_len);
};

// API 函数声明
uart_handler_state_t uart_handler_inst(
    bsp_uart_handler_t *handler_instance,
    uart_handler_input_instance_t *input_instance
);


//线程入口函数
void uart_handler_task(void *argument);


#endif // _BSP_UART_HANDLER_H_
