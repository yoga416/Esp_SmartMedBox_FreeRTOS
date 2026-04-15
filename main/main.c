#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// 引入你的 BSP 组件头文件
#include "bsp_uart_handler.h"
#include "bsp_uart_driver.h"
#include "bsp_os_layer.h"
#include "bsp_uart_port.h"
#include "bsp_uart_reg.h"


static const char *TAG = "APP_MAIN";

// 1. 定义全局实例
static bsp_uart_handler_t g_uart_handler;
static bsp_uart_driver_t  g_uart_driver;

// 引入底层端口硬件操作接口（定义在 bsp_uart_port.c 中）
extern uart_port_ops_t esp32_uart_port_ops;

// 2. 定义回调函数：当收到上位机发出的完整合法数据包时，此函数会被触发
void on_uart_packet_received(const uart_parsed_frame_t *frame) {
    ESP_LOGI(TAG, "成功解析数据帧! Sensor ID: 0x%02X, 数据长度: %d", 
             frame->sensor_id, frame->data_len);
    if(frame->sensor_id==0x01 && frame->data_len >= 4)
    {
       // 1. 拼接十六进制字节
// 假设 payload[0] 是高位 (0x09)，payload[1] 是低位 (0x29)
// 使用位移操作：(高位 << 8) | 低位
int16_t raw_temp = (int16_t)((uint8_t)frame->payload[0] << 8 | (uint8_t)frame->payload[1]);
int16_t raw_humi = (int16_t)((uint8_t)frame->payload[2] << 8 | (uint8_t)frame->payload[3]);

// 2. 还原回带小数的十进制
// 此时 2345 / 100.0f = 23.45
float temp = raw_temp / 100.0f;
float humi = raw_humi / 100.0f;

// 3. 输出日志
ESP_LOGI(TAG, "温度: %.2f°C, 湿度: %.2f%%", temp, humi);
ESP_LOGI(TAG, "原始HEX: %02X %02X %02X %02X", 
         frame->payload[0], frame->payload[1], 
         frame->payload[2], frame->payload[3]);
    }
    if (frame->data_len > 0) {
        // 打印接收到的十六进制数据
        esp_log_buffer_hex(TAG, frame->payload, frame->data_len);
    }
}

// 封装UART及Handler初始化流程
static bool init_uart_and_handler(void)
{
    ESP_LOGI(TAG, "系统启动中...");

    // 3. 实例化底层的 UART 驱动
    if (uart_inst(&g_uart_driver, &esp32_uart_port_ops, 1, &os_time_ops) != UART_OK) {
        ESP_LOGE(TAG, "UART 驱动实例化失败!");
        return false;
    }

    // 调用真正的硬件初始化来安装底层驱动（应用引脚和波特率等）
    uart_init_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .tx_pin    = UART_TX_PIN,
        .rx_pin    = UART_RX_PIN
    };
    if (g_uart_driver.init(&g_uart_driver, &uart_config, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE) != UART_OK) {
        ESP_LOGE(TAG, "UART 硬件初始化失败!");
        return false;
    }

    // 4. 准备 Handler 的注入依赖
    uart_handler_input_instance_t handler_input = {
        .uart_driver = &g_uart_driver,
        .queue_ops   = &os_queue_ops,
        .time_ops    = &os_time_ops
    };

    // 5. 实例化 Handler 并完成内部函数挂载
    if (uart_handler_inst(&g_uart_handler, &handler_input) != UART_HANDLER_OK) {
        ESP_LOGE(TAG, "UART Handler 实例化失败!");
        return false;
    }

    // 6. 注册业务逻辑回调
    g_uart_handler.register_rx_callback(&g_uart_handler, on_uart_packet_received);

    // 7. 创建 FreeRTOS 任务来运行 Handler 状态机
    xTaskCreate(uart_handler_task, "uart_handler_task", 4096, &g_uart_handler, 10, NULL);

    ESP_LOGI(TAG, "UART Handler 任务已启动，正在监听数据...");
    return true;
}

void app_main(void)
{
    if (!init_uart_and_handler()) {
        return;
    }
    uint8_t test_payload[] = {0x09, 0x29,0x45,0x34};
    while(1)
    {
        // 调用你在 bsp_uart_handler 里封装好的发送函数
        // 这里传入 sensor_id = 0x01，以及测试数据
        g_uart_handler.send_frame(&g_uart_handler, 0x01, test_payload, sizeof(test_payload));
        
        ESP_LOGI(TAG, "已通过 GPIO 18 发送了一帧测试数据...");
        
        // 延时 2 秒
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}