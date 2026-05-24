
#include "bsp_wifi_port.h"
#include "bsp_wifi_driver.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define TAG "bsp_wifi_port"


//wifi_hw_init函数实现
int wifi_hw_init(const char* ssid, const char* password, uint8_t channel, 
                  uint8_t max_connection, wifi_auth_mode_t authmode, void **wifi_handle)
{
      if(ssid==NULL)
      {
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_hw_init]hw_init null ptr!\r\n");
#endif
      }
      // ... 之前的参数检查 ...

    // 【新增 1】初始化底层 TCP/IP 堆栈
    ESP_ERROR_CHECK(esp_netif_init());

    // 【新增 2】创建系统事件循环 (WiFi 的连接、断开等事件依赖它)
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 【新增 3】创建默认的 WiFi 站点（STA）网络接口实例
    esp_netif_create_default_wifi_sta();
      //初始化nvs
      //nvs_flash_init();

      //初始化wifi驱动
      wifi_init_config_t cfg=WIFI_INIT_CONFIG_DEFAULT();
      esp_wifi_init(&cfg);

      esp_wifi_set_mode(WIFI_MODE_STA);

      wifi_config_t wifi_config={0};
      strncpy((char*) wifi_config.sta.ssid,ssid,sizeof(wifi_config.sta.ssid)-1);
      strncpy((char*) wifi_config.sta.password,password,sizeof(wifi_config.sta.password)-1);
      esp_wifi_set_config(WIFI_IF_STA,&wifi_config);

      esp_wifi_start();
      esp_wifi_connect();
        //esp_wifi_connect();

        // 分配并返回 WiFi 句柄
        typedef struct {
              bool inited;
        } wifi_handler_t;
        wifi_handler_t *handler = malloc(sizeof(wifi_handler_t));
        if (!handler) {
              printf("[wifi_hw_init] malloc wifi_handler_t failed!\r\n");
              return -1;
        }
        handler->inited = true;
        *wifi_handle = handler;
      ESP_LOGI(TAG, "WiFi初始化成功，SSID: %s", ssid);
      return 0; // 成功
}

//wifi的deinit函数
int wifi_hw_deinit(void* wifi_handle)
{
      if(wifi_handle==NULL)
      {
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_hw_deinit]hw_deinit null ptr!\r\n");    
#endif      
            return -1;
      }
      free(wifi_handle);
      esp_wifi_stop();
      esp_wifi_deinit();
      nvs_flash_deinit();
      return 0;
}
//connect函数实现
int wifi_hw_connect(void* wifi_handle, const char* ssid, const char* password)
{
      if(wifi_handle==NULL || ssid==NULL || password==NULL)
      {
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_hw_connect]hw_connect null ptr!\r\n");  
#endif
            return -1;
      }
      esp_wifi_connect();
      return 0;
}

//disconnect函数实现
int wifi_hw_disconnect(void* wifi_handle)
{
      if(wifi_handle==NULL)
      {
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_hw_disconnect]hw_disconnect null ptr!\r\n");  
#endif      
            return -1;
      }
      esp_wifi_disconnect();
      return 0;
}

//send函数实现
int wifi_hw_send(void* wifi_handle, const uint8_t *data, uint32_t len)
{
      if(wifi_handle==NULL || data==NULL || len==0)
      {
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_hw_send]hw_send null ptr or zero len!\r\n");
#endif
            return -1;
      }
    // 实际应用应通过 socket 接收数据，这里仅为示例

      return 0; // 成功

}

//receive函数实现
int wifi_hw_receive(void* wifi_handle, uint8_t *buf, uint32_t max_len, uint32_t timeout_ms)
{
      if(wifi_handle==NULL || buf==NULL || max_len==0)
      {
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_hw_receive]hw_receive null ptr or zero max_len!\r\n");    
#endif
            return -1;  
      }     
      // 实际应用应通过 socket 接收数据，这里仅为示例
      return 0; // 成功
}