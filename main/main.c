
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
// =========================================================================
// 5. 主函数
// =========================================================================
void app_main(void) {
    struct tm timeinfo;
    time_t nowtime;
    /* Initialize NVS */
    nvs_flash_init();
    /* Initialize Wi-Fi in Station mode */
    wifi_init_STA();
    vTaskDelay(pdMS_TO_TICKS(2000)); // 等待 Wi-Fi 连接稳定
    /* Initialize NTC */
    ntc_init();
    // 启动 MQTT，连接你的服务器（换成你自己的物理机IP或公网云服务器IP）
    mymqtt_init();
    // 初始化串口
    app_uart_init();

    // 发送一条测试数据
    app_uart_send_string("Hello, ESP32-S3 UART works!\r\n");

    // 创建串口接收监听任务 (分配 2048 字节栈空间，优先级设为 5)
    xTaskCreate(app_uart_receive_task, "uart_rx_task", 2048, NULL, 5, NULL);
    int i=0;
    while(1) {
        
        nowtime = time(NULL); // 获取当前时间戳，触发 SNTP 同步
        timeinfo = *localtime(&nowtime); // 获取当前时间戳，触发 SNTP 同步
        ESP_LOGI("APP_MAIN", "当前时间: %04d-%02d-%02d %02d:%02d:%02d", 
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
       // 只有当网络时间同步成功后（年份大于1970），才允许发送，防止不合法的 id 被平台拒绝
    if (timeinfo.tm_year + 1900 > 1970) {
        char report_json[256];
        
        // 尝试把值改成 45（如果网页上原先是 50，我们改成 45 看看数字变没变）
        i++;
        int current_temp = 45+i; // 这里你可以替换成实际的 NTC 读数

        // 规范拼装：每次上报的 "id" 使用当前动态时间戳，防止因 id 重复被平台去重丢弃
        sprintf(report_json, 
                "{\"id\":\"%ld\",\"version\":\"1.0\",\"params\":{\"temperture\":{\"value\":%d}}}", 
                (long)nowtime, current_temp);

        ESP_LOGW("APP_MAIN", "正在向云端定时定量上报数据: %s", report_json);
        
        // 调用封装接口发送
        mymqtt_publish_data(TOPIC_POST, report_json);
    } else {
        ESP_LOGW("APP_MAIN", "SNTP 时间未同步，暂不上报数据...");
    }

        vTaskDelay(pdMS_TO_TICKS(5000)); // 主循环每5秒打印一次日志，保持任务活跃
    }
}