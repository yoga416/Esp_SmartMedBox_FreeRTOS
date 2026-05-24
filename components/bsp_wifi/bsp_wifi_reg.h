#ifndef _BSP_WIFI_REG_H_
#define _BSP_WIFI_REG_H_

#include <stdint.h>
#include <stddef.h>

#define WIFI_DEBUG_ENABLE 

//数据包的结构
#define UART_FRAME_HEADER  0x5A
#define UART_FRAME_TAIL    0xFF
#define UART_DOWNLINK_MAX_PAYLOAD 16

/**
 * @brief 下行帧结构（发往 CH32）
 * @note  使用 __attribute__((packed)) 确保内存布局紧致，与串口协议一致
 *        总帧长 = 5(header+cmd+len+checksum+tail) + payload实际长度
 */
typedef struct __attribute__((packed)) {
    uint8_t header;                    /* 帧头 0x5A                */
    uint8_t cmd;                       /* 指令码                    */
    uint8_t len;                       /* 有效载荷长度              */
    uint8_t payload[UART_DOWNLINK_MAX_PAYLOAD]; /* 有效载荷缓冲区 */
    uint8_t checksum;                  /* CRC-8 校验码             */
    uint8_t tail;                      /* 帧尾 0xFF                */
} uart_downlink_frame_t;

/**
 * @brief 计算下行帧的 CRC-8 校验和
 * @param frame  指向下行帧结构体的指针
 * @return uint8_t 校验值
 */
static inline uint8_t calculate_checksum(const uart_downlink_frame_t *frame) {
    // 计算范围: header + cmd + len + payload(实际长度)
    // CRC 多项式 0x07, 使用 bsp_utils 中的函数
    extern uint8_t bsp_utils_calc_crc8(const uint8_t *data, uint16_t len);
    return bsp_utils_calc_crc8((const uint8_t *)frame, 
                                offsetof(uart_downlink_frame_t, checksum));
}

/**
 * @brief 通过 UART 将数据发送给 CH32（需由应用层提供具体实现）
 */
extern void uart_send_to_ch32(const uint8_t *data, size_t len);

#endif // _BSP_WIFI_REG_H_
