#include "mymqtt.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "uart.h" 
#include "cJSON.h"
#include "sensor_parser.h"
static const char *TAG = "MY_MQTT";

// 全局 MQTT 客户端句柄 (非常重要，不要在局部被覆盖)
static esp_mqtt_client_handle_t g_mqtt_client = NULL;
void handle_onenet_set_schedule(const char *json_data, int data_len);
void handle_onenet_set(const char *json_data, int data_len);
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
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED (已成功连接!)");
        
        msg_id=esp_mqtt_client_subscribe(client, TOPIC_RELAY, 0);
        ESP_LOGI(TAG, "已订阅主题: %s, msg_id=%d", TOPIC_RELAY, msg_id);

        msg_id = esp_mqtt_client_subscribe(client, TOPIC_SET, 0);
        ESP_LOGI(TAG, "已订阅主题: %s, msg_id=%d", TOPIC_SET, msg_id);

        msg_id = esp_mqtt_client_subscribe(client, TOPIC_POST_REPLY, 0);
        ESP_LOGI(TAG, "已订阅主题: %s, msg_id=%d", TOPIC_POST_REPLY, msg_id);

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

        // --- 第一层分流：根据 Topic 主题分流 ---
        
        // 1. 属性设置主题 (通常是 JSON 格式，如 med_schedule)
        if (strncmp(event->topic, TOPIC_SET, event->topic_len) == 0) {
            handle_onenet_set(event->data, event->data_len);
        }
        // 2. 继电器/控制指令主题 (可能是简单的 0/1 或特定指令)
        else if (strncmp(event->topic, TOPIC_RELAY, event->topic_len) == 0) {
            ESP_LOGI(TAG, "收到控制指令!");
            // 这里可以直接处理，或者分发给专门的控制函数
            // handle_control_command(event->data, event->data_len);
        }
        // 3. 其它自定义主题
        else {
            ESP_LOGW(TAG, "收到未定义主题的数据");
        }
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
        .credentials.authentication.password = PASSWORD2_esp32_526,
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
    
    if (payload == NULL) {
        ESP_LOGE(TAG, "payload 为 NULL!");
        return false;
    }

    if (g_mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT 客户端实例为空");
        return false;
    }
    
    
    ESP_LOGI(TAG, "准备发送至主题: %s, 内容: %s", topic, payload);

    int msg_id = esp_mqtt_client_publish(g_mqtt_client, topic, payload, 0, 1, 0);
    
    if (msg_id == -1) {

        ESP_LOGE(TAG, "esp_mqtt_client_publish 返回 -1，请检查连接状态！");
        return false;
    }
    
    ESP_LOGI(TAG, "发布成功，消息 ID: %d", msg_id);
    return true;
}

// 处理 OneNet 下发的属性设置指令
void handle_onenet_set(const char *json_data, int data_len)
{
    ESP_LOGI(TAG, "处理 OneNet 下发的属性设置指令，数据长度: %d", data_len);
    ESP_LOGI(TAG, "原始 JSON 数据: %.*s", data_len, json_data);

    // 1. MQTT 接收到的数据可能没有字符串结束符 '\0'，需要手动拷贝一份
    char *json_str = malloc(data_len + 1);
    if (json_str == NULL) return;
    memcpy(json_str, json_data, data_len);
    json_str[data_len] = '\0';

    // 2. 解析 JSON 根节点
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        ESP_LOGE(TAG, "云端下发数据 JSON 解析失败");
        free(json_str);
        return;
    }

    // --- 新增：获取 msg_id 用于回复云端 ---
    cJSON *id_obj = cJSON_GetObjectItem(root, "id");
    char *request_id = id_obj ? id_obj->valuestring : "0";

    // 3. 提取 "params" 对象
    cJSON *params = cJSON_GetObjectItem(root, "params");
    if (params == NULL) {
        ESP_LOGE(TAG, "JSON 中缺少 'params' 字段");
        cJSON_Delete(root);
        free(json_str);
        return;
    }

    bool parse_success = false;
    // 4. 遍历 params 下的所有属性
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, params) {
        const char *identifier = item->string;
        ESP_LOGI(TAG, "解析到功能标识符: %s", identifier);

        // 获取该标识符对应的 "value" (OneNet 既可能直接下发数据，也可能包裹在 value 里)
        cJSON *value_obj = cJSON_GetObjectItem(item, "value");
        cJSON *content = value_obj ? value_obj : item;

        // ==========================================
        // 🚀 内容分流 (Internal Dispatching)
        // ==========================================
        
        // 用户1的用药时间
        if (strcmp(identifier, "med_schedules") == 0) {
            ESP_LOGI(TAG, ">>> 进入 [用户1服药排班] 分流分支");
            parse_medication_schedule_1(content);
            parse_success = true;
        }
        //用户2的用药时间
         else if (strcmp(identifier, "mechine_time_2") == 0) {
            ESP_LOGI(TAG, ">>> 进入 [用户2服药排班] 分流分支");
            parse_medication_schedule_2(content);
            parse_success = true;
        }
        //用户3的用药时间
         else if (strcmp(identifier, "mechine_time_3") == 0) {
            ESP_LOGI(TAG, ">>> 进入 [用户3服药排班] 分流分支");
            parse_medication_schedule_3(content);
            parse_success = true;
        }
        // led控制
        else if (strcmp(identifier, "LED_STATUS") == 0) {
            ESP_LOGI(TAG, ">>> 进入 [LED控制] 分流分支");
           parse_led_control(content);
           parse_success = true;
        }
        // 蜂鸣器控制
        else if (strcmp(identifier, "BAZZER_STATUS") == 0) {
            ESP_LOGI(TAG, ">>> 进入 [蜂鸣器控制] 分流分支");
            parse_buzzer_control(content);
            parse_success = true;
        }
        //  温度阈值数据下发
        else if (strcmp(identifier, "Temperature_Threshold") == 0) {
            ESP_LOGI(TAG, ">>> 进入 [温度阈值] 分流分支");
            parse_set_temp_threshold(content);
            parse_success = true;
        }
        // 湿度阈值数据下发
        else if (strcmp(identifier, "Humidity_Threshold") == 0) {
            ESP_LOGI(TAG, ">>> 进入 [湿度阈值] 分流分支");
            parse_set_humi_threshold(content);
            parse_success = true;
        }
        else {
            ESP_LOGW(TAG, "未定义的标识符: %s, 无法分流处理", identifier);
        }
    }

    // --- 新增：给云端回信 (set_reply) ---
    // 告知云端：我已收到并处理成功，请停止重试或更新期望状态。
    if (parse_success) {
        char reply_topic[128];
        char reply_json[128];
        // 修正：OneNet 的回复主题通常是 set_reply
        snprintf(reply_topic, sizeof(reply_topic), "$sys/%s/%s/thing/property/set_reply", USENAME, CLIENT_ID);
        snprintf(reply_json, sizeof(reply_json), "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\"}", request_id);
        
        ESP_LOGI(TAG, "向云端发送回复 (set_reply)...");
        mymqtt_publish_data(reply_topic, reply_json);
    }

    cJSON_Delete(root);
    free(json_str);
}