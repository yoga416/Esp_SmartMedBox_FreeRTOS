#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "app_weather.h"
#include "uart.h"

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "WEATHER_SYNC";

// 心知天气 API URL: 使用提供的密钥，定位设置为 'taiyuan' (之前定位 ip 可能失败)，语言简体中文，单位摄氏度
#define SENIVERSE_URL "http://api.seniverse.com/v3/weather/now.json?key=SMppJL-t2_vNrnQ5C&location=taiyuan&language=zh-Hans&unit=c"

// 接收 HTTP 响应数据的最大长度
#define MAX_HTTP_RECV_BUFFER 1024

static void weather_sync_task(void *pvParameters) {
    // 等待系统稳定和网络连接成功后再开始第一次同步
    vTaskDelay(pdMS_TO_TICKS(5000)); 
    
    char local_response_buffer[MAX_HTTP_RECV_BUFFER] = {0};

    esp_http_client_config_t config = {
        .url = SENIVERSE_URL,
        .method = HTTP_METHOD_GET,
    };

    for(;;) {
        time_t now_time;
        struct tm timeinfo;
        time(&now_time);
        localtime_r(&now_time, &timeinfo);
        
        // 如果年份小于 2000，说明 SNTP 还没对时成功，天气请求会因为证书/时间校验失败
        if (timeinfo.tm_year + 1900 < 2000) {
            ESP_LOGW(TAG, "等待网络对时中 (SNTP)...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        ESP_LOGI(TAG, "开始同步天气和定位信息...");

        esp_http_client_handle_t client = esp_http_client_init(&config);
        
        // 执行 HTTP GET 请求
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            int read_len = esp_http_client_get_content_length(client); // 获取回复内容长度
           
        }

        // 修改为更兼容的请求方式
        esp_http_client_cleanup(client);
        
        // 重新初始化并请求
        client = esp_http_client_init(&config);
        err = esp_http_client_open(client, 0);
        if (err == ESP_OK) {
            int content_length = esp_http_client_fetch_headers(client);
            if (content_length >= 0) {
                int read_len = esp_http_client_read(client, local_response_buffer, MAX_HTTP_RECV_BUFFER - 1);
                if (read_len > 0) {
                    local_response_buffer[read_len] = '\0';
                    ESP_LOGI(TAG, "获取数据成功: %s", local_response_buffer);

                // 使用 cJSON 解析返回的数据
                cJSON *root = cJSON_Parse(local_response_buffer);
                if (root != NULL) {
                    cJSON *results = cJSON_GetArrayItem(cJSON_GetObjectItem(root, "results"), 0);
                    if (results != NULL) {
                        // 1. 获取定位信息
                        cJSON *location = cJSON_GetObjectItem(results, "location");
                        const char *city_name = cJSON_GetObjectItem(location, "name")->valuestring;

                        // 2. 获取天气信息
                        cJSON *now = cJSON_GetObjectItem(results, "now");
                        const char *weather_text = cJSON_GetObjectItem(now, "text")->valuestring; // 天气现象，如"晴"
                        const char *temperature_str = cJSON_GetObjectItem(now, "temperature")->valuestring; // 温度
                        const char *code_str = cJSON_GetObjectItem(now, "code")->valuestring; // 天气代码 (用于显示图标)

                        int temp_val = atoi(temperature_str);
                        int code_val = atoi(code_str);

                        ESP_LOGI(TAG, "同步成功! 当前位置: %s, 天气: %s (代码: %d), 温度: %d°C", 
                                 city_name, weather_text, code_val, temp_val);
                        
                        // --- 新增：通过串口发送给下位机 ---
                        app_uart_send_location(city_name);
                        app_uart_send_weather(code_val, temp_val);
                        // ------------------------------
                    }
                    cJSON_Delete(root); // 务必释放内存，防止内存泄漏
                } else {
                    ESP_LOGE(TAG, "JSON 解析失败");
                }
            } else {
                ESP_LOGE(TAG, "读取 HTTP 响应失败");
            }
        } else {
            ESP_LOGE(TAG, "HTTP 客户端打开失败: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client); // 清理客户端资源


        vTaskDelay(pdMS_TO_TICKS(1* 60 * 1000)); // 每1分钟同步一次
    }
}
}

void app_weather_start_task(void) {
    xTaskCreate(weather_sync_task, "weather_task", 8192, NULL, 3, NULL);
}
