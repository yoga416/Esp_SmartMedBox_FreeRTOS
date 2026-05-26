#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "app_time_sync.h"
#include "uart.h"

static const char *TAG = "APP_TIME_SYNC";

static void time_sync_task(void *pvParameters) {
    struct tm timeinfo;
    time_t nowtime;

    while(1) {
        nowtime = time(NULL); 
        timeinfo = *localtime(&nowtime); 

        if (timeinfo.tm_year + 1900 > 2000) {
            // ESP_LOGI(TAG, "下发同步时间: %04d-%02d-%02d %02d:%02d:%02d", 
            //          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
            //          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            
            app_uart_send_time(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                               timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        } else {
            ESP_LOGW(TAG, "等待 SNTP 网络对时...");
        }

        vTaskDelay(pdMS_TO_TICKS(10000)); 
    }
}

void app_time_sync_start_task(void) {
    xTaskCreate(time_sync_task, "time_sync_task", 3072, NULL, 4, NULL);
}
