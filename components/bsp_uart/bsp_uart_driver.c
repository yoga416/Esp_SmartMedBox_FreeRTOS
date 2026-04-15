#include "bsp_uart_driver.h"
#include "bsp_uart_reg.h"
#include "bsp_uart_port.h"

#include <stdio.h>
#include <esp_log.h>

#define TAG "bsp_uart_driver"
#include <string.h>
#include <stdint.h>
// UART驱动相关实现

static uart_state_t uart_init(bsp_uart_driver_t *self,
                                    uart_init_config_t *config,
                                    uint32_t rx_buf_size,
                                    uint32_t tx_buf_size)
   {
      //返回值定义
      uart_state_t ret=UART_OK;
      // 1. 配置 UART 硬件参数（波特率、数据位、停止位等）
      if(self == NULL||config == NULL) {
 #ifdef UART_DEBUG_ENABLE
          ESP_LOGI(TAG, "[uart_init] UART init failed: invalid parameters");
 #endif // DEBUG
          return UART_ERROR; // 参数错误
      }
      // 2. 调用底层硬件接口进行初始化，获取事件队列句柄
      ret=self->hw_instance.hw_init(self->uart_port, 
                              config->baud_rate,
                              config->tx_pin,
                              config->rx_pin,
                               rx_buf_size,
                                tx_buf_size,
                                 &(self->event_queue));
        if(ret !=0){
 #ifdef UART_DEBUG_ENABLE
            ESP_LOGI(TAG, "[uart_init] UART init failed: hardware initialization error");
 #endif // DEBUG
            return UART_ERROR; // 硬件初始化失败
        }
    return UART_OK; // 示例返回值
}

static uart_state_t uart_deinit(bsp_uart_driver_t *self) {
    if(self == NULL) {
 #ifdef UART_DEBUG_ENABLE
    ESP_LOGI(TAG, "[uart_deinit] UART deinit failed: invalid parameter");
 #endif // DEBUG
            return UART_ERROR; // 参数错误
      }
      if((self->hw_instance.hw_deinit)(self->uart_port) != 0) {
 #ifdef UART_DEBUG_ENABLE
          ESP_LOGI(TAG, "[uart_deinit] UART deinit failed: driver deletion error");
 #endif // DEBUG
          return UART_ERROR; // 驱动删除错误
      }
      return UART_OK; // 示例返回值
}

static uart_state_t uart_send(bsp_uart_driver_t *self, const uint8_t *data, uint32_t len) {
    if(self == NULL || data == NULL || len == 0) {
 #ifdef UART_DEBUG_ENABLE
    ESP_LOGI(TAG, "[uart_send] UART send failed: invalid parameters");
 #endif // DEBUG
            return UART_ERROR; // 参数错误
      }
      // UART ISR will then move data from the ring buffer to TX FIFO gradually.
      int bytes_written = (self->hw_instance.hw_write)(self->uart_port, data, len);
      if(bytes_written < 0) {
 #ifdef UART_DEBUG_ENABLE
              ESP_LOGI(TAG, "[uart_send] UART send failed: write error");
 #endif // DEBUG
                  return UART_ERROR; // 写入错误
            }
            return UART_OK; // 示例返回值
      }    

static uart_state_t uart_receive(bsp_uart_driver_t *self, uint8_t *out_data, uint32_t max_len, uint32_t timeout_ms) {
    if(self == NULL || out_data == NULL || max_len == 0) {
 #ifdef UART_DEBUG_ENABLE
    ESP_LOGI(TAG, "[uart_receive] UART receive failed: invalid parameters");
 #endif // DEBUG
            return UART_ERROR; // 参数错误
      }
      int bytes_read = (self->hw_instance.hw_read)(self->uart_port, out_data, max_len, timeout_ms);
      if(bytes_read < 0) {
 #ifdef UART_DEBUG_ENABLE
          ESP_LOGI(TAG, "[uart_receive] UART receive failed: read error");
 #endif // DEBUG
              return UART_ERROR; // 读取错误
        }
        return bytes_read; // 返回实际读取的字节数
      }


uint32_t uart_get_rx_buffered_len(bsp_uart_driver_t *self) {
    if(self == NULL) {
 #ifdef UART_DEBUG_ENABLE
    ESP_LOGI(TAG, "[uart_get_rx_buffered_len] UART get buffered length failed: invalid parameter");
 #endif
        return 0; // 参数错误，返回0
    }
    uint32_t buffered_len = self->hw_instance.hw_get_rx_len(self->uart_port);
    
    return buffered_len;
}

uart_state_t bsp_uart_flush(bsp_uart_driver_t *self) {
    if(self == NULL) {
 #ifdef UART_DEBUG_ENABLE
    ESP_LOGI(TAG, "[uart_flush] UART flush failed: invalid parameter");
 #endif // DEBUG
            return UART_ERROR; // 参数错误
      }
      if((self->hw_instance.hw_flush)(self->uart_port) != 0) {
 #ifdef UART_DEBUG_ENABLE
          ESP_LOGI(TAG, "[uart_flush] UART flush failed: error flushing buffer");
 #endif // DEBUG
          return UART_ERROR; // 刷新缓冲区错误
      }
      return UART_OK; // 示例返回值
}     

uart_state_t uart_inst(bsp_uart_driver_t *driver,
                        uart_port_ops_t *port_ops,
                         uint8_t port, 
                         timebase_os_t *time_ops) {
    if(driver == NULL || port_ops == NULL || time_ops == NULL) {
 #ifdef UART_DEBUG_ENABLE
    ESP_LOGI(TAG, "[uart_inst] UART instantiation failed: invalid parameters");
 #endif // DEBUG
            return UART_ERROR; // 参数错误
      }
      driver->uart_port = port;
      driver->hw_instance = *port_ops;
      driver->time_ops = *time_ops;
      driver->init = uart_init;
      driver->deinit = uart_deinit;
      driver->send = uart_send;
      driver->receive = uart_receive;
      driver->get_rx_buffered_len = uart_get_rx_buffered_len;
      driver->flush = bsp_uart_flush;
      return UART_OK; // 示例返回值
}



