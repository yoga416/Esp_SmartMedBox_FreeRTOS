#ifndef _MYMQTT_H_
#define _MYMQTT_H_

#include <stdint.h>
#include <stdbool.h>


#define TOPIC_RELAY            "$sys/0124xQpA1l/esp32_526/thing/property/relay" // 替换为实际的控制指令主题
#define TOPIC_SET             "$sys/0124xQpA1l/esp32_526/thing/property/set"
#define TOPIC_POST          "$sys/0124xQpA1l/esp32_526/thing/property/post"
#define TOPIC_POST_REPLY    "$sys/0124xQpA1l/esp32_526/thing/property/post/reply"


#define     URI            "mqtt://mqtts.heclouds.com:1883" // 替换为实际 MQTT 服务器地址
#define     USENAME        "0124xQpA1l" // 所属产品/产品ID(0124xQpA1l)，
#define     CLIENT_ID      "esp32_526" // 设备名称/ID
#define     PASSWORD       "version=2018-10-31&res=products%2F0124xQpA1l%2Fdevices%2Fesp32_526&et=1910271117&method=md5&sign=LjfT94Xmd7V5bIbP4iQNhw%3D%3D" 


static const char test_data[] = "{"
    "\"id\": \"123\","
    "\"version\": \"1.0\","
    "\"params\": {"
        "\"ambient_humi\": {"
            "\"value\": 32.2"
        "}"
    "}"
"}";

#define     PASSWORD2_esp32_526      "version=2018-10-31&res=products%2F0124xQpA1l%2Fdevices%2Fesp32_526&et=1910271117&method=md5&sign=caqjMLN3psU9Z8R%2B6otedg%3D%3D"

 /* @brief 初始化 MQTT 客户端并连接服务器
 * @param broker_url MQTT 服务器地址，例如 "mqtt://192.168.1.100:1883"
 */
void mymqtt_init();

/**
 * @brief 发送(发布) MQTT 消息
 * @param topic 主题
 * @param payload 消息内容（比如 JSON 字符串）
 * @return true-成功，false-失败或未连接
 */
bool mymqtt_publish_data(const char *topic, const char *payload);

#endif // _MYMQTT_H_