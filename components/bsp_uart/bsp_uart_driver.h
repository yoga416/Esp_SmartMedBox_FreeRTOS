#ifndef _BSP_UART_DRIVER_H_
#define _BSP_UART_DRIVER_H_

#include "bsp_uart_port.h"
#include "bsp_uart_reg.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// 1. 声明底层用到的句柄类型 (解耦用)
typedef struct bsp_uart_driver_t bsp_uart_driver_t;


// UART 的初始化配置结构体
typedef struct {
     int baud_rate;                      /*!< UART baud rate*/
     int data_bits; 
     int tx_pin;
     int rx_pin; 
     int parity;               /*!< UART parity mode*/
     int stop_bits;         /*!< UART stop bits*/
     int flow_ctrl;    /*!< UART HW flow control mode (cts/rts)*/
     uint8_t rx_flow_ctrl_thresh;        /*!< UART HW RTS threshold*/
     int source_clk;             /*!< UART source clock selection */
      } uart_init_config_t;

// 时间基准适配结构体
typedef struct { 
    uint32_t (*pfget_count)(void); 
    void (*pfdelay_ms)(uint32_t ms);   
} timebase_os_t;

// UART状态枚举
typedef enum {
    UART_OK = 0,
    UART_ERROR = -1,
    UART_TIMEOUT = -2
} uart_state_t;


// 2. 驱动类结构体定义
struct bsp_uart_driver_t {
    /* 属性区 */
    uint8_t  uart_port;      // 物理端口号：0, 1, 2
    void* event_queue;    // 存储 ESP-IDF 的 QueueHandle_t
    timebase_os_t time_ops;  // OS 接口
    uart_port_ops_t hw_instance;  // 硬件操作接口
    /* 方法区 */
    // 初始化：在 ESP32 上负责调用 uart_driver_install 分配 DMA 缓冲区
    uart_state_t (*init)(bsp_uart_driver_t *self,
                        uart_init_config_t *config, 
                        uint32_t rx_buf_size, 
                        uint32_t tx_buf_size); 
    
    uart_state_t (*deinit)(bsp_uart_driver_t *self);
    
    // 发送：调用 uart_write_bytes，底层自动走 DMA
    uart_state_t (*send)(bsp_uart_driver_t *self, const uint8_t *data, uint32_t len);

    // 接收：从底层的环形缓冲区读取数据
    // 返回 int：正数为实际读到长度，负数为错误码
    int (*receive)(bsp_uart_driver_t *self, uint8_t *out_data, uint32_t max_len, uint32_t timeout_ms);

    // 获取缓冲区当前已有的数据量
    uint32_t (*get_rx_buffered_len)(bsp_uart_driver_t *self);
    
    // 清空接收缓冲区
    uart_state_t (*flush)(bsp_uart_driver_t *self);
};

/* 构造函数：初始化驱动对象并关联 OS 接口 */
uart_state_t uart_inst(bsp_uart_driver_t *driver,
                        uart_port_ops_t *port_ops,
                         uint8_t port, 
                         timebase_os_t *time_ops);

#endif // _BSP_UART_DRIVER_H_