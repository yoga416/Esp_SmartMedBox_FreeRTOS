#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

// 引入模块自己的头文件
#include "uart.h"
#include "uart_reg.h"
#include "sensor_parser.h"

static const char *TAG = "UART_MODULE";

/**
 * @brief 初始化 UART 配置与引脚
 */
void app_uart_init(void) {
    // 1. 配置 UART 基本参数 (波特率、数据位、停止位、校验位)
    uart_config_t uart_config = {
        .baud_rate = MY_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // 2. 将配置应用到指定串口
    ESP_ERROR_CHECK(uart_param_config(MY_UART_PORT_NUM, &uart_config));

    // 3. 设置 UART 引脚
    ESP_ERROR_CHECK(uart_set_pin(MY_UART_PORT_NUM, MY_UART_TXD_PIN, MY_UART_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // 4. 安装 UART 驱动程序 (分配接收/发送缓冲区，不使用事件队列则传 NULL)
    ESP_ERROR_CHECK(uart_driver_install(MY_UART_PORT_NUM, MY_UART_RX_BUF_SIZE * 2, MY_UART_TX_BUF_SIZE, 0, NULL, 0));
    
    ESP_LOGI(TAG, "UART%d 初始化成功! TX:IO%d, RX:IO%d", MY_UART_PORT_NUM, MY_UART_TXD_PIN, MY_UART_RXD_PIN);
}

/**
 * @brief 发送字符串
 */
void app_uart_send_string(const char* data) {
    const int len = strlen(data);
    uart_write_bytes(MY_UART_PORT_NUM, data, len);
}

/**
 * @brief 发送指定长度的数据(适用十六进制指令)
 */
void app_uart_send_data(const uint8_t* data, int len) {
    uart_write_bytes(MY_UART_PORT_NUM, (const char*)data, len);
}

/*
 * @brief FreeRTOS 串口接收任务
 */

void app_uart_receive_task(void *pvParameters) {
    uint8_t *data = (uint8_t *) malloc(MY_UART_RX_BUF_SIZE);
    
    if (data == NULL) {
        ESP_LOGE(TAG, "RX Buffer malloc failed");
        vTaskDelete(NULL);
    }

    while (1) {
        // 读取串口数据
        int rxBytes = uart_read_bytes(MY_UART_PORT_NUM, data, MY_UART_RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        
        if (rxBytes > 0) {
            // 将接收到的裸数据流交给协议解析函数处理
            parse_sensor_frame(data, rxBytes);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    free(data);
    vTaskDelete(NULL);
}