#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "sensor_parser.h"
#include "mymqtt.h"
#include "mqtt_client.h"
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
#define CMD_MED_SCHEDULE_UPLOAD 0x21 // 新增：服药计划上传
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
                        float hum = raw_hum / 100.0f;
                        ESP_LOGI(TAG, "✅ [User %d] 环境温湿度: %.2fC, %.2f%%", user_id, temp, hum);
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
                            "}", report_id++,temp, hum);
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
                            "}", report_id++,temp, hum);
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
                            "}", report_id++,temp, hum);
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
                        else  prop_name = "mechine_time_3";

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