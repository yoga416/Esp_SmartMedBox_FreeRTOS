#ifndef _BSP_UART_PORT_H_
#define _BSP_UART_PORT_H_
#include <stdint.h>
#include "driver/uart.h" // <--- 增加这一行以引入系统宏

#ifndef UART_NUM_MAX

#define UART_NUM_MAX (3) // ESP32/S3 通常有 3 个串口

#endif
//uart硬件操作接口（定义在 bsp_uart_port.h）
// 定义底层硬件操作接口 (Port 层必须实现这些接口)
typedef struct {
    // 硬件初始化，返回 0 成功
    int (*hw_init)(uint8_t port, uint32_t baud_rate, int tx_pin, int rx_pin, uint32_t rx_buf_size, uint32_t tx_buf_size, void **event_queue);
    // 硬件反初始化
    int (*hw_deinit)(uint8_t port);
    // 硬件发送
    int (*hw_write)(uint8_t port, const uint8_t *data, uint32_t len);
    // 硬件接收
    int (*hw_read)(uint8_t port, uint8_t *buf, uint32_t max_len, uint32_t timeout_ms);
    // 获取硬件缓冲区数据量
    uint32_t (*hw_get_rx_len)(uint8_t port);
    // 清空硬件缓冲区
    int (*hw_flush)(uint8_t port);
} uart_port_ops_t;

#endif // _BSP_UART_PORT_H_
