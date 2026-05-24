#include "bsp_wifi_driver.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

int mock_hw_init_call_count = 0;
int mock_hw_send_call_count = 0;
int mock_hw_connect_call_count = 0;
int mock_hw_receive_call_count = 0;

int mock_hw_init(const char* ssid, const char* password, int channel, int max_connection, int authmode, void **wifi_handle) {
    mock_hw_init_call_count++;
    *wifi_handle = (void*)0x1234;
    return WIFI_DRIVER_OK;
}
int mock_hw_deinit(void* wifi_handle) { return WIFI_DRIVER_OK; }
int mock_hw_connect(void* wifi_handle, const char* ssid, const char* password) { mock_hw_connect_call_count++; return WIFI_DRIVER_OK; }
int mock_hw_disconnect(void* wifi_handle) { return WIFI_DRIVER_OK; }
int mock_hw_send(void* wifi_handle, const uint8_t *data, uint32_t len) { mock_hw_send_call_count++; return WIFI_DRIVER_OK; }
int mock_hw_receive(void* wifi_handle, uint8_t *buf, uint32_t max_len, uint32_t timeout_ms) {
    mock_hw_receive_call_count++;
    if (max_len > 0) buf[0] = 0xAA;
    return 1;
}

wifi_port_ops_t mock_wifi_port_ops = {
    .hw_init = mock_hw_init,
    .hw_deinit = mock_hw_deinit,
    .hw_connect = mock_hw_connect,
    .hw_disconnect = mock_hw_disconnect,
    .hw_send = mock_hw_send,
    .hw_receive = mock_hw_receive
};
