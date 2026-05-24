/**
 * @file bsp_wifi_handler.c
 * @brief Wi-Fi 处理层：负责维护发送队列、执行 MQTT 任务、打包 JSON 以及内存回收。
 */

#include "bsp_wifi_handler.h"
#include "bsp_wifi_reg.h"
#include "bsp_wifi_bridge.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>

// 假设你的工程里用的是 ESP 官方的 MQTT 库，如果是其他的请替换头文件
#include "mqtt_client.h" 

static const char *TAG = "WIFI_HANDLER";

// ==========================================================
// 1. 内部全局变量
// ==========================================================

// 发送队列句柄
static QueueHandle_t s_upload_queue = NULL;

// 你的 MQTT 客户端句柄 (通常在初始化网络时赋值)
extern esp_mqtt_client_handle_t mqtt_client; 

// ==========================================================
// 2. 供 Bridge 调用的入队函数
// ==========================================================

int bsp_wifi_handler_send_msg(internal_raw_msg_t *msg) {
    if (s_upload_queue == NULL) {
        ESP_LOGE(TAG, "发送队列未初始化！");
        return -1;
    }

    // 将消息体推入队列，如果满了最多等待 10 个系统 Tick
    if (xQueueSend(s_upload_queue, msg, pdMS_TO_TICKS(10)) != pdPASS) {
        ESP_LOGW(TAG, "网络发送队列已满，数据丢弃！");
        return -1; // 队列满，返回失败，让 Bridge 去释放内存
    }
    else{
        ESP_LOGI(TAG, "数据已成功入队，等待 MQTT 任务发送...");
    }

    return 0; // 入队成功
}

// ==========================================================
// 3. 【核心大脑】：MQTT 后台发送任务
// ==========================================================

static void mqtt_upload_task(void *pvParameters) {
    internal_raw_msg_t rx_msg;
    const char *publish_topic = "/device/medbox/upload"; // 你的云端上报 Topic

    ESP_LOGI(TAG, "MQTT 数据上传任务已启动...");

    while (1) {
        // 1. 阻塞等待队列中的数据 (portMAX_DELAY 意味着没有数据时不消耗任何 CPU 资源)
        if (xQueueReceive(s_upload_queue, &rx_msg, portMAX_DELAY) == pdPASS) {
            
            // 2. 创建一个空的 JSON 根节点
            cJSON *root = cJSON_CreateObject();
            
            // 3. 【分拣中心】：根据 Sensor_ID 还原不定长数据，并打包 JSON
            switch (rx_msg.sensor_id) {
                case 0x01: // 【温湿度】(预期4字节)
                    if (rx_msg.len == 4) {
                        float temp = (float)((rx_msg.payload[0] << 8) | rx_msg.payload[1]) / 100.0f;
                        float humi = (float)((rx_msg.payload[2] << 8) | rx_msg.payload[3]) / 100.0f;
                        cJSON_AddNumberToObject(root, "temp", temp);
                        cJSON_AddNumberToObject(root, "humi", humi);
                    }
                    break;

                case 0x02: // 【心率血氧】(预期2字节)
                    if (rx_msg.len >= 2) {
                        cJSON_AddNumberToObject(root, "heart_rate", rx_msg.payload[0]);
                        cJSON_AddNumberToObject(root, "spo2", rx_msg.payload[1]);
                    }
                    break;

                case 0x04: // 【K210 变长人脸特征码】
                {
                    // 因为串口传来的可能是不带 \0 结尾的字符数组，为了安全，我们再分配一个字符串
                    char *face_str = malloc(rx_msg.len + 1);
                    if (face_str) {
                        memcpy(face_str, rx_msg.payload, rx_msg.len);
                        face_str[rx_msg.len] = '\0'; // 手动加上字符串结束符
                        cJSON_AddStringToObject(root, "face_id", face_str);
                        free(face_str);
                    }
                    break;
                }

                default:
                    ESP_LOGW(TAG, "收到未知的 Sensor_ID: 0x%02X,无法打包", rx_msg.sensor_id);
                    break;
            }

            // 4. 将 JSON 对象转换为无格式字符串
            char *json_str = cJSON_PrintUnformatted(root);
            
            // 5. 执行 MQTT 发布
            if (json_str != NULL && mqtt_client != NULL) {
                // 参数：客户端句柄，主题，数据，长度，QoS=1(保证到达)，Retain=0
                esp_mqtt_client_publish(mqtt_client, publish_topic, json_str, strlen(json_str), 1, 0);
                ESP_LOGI(TAG, "成功上报云端\r\n"
                         "  Sensor ID: 0x%02X\r\n"
                         "  JSON Payload: %s", rx_msg.sensor_id, json_str);
            }

            // 6. 【生死攸关的步骤】：打扫战场，释放所有内存！
            if (json_str) free(json_str);       // 释放 cJSON_Print 分配的字符串内存
            cJSON_Delete(root);                 // 释放 cJSON_CreateObject 分配的 JSON 树结构内存
            free(rx_msg.payload);               // 释放由 Bridge 层 malloc 出来的原始数据缓存池！
        }
    }
}


static const char *TAG_1 = "MQTT_DOWNLINK";

//doenload task
/**
 * @brief MQTT 下行数据处理回调函数
 * @note  这个函数通常在 mqtt_event_handler 的 MQTT_EVENT_DATA 分支中被调用
 */
