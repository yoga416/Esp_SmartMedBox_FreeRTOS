#ifndef _SENSOR_PARSER_H_
#define _SENSOR_PARSER_H_

#include <stdint.h>
#include "cJSON.h"

// ==========================================
// 🚀 协议常量与传感器 ID 定义
// ==========================================
#define FRAME_HEADER 0x5A
#define FRAME_TAIL   0xFF

#define SENSOR_SHT40    0x00
#define SENSOR_MLX90614 0x03
#define SENSOR_MAX30102 0x04
#define CMD_RTC_SYNC    0x10
#define CMD_WEATHER_SYNC 0x11
#define CMD_LOCATION_SYNC 0x12
#define CMD_WIFI_STATUS   0x13
#define CMD_MED_SCHEDULE_SET 0x22
#define CMD_LED_CONTROL 0x23
#define CMD_BUZZER_CONTROL 0x24

uint8_t calc_crc8(const uint8_t *data, uint16_t len) ;
void parse_sensor_frame(const uint8_t *buffer, uint16_t len) ;
void parse_medication_schedule_1(cJSON *value_arr);
void parse_medication_schedule_2(cJSON *value_arr);
void parse_medication_schedule_3(cJSON *value_arr);
void parse_led_control(cJSON *value_obj);
void parse_bazzer_control(cJSON *value_obj);
void parse_set_temp_threshold(cJSON *value_obj);
void parse_set_humi_threshold(cJSON *value_obj);
void parse_buzzer_control(cJSON *content);
#endif /* _SENSOR_PARSER_H_ */