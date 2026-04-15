#include "bsp_uart_port.h"
#include <string.h>
#include <stdint.h>
// --- 定义 Mock 全局状态变量 (注意：千万不能加 static) ---
int mock_hw_init_call_count = 0;
int mock_hw_write_call_count = 0;
uint32_t last_configured_baud = 0;

// 1. 模拟硬件初始化
int mock_esp32_hw_init(uint8_t port, uint32_t baud, int tx, int rx, 
                       uint32_t rx_size, uint32_t tx_size, void **event_q) {
    mock_hw_init_call_count++;
    last_configured_baud = baud;
    static int fake_queue = 0xAAAA;
    if (event_q) *event_q = &fake_queue;
    return 0; 
}

// 2. 模拟硬件发送
int mock_esp32_hw_write(uint8_t port, const uint8_t *data, uint32_t len) {
    mock_hw_write_call_count++;
    return len; 
}

// 其他函数简单实现
int mock_esp32_hw_read(uint8_t port, uint8_t *buf, uint32_t max, uint32_t ms) { return 0; }
int mock_esp32_hw_deinit(uint8_t port) { return 0; }
uint32_t mock_esp32_hw_get_len(uint8_t port) { return 10; } // 对应测试里的 10
int mock_esp32_hw_flush(uint8_t port) { return 0; }

// 3. 组装接口对象 (注意：名字必须和 extern 声明的一模一样)
uart_port_ops_t esp32_uart_port_ops = {
    .hw_init   = mock_esp32_hw_init,
    .hw_write  = mock_esp32_hw_write,
    .hw_read   = mock_esp32_hw_read,
    .hw_deinit = mock_esp32_hw_deinit,
    .hw_get_rx_len = mock_esp32_hw_get_len,
    .hw_flush  = mock_esp32_hw_flush
};