#ifndef _BSP_WIFI_BRIDGE_H_
#define _BSP_WIFI_BRIDGE_H_

#include <stdint.h>
#include <stdbool.h>


#define MAX_CALLBACKS 20 // 系统最多支持注册的云端指令监听器数量

typedef enum {
    /* =======================================================
     * 1. 基础配置类指令 (0x01 - 0x0F)
     * 负责设备的基础运行状态同步
     * ======================================================= */
    CMD_UPDATE_CLOCK            = 0x01, // 同步RTC时间戳 (载荷: Unix Timestamp)
    CMD_SYNC_DATA_REQ           = 0x02, // 强制要求设备上报当前所有状态 (电量, 药仓余量等)
    CMD_SET_VOLUME              = 0x03, // 设置语音播报音量 (载荷: 1-10级)

    /* =======================================================
     * 2. 用药管理类指令 (0x10 - 0x1F)
     * 负责核心的“用药”参数配置与远程干预
     * ======================================================= */
    CMD_SET_MEDICATION_PLAN     = 0x10, // 下发完整用药计划 (包含闹钟时间、对应药仓号、剂量)
    CMD_CLEAR_MEDICATION_PLAN   = 0x11, // 清空某用户的用药计划
    CMD_FORCE_OPEN_BOX          = 0x12, // 远程强制弹出指定药仓 (紧急干预/家属远程发药)
    CMD_PLAY_VOICE_PROMPT       = 0x13, // 远程触发语音播报 (如家属远程语音催服)

    /* =======================================================
     * 3. 健康监测类指令 (0x20 - 0x2F)
     * 针对心率、血氧等多模态传感器的动态调参
     * ======================================================= */
    CMD_SET_MONITOR_WINDOW      = 0x20, // 设置服药后的重点监测时长 (例如：服药后30分钟内开启高频监测)
    CMD_SET_ALERT_THRESHOLD     = 0x21, // 设置生理指标报警阈值 (如：心率上限、血氧下限，因人而异)
    CMD_FORCE_START_MONITOR     = 0x22, // 医生/家属远程强制启动一次体征测量

    /* =======================================================
     * 4. 多用户与权限类指令 (0x30 - 0x3F)
     * 专门针对 K210 视觉模块和多用户管理的指令
     * ======================================================= */
    CMD_K210_ENTER_ENROLL_MODE  = 0x30, // 下发指令让K210进入“录入人脸”模式 (绑定新用户)
    CMD_K210_DELETE_FACE        = 0x31, // 删除指定用户的人脸特征数据
    CMD_SET_USER_RELATION       = 0x32, // 绑定用户ID与对应药仓的关系 (如: 张三=1号仓)

    /* =======================================================
     * 5. 系统运维类指令 (0xF0 - 0xFF)
     * 固件升级与设备维护
     * ======================================================= */
    CMD_REBOOT_DEVICE           = 0xF0, // 远程重启 CH32V307 主控
    CMD_OTA_UPDATE_PREPARE      = 0xF1  // 下发 OTA 升级固件的下载地址和校验码

} cloud_cmd_type_t;

//上行命令类型定义/* =======================================================
typedef enum {
    // 1. 健康体征数据 (0x10 - 0x1F)
    UPLOAD_TYPE_HEART_RATE      = 0x10, // 上报心率/血氧数据
    UPLOAD_TYPE_BODY_TEMP       = 0x11, // 上报体温数据
    UPLOAD_TYPE_HUMI_TEMP       = 0x12, // 上报温湿度数据 (来自独立的温湿度传感器)
    

    // 2. 用药行为事件 (0x20 - 0x2F)
    UPLOAD_EVENT_PILL_TAKEN     = 0x20, // 正常服药打卡 (载荷: 用户ID + 药仓号 + 时间)
    UPLOAD_EVENT_PILL_MISSED    = 0x21, // 漏服药警报 (触发云端发短信给家属)
    UPLOAD_EVENT_WRONG_PILL     = 0x22, // 误服药警报 (K210识别出非该药仓主人开盒)

    // 3. 视觉与交互事件 (0x30 - 0x3F)
    UPLOAD_EVENT_FACE_RECOGNIZED= 0x30, // K210 识别到人脸 (载荷: 用户ID)
    UPLOAD_EVENT_FACE_ENROLLED  = 0x31, // K210 新人脸录入成功上报

    // 4. 设备运行状态 (0xF0 - 0xFF)
    UPLOAD_STATUS_BATTERY       = 0xF0, // 定时上报当前电量
    UPLOAD_STATUS_BOX_EMPTY     = 0xF1  // 药仓缺药警报 (提醒家属补充)
} upload_data_type_t;

// 云端指令回调函数类型定义
typedef void (*cloud_cmd_callback_t)(cloud_cmd_type_t cmd, const uint8_t *payload, uint16_t len);

// 回调注册表节点结构体
typedef struct {
    cloud_cmd_type_t cmd;     // 监听的指令类型
    cloud_cmd_callback_t cb;  // 对应的回调函数
    bool is_used;             // 该槽位是否被占用
} bridge_cb_node_t;

//bridge 层的 API 定义
int bsp_wifi_bridge_init(void);

//下行云端指令注册接口，供业务层调用
int bsp_wifi_bridge_register_callback(cloud_cmd_type_t cmd, cloud_cmd_callback_t cb);

//上传云端数据的接口，供串口或业务层调用
int bsp_wifi_bridge_publish_raw(upload_data_type_t sensor_id, const uint8_t *data, uint16_t len);

#endif // _BSP_WIFI_BRIDGE_H_