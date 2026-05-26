#ifndef _UART_REG_H_
#define _UART_REG_H_

// ==========================================
// 🚀 UART 硬件配置参数 (ESP32-S3)
// ==========================================

// 选择使用的串口号 (UART_NUM_0 通常被系统日志占用，建议外设使用 UART_NUM_1 或 2)
#define MY_UART_PORT_NUM      UART_NUM_1

// 串口波特率
#define MY_UART_BAUD_RATE     115200

// 硬件引脚定义 (ESP32-S3 支持引脚矩阵，可以映射到大部分 GPIO)
#define MY_UART_TXD_PIN       (GPIO_NUM_17)
#define MY_UART_RXD_PIN       (GPIO_NUM_18)

// 缓冲区大小定义 (FreeRTOS 机制要求 Rx 必须大于硬件 FIFO 尺寸)
#define MY_UART_RX_BUF_SIZE   (1024)
#define MY_UART_TX_BUF_SIZE   (0)      // TX 不使用 RingBuffer 时设为 0 即可

#endif /* _UART_REG_H_ */