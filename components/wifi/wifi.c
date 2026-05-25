#include "wifi.h"
#include "wifi_reg.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "mymqtt.h"
static const char *TAG = "WIFI";

void wifi_event_handler(void* arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
            ESP_LOGI(TAG, "Wi-Fi STA 已连接");
        }
    } 
    // 必须与上面的 if 平级！
    else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            esp_netif_ip_info_t* event = (esp_netif_ip_info_t*) event_data;
            ESP_LOGI(TAG, "IP地址: " IPSTR, IP2STR(&event->ip));
        }
    }
}


/*wifi init*/
void wifi_init_STA(void)
{
      ESP_LOGI(TAG, "Wi-Fi init started");
      /*调用nvs_flash_init（在main函数中）*/
      /*nvs_flash_init();*/
      
      /*初始化lwip*/
      esp_netif_init();

      /*创建默认事件循环*/
      esp_event_loop_create_default();

      /*创建WiFi事件处理器*/
      esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
      
      /*创建IP事件处理器*/
      esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
      
      /*创建默认WiFi STA网络接口实例*/
      esp_netif_create_default_wifi_sta();
      
      /*初始化WiFi*/
      wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
      esp_wifi_init(&cfg);
      
      /*设置WiFi模式*/
      esp_wifi_set_mode(WIFI_MODE_STA);
     
      /*设置WiFi配置*/
     wifi_config_t wifi_config = {
    .sta = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
        .scan_method = WIFI_FAST_SCAN,
        .failure_retry_cnt = 5,
        // 强制指定认证模式，增加稳定性
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
    },
};
      esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);

      /*检查wifide*/
      ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
      /*start wifi*/
      esp_wifi_start();
      ESP_LOGI(TAG, "Wi-Fi init finished");     
}