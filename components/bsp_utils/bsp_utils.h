#ifndef _BSP_UTILS_H_
#define _BSP_UTILS_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @brief 计算 CRC-8 校验码
 * @note  多项式: 0x07 (x^8 + x^2 + x + 1), 初始值: 0x00
 * @param data  待校验的数据缓冲区指针
 * @param len   数据长度（字节数）
 * @return uint8_t 计算得到的 8 位校验值
 */
uint8_t bsp_utils_calc_crc8(const uint8_t *data, uint16_t len);

#endif // _BSP_UTILS_H_
