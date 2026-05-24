#ifndef _BSP_WIFI_HANDLER_H_
#define _BSP_WIFI_HANDLER_H_

#include <stdint.h>

typedef enum{
    WIFI_HANDLER_OK = 0,
    WIFI_HANDLER_ERR = -1
}bsp_wifi_handler_status_t;

typedef struct {
    uint8_t sensor_id;  // 传感器类型
    uint16_t len;       // 数据长度
    uint8_t *payload;   // 指向由 Bridge 层 malloc 出来的内存
} internal_raw_msg_t;


/* ==========================================================
 * 暴露给 Bridge 层的 API
 * ========================================================== */

/**
 * @brief 将带有动态内存指针的数据塞入网络发送队列
 * @param msg 包含指针的内部消息体
 * @return 0 成功，-1 失败（如队列已满）
 */
int bsp_wifi_handler_send_msg(internal_raw_msg_t *msg);


/* ==========================================================
 * 暴露给主程序的 API
 * ========================================================== */

/**
 * @brief 初始化网络处理层（创建队列、启动 MQTT 后台任务）
 * @return 0 成功
 */
int bsp_wifi_handler_init(void);

#endif // _BSP_WIFI_HANDLER_H_