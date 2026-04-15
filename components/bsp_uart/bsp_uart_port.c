#include "bsp_uart_port.h"
#include "bsp_uart_driver.h"
#include "bsp_uart_reg.h"


#include <stdio.h>
#include <esp_log.h>

#define TAG "bsp_uart_port"
#include <stdint.h>
#include <string.h>

#include "driver/uart.h"


//顶层驱动的实现

 static uart_state_t hardwares_uart_init(uint8_t port,
                                           uint32_t baud_rate,
                                            int tx_pin,
                                             int rx_pin,
                                              uint32_t rx_buf_size,
                                               uint32_t tx_buf_size,
                                                void **event_queue) {

//底层驱动函数的实现
//检查参数合法性
    if (baud_rate == 0 || event_queue == NULL) {
 #ifdef UART_DEBUG_ENABLE
            ESP_LOGI(TAG, "[hardwares_uart_init] UART hardware init failed: invalid parameters");
 #endif // DEBUG
            return UART_ERROR; // 参数错误
    }
      // 1. 配置 UART 硬件参数（波特率、数据位、停止位等）
      uart_config_t uart_config = {
            .baud_rate = baud_rate,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
            };

      // 2. 调用底层硬件接口进行初始化，获取事件队列句柄
      if (uart_param_config(port, &uart_config) != ESP_OK) {
 #ifdef UART_DEBUG_ENABLE
              ESP_LOGI(TAG, "[hardwares_uart_init] UART hardware init failed: parameter configuration error");
 #endif // DEBUG
              return UART_ERROR; // 参数配置错误
      }

      // 设置 UART 的引脚
      if (uart_set_pin(port, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
 #ifdef UART_DEBUG_ENABLE
                  ESP_LOGI(TAG, "[hardwares_uart_init] UART hardware init failed: pin configuration error");
 #endif // DEBUG
                  return UART_ERROR; // 引脚配置错误
      }     

      uint32_t uart_queue_size = 20; // 事件队列大小
      // 安装 UART 驱动，分配 DMA 缓冲区
      if (uart_driver_install(port, rx_buf_size, tx_buf_size, uart_queue_size, (QueueHandle_t *)event_queue, 0) != ESP_OK) {
 #ifdef UART_DEBUG_ENABLE
                  ESP_LOGI(TAG, "[hardwares_uart_init] UART hardware init failed: driver installation error");
 #endif // DEBUG
                  return UART_ERROR; // 驱动安装错误

      }
      
      return UART_OK; // 初始化成功
      }

static uart_state_t hardwares_uart_deinit(uint8_t port) {
      //参数检查
    if (port >= UART_NUM_MAX) {
        return UART_ERROR; // 无效的 UART 端口号
    }

    if (uart_driver_delete(port) != ESP_OK) {
 #ifdef UART_DEBUG_ENABLE
      ESP_LOGI(TAG, "[hardwares_uart_deinit] UART hardware deinit failed: driver deletion error"); 
 #endif // DEBUG
        return UART_ERROR; // 驱动删除错误
    }
    return UART_OK; // 示例返回值
}     

static int  hardwares_uart_write(uint8_t port, const uint8_t *data, uint32_t len) {
    if (port >= UART_NUM_MAX || data == NULL || len == 0) {
 #ifdef UART_DEBUG_ENABLE
      ESP_LOGI(TAG, "[hardwares_uart_write] UART hardware write failed: invalid parameters");      
 #endif // DEBUG
            return UART_ERROR; // 参数错误
      }
      int bytes_written = uart_write_bytes(port, (const char *)data, len);
      if (bytes_written < 0) {
 #ifdef UART_DEBUG_ENABLE
              ESP_LOGI(TAG, "[hardwares_uart_write] UART hardware write failed: write error");
 #endif // DEBUG
              return UART_ERROR; // 写入错误
      }
      return bytes_written; // 返回实际写入的字节数
}

static int hardwares_uart_read(uint8_t port, uint8_t *buf, uint32_t max_len, uint32_t timeout_ms) {
    if (port >= UART_NUM_MAX || buf == NULL || max_len == 0) {
 #ifdef UART_DEBUG_ENABLE
      ESP_LOGI(TAG, "[hardwares_uart_read] UART hardware read failed: invalid parameters");
 #endif // DEBUG
            return UART_ERROR; // 参数错误
      }
      int bytes_read = uart_read_bytes(port, (uint8_t *)buf, max_len, timeout_ms / portTICK_PERIOD_MS);
      if (bytes_read < 0) {
 #ifdef UART_DEBUG_ENABLE
              ESP_LOGI(TAG, "[hardwares_uart_read] UART hardware read failed: read error");
 #endif // DEBUG
              return UART_ERROR; // 读取错误
      }
      return bytes_read; // 返回实际读取的字节数
}

static uint32_t hardwares_uart_get_rx_len(uint8_t port) {
    if (port >= UART_NUM_MAX) {
        return 0; // 无效的 UART 端口号，返回 0
    }
    size_t buffered_len = 0;
    if (uart_get_buffered_data_len(port, &buffered_len) != ESP_OK) {
 #ifdef UART_DEBUG_ENABLE
      ESP_LOGI(TAG, "[hardwares_uart_get_rx_len] UART hardware get RX length failed: error");
 #endif // DEBUG
            return 0; // 获取长度失败，返回 0
      }
      return buffered_len; // 返回当前 RX 缓冲区中的数据长度
      }

static int hardwares_uart_flush(uint8_t port) {
      if (port >= UART_NUM_MAX) {
            return UART_ERROR; // 无效的 UART 端口号
      }
      if (uart_flush_input(port) != ESP_OK) {
 #ifdef UART_DEBUG_ENABLE
              ESP_LOGI(TAG, "[hardwares_uart_flush] UART hardware flush failed: error"); 
 #endif // DEBUG
              return UART_ERROR; // 刷新失败
      }
      return UART_OK; // 刷新成功
}


//挂载底层硬件接口到驱动对象
uart_port_ops_t esp32_uart_port_ops = {
    .hw_init = hardwares_uart_init,
    .hw_deinit = hardwares_uart_deinit,
    .hw_write = hardwares_uart_write,
    .hw_read = hardwares_uart_read,
    .hw_get_rx_len = hardwares_uart_get_rx_len,
    .hw_flush = hardwares_uart_flush
};

