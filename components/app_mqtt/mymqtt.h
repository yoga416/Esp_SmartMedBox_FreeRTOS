#ifndef _MYMQTT_H_
#define _MYMQTT_H_

#include <stdint.h>
#include <stdbool.h>


#define TOPIC_RELAY            "$sys/4N7BMwx111/ESP32_32/thing/property/relay" // 替换为实际的控制指令主题
#define TOPIC_SET             "$sys/4N7BMwx111/ESP32_32/thing/property/set"
#define TOPIC_POST          "$sys/4N7BMwx111/ESP32_32/thing/property/post"


#define     URI            "mqtt://mqtts.heclouds.com:1883" // 替换为实际 MQTT 服务器地址
#define     USENAME        "4N7BMwx111" // 所属产品/产品ID(4N7BMwx111)，
#define     CLIENT_ID      "ESP32_32" // 设备名称/ID
#define     PASSWORD       "version=2018-10-31&res=products%2F4N7BMwx111%2Fdevices%2FESP32_32&et=1910271117&method=md5&sign=LjfT94Xmd7V5bIbP4iQNhw%3D%3D" 


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