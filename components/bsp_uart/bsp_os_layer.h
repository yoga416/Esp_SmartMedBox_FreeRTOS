#ifndef _BSP_OS_LAYER_H_
#define _BSP_OS_LAYER_H_

#include "bsp_uart_handler.h" // 引用你定义的 ops 结构体类型

// 导出 OS 实例，供 main.c 组装使用
extern timebase_os_t    os_time_ops;
extern uart_queue_ops_t os_queue_ops;

#endif