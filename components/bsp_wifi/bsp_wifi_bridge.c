
#include "bsp_wifi_bridge.h"
#include "bsp_wifi_handler.h" 
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"          

static const char *TAG = "WIFI_BRIDGE";

// 回调注册表（在 .c 中定义，避免头文件中 static 导致多份副本）
static bridge_cb_node_t s_cb_table[MAX_CALLBACKS];

// ==========================================================
// 2. 桥接层初始化与配置 API
// ==========================================================

int bsp_wifi_bridge_init(void) {
    // 清空回调注册表，防止有野指针
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        s_cb_table[i].is_used = false;
        s_cb_table[i].cb = NULL;
    }
    ESP_LOGI(TAG, "Wi-Fi 桥接层初始化完成");
    return 0;
}

int bsp_wifi_bridge_register_callback(cloud_cmd_type_t cmd, cloud_cmd_callback_t cb) {
    if (cb == NULL) return -1;

    // 1. 检查是否已经注册过该指令，如果注册过则覆盖
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (s_cb_table[i].is_used && s_cb_table[i].cmd == cmd) {
            s_cb_table[i].cb = cb;
            return 0;
        }
    }

    // 2. 寻找空闲槽位进行注册
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (!s_cb_table[i].is_used) {
            s_cb_table[i].cmd = cmd;
            s_cb_table[i].cb = cb;
            s_cb_table[i].is_used = true;
            ESP_LOGI(TAG, "成功注册云端指令监听: CMD=0x%02X", cmd);
            return 0;
        }
    }

    ESP_LOGE(TAG, "注册表已满，无法注册新的监听器！");
    return -1;
}


// ==========================================================
// 3. 上行链路：终极透传上报 API (供串口或业务层调用)
// ==========================================================

int bsp_wifi_bridge_publish_raw(upload_data_type_t sensor_id, const uint8_t *data, uint16_t len) {
    // 安全性检查
    if (data == NULL || len == 0) {
        ESP_LOGE(TAG, "上报数据为空或长度为0");
        return -1;
    }

    // 定义内部消息“信封” (依赖 bsp_wifi_handler.h 中的定义)
    internal_raw_msg_t msg;
    msg.sensor_id = sensor_id;
    msg.len = len;
    
    // 【核心安全设计】：深拷贝，使用动态内存分配
    msg.payload = (uint8_t *)malloc(len);
    if (msg.payload == NULL) {
        ESP_LOGE(TAG, "Bridge 层 malloc 失败！系统内存不足，丢弃数据帧");
        return -1; // 内存分配失败，直接放弃
    }

    // 将外部易失的数据，拷贝到安全的专属堆内存中
    memcpy(msg.payload, data, len);

    // 调用 Handler 层的接口，将包含新指针的信封塞入底层发送队列
    // 假设 bsp_wifi_handler_send_msg 成功返回 0
    if (bsp_wifi_handler_send_msg(&msg) != 0) {
        ESP_LOGW(TAG, "底层发送队列已满，丢弃该数据并释放内存");
        // 【关键防漏】：如果队列满了放不进去，必须立刻释放刚才申请的内存！
        free(msg.payload); 
        return -1;
    }

    return 0; // 成功！内存将由消费者(Handler的MQTT任务)负责释放
}


// ==========================================================
// 4. 下行链路：内部指令分发 (仅供 Handler 层收到 MQTT 数据后调用)
// ==========================================================

/**
 * @brief Handler 层解析完云端 MQTT JSON 后，调用此函数触发业务层动作
 * @note  这个函数可以声明在 bsp_wifi_bridge.h 的末尾，或者放在一个内部专用的头文件中
 */
void bsp_wifi_bridge_dispatch_cloud_cmd(cloud_cmd_type_t cmd, const uint8_t *payload, uint16_t len) {
    bool is_handled = false;

    // 遍历注册表，寻找订阅了此指令的业务回调
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (s_cb_table[i].is_used && s_cb_table[i].cmd == cmd) {
            if (s_cb_table[i].cb != NULL) {
                // 触发回调函数，将数据抛给上层应用（比如电机控制逻辑）
                s_cb_table[i].cb(cmd, payload, len);
                is_handled = true;
            }
        }
    }

    if (!is_handled) {
        ESP_LOGW(TAG, "收到云端指令 0x%02X,但没有业务层注册该指令的监听器", cmd);
    }
}