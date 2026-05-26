#ifndef _UART_H_
#define _UART_H_

#include <stdint.h>

// 初始化串口环境
void app_uart_init(void);

// 发送字符串数据
void app_uart_send_string(const char* data);

// 发送十六进制数组数据
void app_uart_send_data(const uint8_t* data, int len);

// 向下位机发送同步时间
void app_uart_send_time(int year, int mon, int mday, int hour, int min, int sec);

// 向下位机发送天气同步信息 (天气现象代码, 温度)
void app_uart_send_weather(int weather_code, int temp);

// 向下位机发送地理位置同步信息 (城市名称字符串)
void app_uart_send_location(const char *city_name);

// FreeRTOS 串口接收监听任务
void app_uart_receive_task(void *pvParameters);

#endif /* _UART_H_ */