#ifndef _UART_H_
#define _UART_H_

#include <stdint.h>

// 初始化串口环境
void app_uart_init(void);

// 发送字符串数据
void app_uart_send_string(const char* data);

// 发送十六进制数组数据
void app_uart_send_data(const uint8_t* data, int len);

// FreeRTOS 串口接收监听任务
void app_uart_receive_task(void *pvParameters);

#endif /* _UART_H_ */