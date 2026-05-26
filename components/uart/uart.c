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

/**
 * @brief 向发送时间同步包到下位机
 * 格式: [5A] [LEN=6] [ID=10] [YY] [MM] [DD] [HH] [MM] [SS] [CRC] [FF]
 */
void app_uart_send_time(int year, int mon, int mday, int hour, int min, int sec) {
    uint8_t frame[11];
    frame[0] = 0x5A;             // 帧头
    frame[1] = 6;                // Payload 长度 (YY,MM,DD,HH,MM,SS)
    frame[2] = CMD_RTC_SYNC;     // 命令 ID
    frame[3] = (uint8_t)(year % 100); // 年 (取后两位)
    frame[4] = (uint8_t)mon;
    frame[5] = (uint8_t)mday;
    frame[6] = (uint8_t)hour;
    frame[7] = (uint8_t)min;
    frame[8] = (uint8_t)sec;
    
    // 计算 CRC (从 LEN 字段到 Payload 结束)
    frame[9] = calc_crc8(&frame[1], 1 + 1 + frame[1]); 
    frame[10] = 0xFF;            // 帧尾

    app_uart_send_data(frame, 11);
    // ESP_LOGI(TAG, "已发送时间同步包: %02d-%02d-%02d %02d:%02d:%02d", 
    //          frame[3], frame[4], frame[5], frame[6], frame[7], frame[8]);
   
}
/**
 * @brief 发送天气数据到下位机
 * 格式: [5A] [LEN=2] [ID=11] [CODE] [TEMP] [CRC] [FF]
 */
void app_uart_send_weather(int weather_code, int temp) {
    uint8_t frame[7];
    frame[0] = 0x5A;
    frame[1] = 2;                // Payload: Code 和 Temp
    frame[2] = CMD_WEATHER_SYNC;
    frame[3] = (uint8_t)weather_code;
    frame[4] = (uint8_t)temp;
    frame[5] = calc_crc8(&frame[1], 4); // LEN+ID+Data
    frame[6] = 0xFF;

    app_uart_send_data(frame, 7);
    ESP_LOGI(TAG, "已发送天气同步包: 现象代码=%d, 温度=%dC", weather_code, temp);
   
}

/**
 * @brief 发送位置数据到下位机 (城市名称)
 * 格式: [5A] [LEN] [ID=12] [City String...] [CRC] [FF]
 */
void app_uart_send_location(const char *city_name) {
    if (city_name == NULL) return;
    
    int str_len = strlen(city_name);
    int total_len = str_len + 5; // HEAD, LEN, ID, DATA..., CRC, TAIL
    uint8_t *frame = malloc(total_len);
    if (frame == NULL) return;
    
    frame[0] = 0x5A;
    frame[1] = (uint8_t)str_len;
    frame[2] = CMD_LOCATION_SYNC;
    memcpy(&frame[3], city_name, str_len);
    frame[total_len - 2] = calc_crc8(&frame[1], str_len + 2);
    frame[total_len - 1] = 0xFF;

    /*frame格式: [5A] [LEN] [ID] [City String...] [CRC] [FF]*/
    app_uart_send_data(frame, total_len);
    ESP_LOGI(TAG, "已发送位置同步包: 城市=%s", city_name);
    
    free(frame);
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

    for (;;) {
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