#ifndef _BSP_WIFI_PORT_H_
#define _BSP_WIFI_PORT_H_

#include <stdint.h>
#include "esp_wifi_types.h"  // 推荐包含 ESP-IDF 官方 WiFi 类型定义

typedef struct {
    // 硬件初始化，返回0成功
    int (*hw_init)(const char* ssid, const char* password, uint8_t channel, uint8_t max_connection, wifi_auth_mode_t authmode, void **wifi_handle);
    // 硬件反初始化
    int (*hw_deinit)(void* wifi_handle);
    // 连接指定AP
    int (*hw_connect)(void* wifi_handle, const char* ssid, const char* password);
    // 断开连接
    int (*hw_disconnect)(void* wifi_handle);
    // 发送数据
    int (*hw_send)(void* wifi_handle, const uint8_t *data, uint32_t len);
    // 接收数据
    int (*hw_receive)(void* wifi_handle, uint8_t *buf, uint32_t max_len, uint32_t timeout_ms);
} wifi_port_instance_t;

#endif // _BSP_WIFI_PORT_H_