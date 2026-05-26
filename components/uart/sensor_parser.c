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

#define SENSOR_SHT40    0x00
#define SENSOR_MLX90614 0x03
#define SENSOR_MAX30102 0x04

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
    char report_json[256];
    static uint32_t report_id = 1; // 增加自增 ID，确保每条消息 ID 唯一

    // 遍历缓冲区寻找合法帧
    for (uint16_t i = 0; i < len - 4; i++) {
        
        // 1. 寻找帧头 0x5A
        if (buffer[i] == FRAME_HEADER) {
            uint8_t payload_len = buffer[i + 1];
            uint16_t frame_total_len = payload_len + 5; // 计算当前完整帧的总长度

            // 如果剩余数据长度不够一个完整帧，退出等待下一次数据到来
            if (i + frame_total_len > len) {
                break; 
            }

            // 2. 检查帧尾 0xFF
            if (buffer[i + frame_total_len - 1] != FRAME_TAIL) {
                continue; // 帧尾不对，可能是伪造的帧头，继续往后扫描
            }

            uint8_t expected_crc = buffer[i + payload_len + 3];
            uint8_t calc_crc = calc_crc8(&buffer[i + 1], payload_len + 2);

            if (expected_crc != calc_crc) {
                ESP_LOGW(TAG, "CRC校验失败! 计算值:0x%02X, 期望值:0x%02X", calc_crc, expected_crc);
                continue; 
            }

            // 4. 解析有效载荷 (Payload)
            uint8_t sensor_id = buffer[i + 2];
            const uint8_t *payload = &buffer[i + 3];

            switch (sensor_id) {
                case SENSOR_SHT40: {
                    if (payload_len == 8) {
                        int32_t raw_temp = 0, raw_hum = 0;
                        memcpy(&raw_temp, payload, 4);
                        memcpy(&raw_hum, payload + 4, 4);
                        
                        float temp = raw_temp / 100.0f;
                        float hum = raw_hum / 100.0f;
                        ESP_LOGI(TAG, "✅ [SHT40] 环境温湿度: %.2fC, %.2f%%", temp, hum);

                        /*上传温度 */
                        sprintf(report_json, 
                                "{"
                            "\"id\": \"%lu\","
                            "\"version\": \"1.0\","
                            "\"params\": {"
                            "\"ambient_temp\": {"
                            "\"value\": %.1f"
                            "}"
                            "}"
                            "}", 
                                report_id++, temp);
                        mymqtt_publish_data(TOPIC_POST, report_json);

                         /*上传湿度 */
                        sprintf(report_json, 
                                "{"
                            "\"id\": \"%lu\","
                            "\"version\": \"1.0\","
                            "\"params\": {"
                            "\"ambient_humi\": {"
                            "\"value\": %.1f"
                            "}"
                            "}"
                            "}", 
                                report_id++, hum);
                        mymqtt_publish_data(TOPIC_POST, report_json);
                    }
                    break;
                }
                
                case SENSOR_MLX90614: {
                    if (payload_len == 8) {
                        int32_t raw_obj = 0;
                        // 这里我们只关心物体温度（人体温度）
                        memcpy(&raw_obj, payload + 4, 4);
                        
                        float obj_temp = raw_obj / 100.0f;
                        ESP_LOGI(TAG, "✅ [MLX90614] 体温: %.2fC", obj_temp);

                       /*上传体温 */
                        sprintf(report_json, 
                                "{"
                            "\"id\": \"%lu\","
                            "\"version\": \"1.0\","
                            "\"params\": {"
                            "\"body_temp\": {"
                            "\"value\": %.1f"
                            "}"
                            "}"
                            "}", 
                                report_id++, obj_temp);
                        mymqtt_publish_data(TOPIC_POST, report_json);
                        ESP_LOGI(TAG, "数据已上传到 MQTT 服务器: %s", report_json);
                    }
                    break;
                }
                
                case SENSOR_MAX30102: {
                    if (payload_len == 4) {
                        uint16_t raw_hr = 0, raw_spo2 = 0;
                        memcpy(&raw_hr, payload, 2);
                        memcpy(&raw_spo2, payload + 2, 2);
                        
                        float hr = raw_hr / 100.0f;
                        float spo2 = raw_spo2 / 100.0f;
                        ESP_LOGI(TAG, "✅ [MAX30102] 心率: %.2f bpm, 血氧: %.2f %%", hr, spo2);

                        /*上传心率 */
                        sprintf(report_json, 
                                "{"
                            "\"id\": \"%lu\","
                            "\"version\": \"1.0\","
                            "\"params\": {"
                            "\"heart_rate\": {"
                            "\"value\": %ld"
                            "}"
                            "}"
                            "}", 
                        report_id++, (long)hr);
                        mymqtt_publish_data(TOPIC_POST, report_json);

                        /*上传血氧 */
                        sprintf(report_json, 
                                "{"
                            "\"id\": \"%lu\","
                            "\"version\": \"1.0\","
                            "\"params\": {"
                            "\"spo2\": {"
                            "\"value\": %ld"
                            "}"
                            "}"
                            "}", 
                                report_id++, (long)spo2);
                        mymqtt_publish_data(TOPIC_POST, report_json);
                    }
                    break;
                }
                
                default:
                    ESP_LOGW(TAG, "未知的传感器 ID: 0x%02X", sensor_id);
                    break;
            }

            // 一帧解析成功，将循环索引推移到该帧尾部，避免重复解析
            i += frame_total_len - 1; 
        }
    }
}