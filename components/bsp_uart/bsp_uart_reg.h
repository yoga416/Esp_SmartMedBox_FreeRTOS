#ifndef _BSP_UART_REG_H_
#define _BSP_UART_REG_H_

#include "driver/gpio.h"  // <--- 增加这一行，解决 GPIO 未声明的问题

#define UART_DEBUG_ENABLE  /**< UART 驱动调试开关，定义后会输出调试日志 */

//uart的引脚定义
#define UART_TX_PIN GPIO_NUM_18
#define UART_RX_PIN GPIO_NUM_17

//uart的波特率
#define UART_BAUD_RATE 115200

//uart的缓冲区大小
#define UART_RX_BUFFER_SIZE 1024
#define UART_TX_BUFFER_SIZE 1024

//数据包的结构
#define PACKET_HEAD_VAL  0x5A
#define PACKET_TAIL_VAL  0xFF

//状态机的枚举
typedef enum{
      PACKET_HEAD ,
      PACKET_LENGTH,
      PACKET_SENSOR_ID,
      PACKET_DATA,
      PACKET_CRC,
      PACKET_TAIL,
      PACKET_WAIT_HEAD,
      PACKET_WAIT_TAIL
}interal_packet_state_t;


#endif // _BSP_UART_REG_H_
