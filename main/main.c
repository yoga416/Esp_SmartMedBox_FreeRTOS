
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "ntc.h"
#include <time.h>
#include "mymqtt.h"
#include "mqtt_client.h"
#include "uart.h"
#include "uart_reg.h"
#include "app_weather.h"
#include "app_time_sync.h"

// =========================================================================
// 5. 主函数
// =========================================================================
void app_main(void) {
    /* Initialize NVS */
    nvs_flash_init();

    // 优先初始化串口，防止 WiFi 连上后发送状态时驱动还没准备好
    app_uart_init();

    /* Initialize Wi-Fi in Station mode */
    wifi_init_STA();
    vTaskDelay(pdMS_TO_TICKS(1000)); // 等待 Wi-Fi 连接稳定
    /* Initialize NTC */
    ntc_init();
    // 启动 MQTT，连接你的服务器
    mymqtt_init();

    // 发送一条测试数据
    app_uart_send_string("Hello, ESP32-S3 UART works!\r\n");

    // 1. 创建串口接收监听任务
    xTaskCreate(app_uart_receive_task, "uart_rx_task", 4096, NULL, 5, NULL);
    
    // 2. 启动独立的时间同步任务 (已移至单独模块)
    app_time_sync_start_task();

    // 3. 启动天气同步任务 (已移至单独模块)
    app_weather_start_task();

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

