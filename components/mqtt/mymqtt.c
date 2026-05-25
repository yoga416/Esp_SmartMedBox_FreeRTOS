#include "mymqtt.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "MY_MQTT";

// 全局 MQTT 客户端句柄 (非常重要，不要在局部被覆盖)
static esp_mqtt_client_handle_t g_mqtt_client = NULL;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    int count = 0;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED (已成功连接!)");
        
        msg_id=esp_mqtt_client_subscribe(client, TOPIC_RELAY, 0);
        ESP_LOGI(TAG, "已订阅主题: %s, msg_id=%d", TOPIC_RELAY, msg_id);

        msg_id = esp_mqtt_client_subscribe(client, TOPIC_SET, 0);
        ESP_LOGI(TAG, "已订阅主题: %s, msg_id=%d", TOPIC_SET, msg_id);

        

        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED (连接断开)");
        //重新连接会由 esp-mqtt 内部自动处理，除非在配置中禁用了自动重连功能
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA (收到下发数据)");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
        
    default:
        ESP_LOGD(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mymqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = URI,
        .credentials.username = USENAME, 
        .credentials.authentication.password = PASSWORD,
        .credentials.client_id = CLIENT_ID
    };

    g_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    
    if (g_mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT 客户端初始化失败!");
        return;
    }

    esp_mqtt_client_register_event(g_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(g_mqtt_client);
}


bool mymqtt_publish_data(const char *topic, const char *payload)
{
    if (g_mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT 尚未初始化，无法发送!");
        return false;
    }
    
    int msg_id = esp_mqtt_client_publish(g_mqtt_client, topic, payload, 0, 1, 0);
    
    if (msg_id == -1) {
        ESP_LOGE(TAG, "数据发布失败!");
        return false;
    }
    return true;
}