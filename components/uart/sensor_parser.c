#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "sensor_parser.h"
#include "mymqtt.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "uart.h"
static const char *TAG = "SENSOR_PARSER";

// ==========================================
//  ID 定义
// ==========================================
#define FRAME_HEADER 0x5A
#define FRAME_TAIL   0xFF

#define CMD_UPLOAD_SHT40    0x00
#define CMD_UPLOAD_MLX90614 0x03
#define CMD_UPLOAD_MAX30102 0x04
#define CMD_UPLOAD_MISSED_MED  0x20  // 新增：漏服记录上传
#define CMD_MED_SCHEDULE_UPLOAD 0x31 // 新增：服药计划上传0x21


/*全局变量*/
static uint32_t temp_threshold =0; 
static uint32_t humi_threshold =0; 
// ==========================================
// 🧮 CRC8 校验函数 (多项式 0x07, 初始值 0x00)
// ==========================================
uint8_t calc_crc8(const uint8_t *data, uint16_t len) {
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// ==========================================
// 🔍 数据帧解析主函数
// ==========================================
void parse_sensor_frame(const uint8_t *buffer, uint16_t len) {
    char report_json[512]; // 稍微增大 buffer 防止多字段溢出
    static uint32_t report_id = 1;
    char miss_str[80];
    // 遍历缓冲区寻找合法帧 (总长至少为 6 bytes: 头+长+Cmd+User+CRC+尾)
    for (uint16_t i = 0; i < len - 5; i++) {
        
        // 1. 寻找帧头 0x5A
        if (buffer[i] == FRAME_HEADER) {
            uint8_t payload_len = buffer[i + 1];
            // 新格式总长：帧头(1) + 长度(1) + 指令ID(1) + 用户ID(1) + Payload(N) + CRC(1) + 帧尾(1)
            uint16_t frame_total_len = payload_len + 6; 

            // 如果剩余数据长度不够一个完整帧，退出等待下一次数据到来
            if (i + frame_total_len > len) {
                break; 
            }

            // 2. 检查帧尾 0xFF
            if (buffer[i + frame_total_len - 1] != FRAME_TAIL) {
                continue; // 帧尾不对，可能是伪造的帧头，继续往后扫描
            }

            // 3. 校验 CRC8
            // CRC 位于帧尾前一个字节：i + payload_len + 4
            uint8_t expected_crc = buffer[i + payload_len + 4];
            // 参与校验的字段：Length(1) + Cmd_ID(1) + User_ID(1) + Payload(N) = N + 3
            uint8_t calc_crc = calc_crc8(&buffer[i + 1], payload_len + 3);

            if (expected_crc != calc_crc) {
                ESP_LOGW(TAG, "CRC校验失败! 计算值:0x%02X, 期望值:0x%02X", calc_crc, expected_crc);
                continue; 
            }

            // 4. 提取信息
            uint8_t cmd_id   = buffer[i + 2];
            uint8_t user_id  = buffer[i + 3];
            const uint8_t *payload = &buffer[i + 4];

            ESP_LOGI(TAG, "收到有效帧 -> Cmd: 0x%02X, UserID: %d", cmd_id, user_id);

            switch (cmd_id) {
                case CMD_UPLOAD_SHT40: {
                    if (payload_len == 8) {
                        int32_t raw_temp = 0, raw_hum = 0;
                        memcpy(&raw_temp, payload, 4);
                        memcpy(&raw_hum, payload + 4, 4);
                        
                        float temp = raw_temp / 100.0f;
                        float humi = raw_hum / 100.0f;
                        ESP_LOGI(TAG, "✅ [User %d] 环境温湿度: %.2fC, %.2f%%", user_id, temp, humi);

                        /*检查盒内温湿度是否超过阈值*/
                        if (temp > temp_threshold) {
                            ESP_LOGW(TAG, "⚠️ 温度 %.2fC 超过阈值!", temp);  
                            /*可以上传，可以下发给主机*/  
                        }
                        if(humi > humi_threshold) {
                            ESP_LOGW(TAG, "⚠️ 湿度 %.2f%% 超过阈值!", humi);  
                            /*可以上传，可以下发给主机*/    
                        }
                         /*向云端同步*/
                        /*根据user_id选择ison文本*/
                        switch (user_id) {
                            case 1:
                            sprintf(report_json, 
                            "{"
                                "\"id\": \"%lu\","
                                "\"version\": \"1.0\","
                                "\"params\": {"
                                    "\"ambient_temp\": {\"value\": %.1f},"
                                    "\"ambient_humi\": {\"value\": %.1f}"
                                "}"
                            "}", report_id++,temp, humi);
                                break;

                            case 2:
                               sprintf(report_json, 
                            "{"
                                "\"id\": \"%lu\","
                                "\"version\": \"1.0\","
                                "\"params\": {"
                                    "\"user2_ambient_temp\": {\"value\": %.1f},"
                                    "\"user2_ambient_humi\": {\"value\": %.1f}"
                                "}"
                            "}", report_id++,temp, humi);
                                break;
                            case 3:
                               sprintf(report_json, 
                            "{"
                                "\"id\": \"%lu\","
                                "\"version\": \"1.0\","
                                "\"params\": {"
                                    "\"user3_ambient_temp\": {\"value\": %.1f},"
                                    "\"user3_ambient_humi\": {\"value\": %.1f}"
                                "}"
                            "}", report_id++,temp, humi);
                                break;
                            default:
                                ESP_LOGW(TAG, "未知用户ID: %d", user_id);
                                break;
                        } 
                        mymqtt_publish_data(TOPIC_POST, report_json);
                    }
                    break;
                
                case CMD_UPLOAD_MLX90614: {
                    if (payload_len == 8) {
                        int32_t raw_obj = 0;
                        memcpy(&raw_obj, payload + 4, 4);
                        float obj_temp = raw_obj / 100.0f;
                        
                        ESP_LOGI(TAG, "✅ [User %d] 体温: %.2fC", user_id, obj_temp);
                        switch (user_id) {
                            case 1:
                                sprintf(report_json, 
                            "{"
                                "\"id\": \"%lu\","
                                "\"version\": \"1.0\","
                                "\"params\": {"
                                    "\"body_temp\": {\"value\": %.1f}"
                                "}"
                            "}", report_id++, obj_temp);
                                break;
                            case 2:
                                 sprintf(report_json, 
                            "{"
                                "\"id\": \"%lu\","
                                "\"version\": \"1.0\","
                                "\"params\": {"
                                    "\"uesr2_body_temp\": {\"value\": %.1f}"
                                "}"
                            "}", report_id++, obj_temp);
                                break;
                            case 3:
                                 sprintf(report_json, 
                            "{"
                                "\"id\": \"%lu\","
                                "\"version\": \"1.0\","
                                "\"params\": {"
                                    "\"user3_body_temp\": {\"value\": %.1f}"
                                "}"
                            "}", report_id++, obj_temp);
                                break;
                            default:
                                ESP_LOGW(TAG, "未知用户ID: %d", user_id);
                                break;
                        } 
                         mymqtt_publish_data(TOPIC_POST, report_json);
                        }
                    }
                    break;
                case CMD_UPLOAD_MAX30102: {
                    if (payload_len == 4) {
                        uint16_t raw_hr = 0, raw_spo2 = 0;
                        memcpy(&raw_hr, payload, 2);
                        memcpy(&raw_spo2, payload + 2, 2);
                        
                        float hr = raw_hr / 100.0f;
                        float spo2 = raw_spo2 / 100.0f;
                        ESP_LOGI(TAG, "✅ [User %d] 心率: %.0f bpm, 血氧: %.0f %%", user_id, hr, spo2);
                            switch (user_id) {
                                case 1:
                                    sprintf(report_json, 
                                "{"
                                    "\"id\": \"%lu\","
                                    "\"version\": \"1.0\","
                                    "\"params\": {"
                                        "\"heart_rate\": {\"value\": %.0f},"
                                        "\"spo2\": {\"value\": %.0f}"
                                    "}"
                                "}", report_id++, hr, spo2);
                                    break;
                                case 2:
                                    sprintf(report_json, 
                                "{"
                                    "\"id\": \"%lu\","
                                    "\"version\": \"1.0\","
                                    "\"params\": {"
                                        "\"uesr2_heart_rate\": {\"value\": %.0f},"
                                        "\"user2_spo2\": {\"value\": %.0f}"
                                    "}"
                                "}", report_id++,  hr, spo2);
                                    break;
                                case 3:
                                    sprintf(report_json, 
                                "{"
                                    "\"id\": \"%lu\","
                                    "\"version\": \"1.0\","
                                    "\"params\": {"
                                        "\"uesr3_heart_rate\": {\"value\": %.0f},"
                                        "\"user3_spo2\": {\"value\": %.0f}"
                                    "}"
                                "}", report_id++, hr, spo2);
                                    break;
                                default:
                                    ESP_LOGW(TAG, "未知用户ID: %d", user_id);
                                    break;
                            } 
                            mymqtt_publish_data(TOPIC_POST, report_json);
                    }
                    break;
                }

                case CMD_UPLOAD_MISSED_MED: {
                    // 解析漏服记录 (例如: payload_len = 7, Payload[0]=顿数, Payload[1~6]=年月日时分秒)
                    if (payload_len == 7) {
                        uint8_t med_idx = payload[0]; // 第几顿药 (0=早上, 1=中午, 2=晚上)
                        uint8_t y = payload[1];
                        uint8_t m = payload[2];
                        uint8_t d = payload[3];
                        uint8_t h = payload[4];
                        uint8_t min = payload[5];
                        uint8_t s = payload[6];

                        ESP_LOGW(TAG, "🚨 [User %d] 漏服警告! 第 %d 顿, 时间: 20%02d-%02d-%02d %02d:%02d:%02d", 
                                 user_id, med_idx+1, y, m, d, h, min, s);

                      switch (user_id) {
                            case 1:
                            snprintf(miss_str, sizeof(miss_str),
                            "med_index=%d,timestamp=20%02d-%02d-%02d %02d:%02d:%02d",
                            med_idx, y, m, d, h, min, s);
                           snprintf(report_json, sizeof(report_json),
                            "{"
                            "\"id\":\"%lu\","
                            "\"version\":\"1.0\","
                            "\"params\":{"
                            "\"user%d_miss\":{"
                            "\"value\":\"%s\""
                            "}"
                            "}"
                            "}",
                     report_id++, user_id, miss_str);

                            ESP_LOGW(TAG, "发布漏服记录到 MQTT: %s", report_json);
                            break;

                            case 2:
                            snprintf(miss_str, sizeof(miss_str),
                            "med_index=%d,timestamp=20%02d-%02d-%02d %02d:%02d:%02d",
                            med_idx, y, m, d, h, min, s);
                             snprintf(report_json, sizeof(report_json),
                            "{"
                            "\"id\":\"%lu\","
                            "\"version\":\"1.0\","
                            "\"params\":{"
                            "\"user%d_miss\":{"
                            "\"value\":\"%s\""
                            "}"
                            "}"
                            "}",
                            report_id++, user_id, miss_str);
                            break;

                            case 3:
                            snprintf(miss_str, sizeof(miss_str),
                            "med_index=%d,timestamp=20%02d-%02d-%02d %02d:%02d:%02d",
                            med_idx, y, m, d, h, min, s);
                           snprintf(report_json, sizeof(report_json),
                            "{"
                            "\"id\":\"%lu\","
                            "\"version\":\"1.0\","
                            "\"params\":{"
                            "\"user%d_miss\":{"
                            "\"value\":\"%s\""
                            "}"
                            "}"
                            "}",
                            report_id++, user_id, miss_str);
                            break;

                            default:
                                ESP_LOGW(TAG, "未知用户ID: %d", user_id);
                                break;
                        } 
                        
                        // 确保 report_json 不是空的再去发布
                        if(user_id >= 1 && user_id <= 3) {
                            ESP_LOGW(TAG, "发布漏服记录到 MQTT: %s", report_json);
                            ESP_LOGW(TAG, "发布漏服记录到 MQTT: %s", TOPIC_POST);
                            mymqtt_publish_data(TOPIC_POST, report_json);
                        }
                    }
                    break;
                }

                case CMD_MED_SCHEDULE_UPLOAD: {
                    // 解析服药计划: 3个时间点*2字节(时,分) + 3个药丸数量 = 9字节
                    if (payload_len == 9) {
                        for (int k = 0; k < 3; k++) {
                            uint8_t hour  = payload[k * 2];
                            uint8_t min   = payload[k * 2 + 1];
                            uint8_t count = payload[6 + k];
                            ESP_LOGI(TAG, "📅 [User %d] 计划 %d: %02d:%02d, 药量: %d 颗", 
                                     user_id, k + 1, hour, min, count);
                        }

                        // 动态选择云端标识符 (家庭组模式)
                        const char* prop_name;
                        if (user_id == 1)        prop_name = "med_schedules";
                        else if (user_id == 2)   prop_name = "mechine_time_2";
                        else                     prop_name = "mechine_time_3";

                        // 构造符合 OneNet 数组结构体格式的 JSON
                        snprintf(report_json, sizeof(report_json),
                            "{\"id\":\"%lu\",\"version\":\"1.0\",\"params\":{\"%s\":{\"value\":[{"
                            "\"user_id\":%d,"
                            "\"time_1\":\"%02d:%02d(%d)\","
                            "\"time_2\":\"%02d:%02d(%d)\","
                            "\"time_3\":\"%02d:%02d(%d)\""
                            "}]}}}",
                            report_id++,
                            prop_name,
                            (int)user_id,
                            payload[0], payload[1], payload[6],
                            payload[2], payload[3], payload[7],
                            payload[4], payload[5], payload[8]
                        );

                        ESP_LOGI(TAG, "上报 JSON : %s", report_json);
                        mymqtt_publish_data(TOPIC_POST, report_json);
                       
                    } else {
                        ESP_LOGW(TAG, "服药计划长度错误: %d (预期 9)", payload_len);
                    }
                    break;
                }
                default:
                    ESP_LOGW(TAG, "未知的 Cmd ID: 0x%02X", cmd_id);
                    break;
            
            // 一帧解析成功，将循环索引推移到该帧尾部，避免重复解析
            i += frame_total_len - 1; 
        }
    }
}
}
}

/*用户1的用药时间的下发*/
void parse_medication_schedule_1(cJSON *value_arr)
{

    int count = cJSON_GetArraySize(value_arr);
    for (int i = 0; i < count; i++) {
        cJSON *schedule = cJSON_GetArrayItem(value_arr, i);
        if (schedule == NULL) continue;

        cJSON *uid_obj = cJSON_GetObjectItem(schedule, "user_id");
        cJSON *t1_obj = cJSON_GetObjectItem(schedule, "time_1");
        cJSON *t2_obj = cJSON_GetObjectItem(schedule, "time_2");
        cJSON *t3_obj = cJSON_GetObjectItem(schedule, "time_3");

        if (!uid_obj || !t1_obj || !t2_obj || !t3_obj) {
            ESP_LOGW(TAG, "节点数据不完整，跳过");
            continue;
        }

        int user_id = uid_obj->valueint;
        const char *time_1 = t1_obj->valuestring;
        const char *time_2 = t2_obj->valuestring;
        const char *time_3 = t3_obj->valuestring;

        ESP_LOGI(TAG, ">>> [User %d] 原始数据: T1:%s, T2:%s, T3:%s", user_id, time_1, time_2, time_3);

        int h1, m1, c1, h2, m2, c2, h3, m3, c3;
        if (sscanf(time_1, "%d:%d(%d)", &h1, &m1, &c1) == 3 &&
            sscanf(time_2, "%d:%d(%d)", &h2, &m2, &c2) == 3 &&
            sscanf(time_3, "%d:%d(%d)", &h3, &m3, &c3) == 3) 
        {
            ESP_LOGI(TAG, "成功解析服药计划: user_id: %d", user_id);
            app_uart_send_med_schedule((uint8_t)user_id, 
                                       (uint8_t)h1, (uint8_t)m1, (uint8_t)c1,
                                       (uint8_t)h2, (uint8_t)m2, (uint8_t)c2,
                                       (uint8_t)h3, (uint8_t)m3, (uint8_t)c3);
        } else {
            ESP_LOGE(TAG, "字符串解析失败 (格式应为 HH:MM(Count))");
        }
    }
}

/*用户2的用药时间的下发*/
void parse_medication_schedule_2(cJSON *value_arr)
{
    parse_medication_schedule_1(value_arr);
}

/*用户3的用药时间的下发*/
void parse_medication_schedule_3(cJSON *value_arr)
{
    parse_medication_schedule_1(value_arr);
}

/* led控制 */
void parse_led_control(cJSON *content)
{
    int state = 0;
    static uint32_t report_id = 1;
    // 1. 完美兼容：如果是布尔型 (true/false)
    if (cJSON_IsBool(content)) {
        state = cJSON_IsTrue(content) ? 1 : 0;
    }
    // 2. 完美兼容：如果云端改发数字 (1/0)
    else if (cJSON_IsNumber(content)) {
        state = content->valueint;
    }
    ESP_LOGI(TAG, "解析到LED控制指令: %d", state);
    uint8_t frame[10];
    frame[0] = FRAME_HEADER;
    frame[1] = 0x02; // Payload 长度
    frame[2] = CMD_LED_CONTROL; 
    frame[3] = 0x01; //定死的用户ID
    frame[4] = (uint8_t)state; // LED 状态
    frame[5] = calc_crc8(&frame[1], 4);// 计算 CRC8，参与校验的字段是 Length + Cmd_ID + User_ID + Payload = 2 + 1 + 1 + 1 = 5 字节
    frame[6] = FRAME_TAIL;
    app_uart_send_data(frame, 7);
        /*向云端同步*/
    /* LED 同步代码修复 (蜂鸣器同理，只需改键值 BAZZER_STATUS) */
char report_json[128];
snprintf(report_json, sizeof(report_json),
         "{"
         "\"id\":\"%lu\","
         "\"version\":\"1.0\","
         "\"params\":{"
         "\"LED_STATUS\":{"
         "\"value\":%s"
         "}"
         "}"
         "}",
         report_id++,
         state ? "true" : "false");
mymqtt_publish_data(TOPIC_POST, report_json);   
  
}

/* 蜂鸣器控制 */
void parse_buzzer_control(cJSON *content)
{
    int state = 0;
static uint32_t report_id = 1;
    // 1. 完美兼容：如果是布尔型 (true/false)
    if (cJSON_IsBool(content)) {
        state = cJSON_IsTrue(content) ? 1 : 0;
    }
    // 2. 完美兼容：如果云端改发数字 (1/0)
    else if (cJSON_IsNumber(content)) {
        state = content->valueint;
    }

    ESP_LOGI(TAG, "解析到蜂鸣器控制指令: %d", state);

    // 后面打包发送给下位机（STM32/CH32）的串口逻辑保持不变
    uint8_t frame[7];
    frame[0] = FRAME_HEADER;
    frame[1] = 0x02; 
    frame[2] = CMD_BUZZER_CONTROL; // 你的蜂鸣器命令ID
    frame[3] = 0x01; 
    frame[4] = (uint8_t)state;     // 此时 state 就能正确对应 1 或 0 了
    frame[5] = calc_crc8(&frame[1], 4);
    frame[6] = FRAME_TAIL;
    
    app_uart_send_data(frame, 7);
    /*向云端同步*/
    char report_json[128];
    snprintf(report_json, sizeof(report_json),
         "{"
         "\"id\":\"%lu\","
         "\"version\":\"1.0\","
         "\"params\":{"
         "\"BAZZER_STATUS\":{"
         "\"value\":%s"
         "}"
         "}"
         "}",
         report_id++,
         state ? "true" : "false");   
    mymqtt_publish_data(TOPIC_POST, report_json);
}

/* 温度阈值设置 */
void parse_set_temp_threshold(cJSON *content)
{
    static uint32_t report_id = 1;
     float threshold = cJSON_IsNumber(content) ? content->valuedouble : 0.0f;
     temp_threshold = (uint32_t)(threshold); // 转换为整数形式，单位是0.01度
    ESP_LOGI(TAG, "解析到温度阈值设置指令: %.1f", threshold);
     /*向云端同步*/
char report_json[128];

snprintf(report_json, sizeof(report_json),
         "{"
         "\"id\":\"%lu\","
         "\"version\":\"1.0\","
         "\"params\":{"
         "\"Temperature_Threshold\":{"
         "\"value\":%.1f"
         "}"
         "}"
         "}",
         report_id++,
         threshold);

mymqtt_publish_data(TOPIC_POST, report_json);
}

/* 湿度阈值设置 */
void parse_set_humi_threshold(cJSON *content)
{
    static uint32_t report_id = 1;
   float threshold = cJSON_IsNumber(content) ? content->valuedouble : 0.0f;
   humi_threshold = (uint32_t)(threshold); // 转换为整数形式，单位是0.01%
    ESP_LOGI(TAG, "解析到湿度阈值设置指令: %.1f", threshold);
     /*向云端同步*/
char report_json[128];

snprintf(report_json, sizeof(report_json),
         "{"
         "\"id\":\"%lu\","
         "\"version\":\"1.0\","
         "\"params\":{"
         "\"Humidity_Threshold\":{"
         "\"value\":%.1f"
         "}"
         "}"
         "}",
         report_id++,
         threshold);

mymqtt_publish_data(TOPIC_POST, report_json);
}