void mqtt_downlink_process(const char *topic, const char *payload, int payload_len) {
    ESP_LOGI(TAG_1, "收到云端下发数据, Topic: %s", topic);

    // 1. 【安全第一】：将载荷复制出为标准 C 字符串，防止没有 \0 导致 cJSON 越界崩溃
    char *json_str = malloc(payload_len + 1);
    if (json_str == NULL) {
        ESP_LOGE(TAG_1, "内存不足，无法解析下发指令");
        return;
    }
    memcpy(json_str, payload, payload_len);
    json_str[payload_len] = '\0';

    // 2. 解析 JSON
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        ESP_LOGW(TAG_1, "JSON 解析失败，抛弃该指令");
        free(json_str); // 别忘了释放刚才 malloc 的字符串
        return;
    }

    // 3. 提取核心指令 ID (对应你定义的 cloud_cmd_type_t)
    cJSON *cmd_item = cJSON_GetObjectItem(root, "cmd");
    if (cmd_item == NULL || !cJSON_IsNumber(cmd_item)) {
        ESP_LOGW(TAG_1, "指令格式错误，缺少 'cmd' 字段");
        cJSON_Delete(root);
        free(json_str);
        return;
    }

    // 4. 【组装二进制帧】：准备发给 CH32
    uart_downlink_frame_t tx_frame;
    memset(&tx_frame, 0, sizeof(tx_frame)); 
    tx_frame.header = UART_FRAME_HEADER;
    tx_frame.cmd = (uint8_t)cmd_item->valueint;
    tx_frame.tail = UART_FRAME_TAIL;

    // 5. 【分拣中心】：根据 CMD 提取参数并打包到 Payload 中
    switch (tx_frame.cmd) {
        case CMD_SET_MEDICATION_PLAN: // 0x10 设置用药计划
        {
            cJSON *box_item = cJSON_GetObjectItem(root, "box");
            cJSON *dose_item = cJSON_GetObjectItem(root, "dose");
            if (box_item && dose_item) {
                tx_frame.payload[0] = (uint8_t)box_item->valueint;
                tx_frame.payload[1] = (uint8_t)dose_item->valueint;
                tx_frame.len = 2; // 载荷长度2字节
            } else {
                ESP_LOGW(TAG, "CMD 0x10 缺少参数 box 或 dose");
            }
            break;
        }
        case CMD_FORCE_OPEN_BOX: // 0x12 强制开盖
        {
            cJSON *box_item = cJSON_GetObjectItem(root, "box");
            if (box_item) {
                tx_frame.payload[0] = (uint8_t)box_item->valueint;
                tx_frame.len = 1;
            }
            break;
        }
        case CMD_UPDATE_CLOCK: // 0x01 更新时钟
        {
            cJSON *time_item = cJSON_GetObjectItem(root, "timestamp");
            if (time_item) {
                uint32_t ts = (uint32_t)time_item->valuedouble; // Unix 时间戳比较大，用 double 承接
                // 大端序或小端序打包，这里假设小端序
                tx_frame.payload[0] = (uint8_t)(ts & 0xFF);
                tx_frame.payload[1] = (uint8_t)((ts >> 8) & 0xFF);
                tx_frame.payload[2] = (uint8_t)((ts >> 16) & 0xFF);
                tx_frame.payload[3] = (uint8_t)((ts >> 24) & 0xFF);
                tx_frame.len = 4;
            }
            break;
        }
        default:
            ESP_LOGW(TAG, "收到未处理的 CMD: 0x%02X,仅透传指令码", tx_frame.cmd);
            tx_frame.len = 0; // 没有额外参数
            break;
    }

    // 6. 计算校验和 (保障串口传输安全)
    tx_frame.checksum = calculate_checksum(&tx_frame);

    // 7. 发送给 CH32
    // 总长度 = 头(1) + CMD(1) + LEN(1) + Payload实际长度 + 校验(1) + 尾(1)
    size_t total_len = 5 + tx_frame.len; 
    uart_send_to_ch32((uint8_t *)&tx_frame, total_len);

    ESP_LOGI(TAG, "指令已转换为二进制发出, CMD: 0x%02X, PayloadLen: %d", tx_frame.cmd, tx_frame.len);

    // 8. 【生死攸关的步骤】：打扫战场
    cJSON_Delete(root); 
    free(json_str);

}













// ==========================================================
// 4. 初始化函数
// ==========================================================

int bsp_wifi_handler_init(void) {
    // 1. 创建消息队列，最多缓存 20 个包裹（足以应对高并发的传感器上报）
    s_upload_queue = xQueueCreate(20, sizeof(internal_raw_msg_t));
    if (s_upload_queue == NULL) {
        ESP_LOGE(TAG, "创建上传队列失败！");
        return -1;
    }

    // 2. 创建 MQTT 后台任务
    // 分配 2048 字节的任务栈，优先级设为 4 (中等优先级)
    BaseType_t ret = xTaskCreate(mqtt_upload_task, "mqtt_tx_task", 2048, NULL, 4, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "创建 MQTT 上传任务失败！");
        return -1;
    }

    ESP_LOGI(TAG, "Wi-Fi Handler 初始化完成！");
    return 0;
}