// test_uart_handler/bsp_utils.h 的内容
#ifndef _BSP_UTILS_H_
#define _BSP_UTILS_H_
#include <stdint.h>
uint8_t bsp_utils_calc_crc8(const uint8_t *data, uint16_t len);
#endif