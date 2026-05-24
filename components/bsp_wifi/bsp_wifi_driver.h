#ifndef _BSP_WIFI_DRIVER_H_
#define _BSP_WIFI_DRIVER_H_

#include "bsp_wifi_reg.h"
#include "bsp_wifi_port.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>


      

//前置声明
typedef struct bsp_wifi_driver_t bsp_wifi_driver_t;

//config
typedef  struct {
      const char *ssid;
      const char *password;
      int channel;
      int max_connection;
      int authmode;
} wifi_config_instance_t;



//时间基准
typedef struct
{
      uint32_t (*pfget_count)(void);
      void (*pfdelay_ms)(uint32_t ms);
}wifi_timebase_os_t;

//WIFI状态
typedef enum{
      WIFI_DRIVER_OK,
      WIFI_DRIVER_ERROR,
      WIFI_DRIVER_TIMEOUT,
      WIFI_DRIVER_NULL_PTR
}wifi_state_t;

//驱动结构体定义

struct bsp_wifi_driver_t {

      //返回wifi句柄
      void *wifi_handler;
      wifi_timebase_os_t timebase_instance;
      wifi_port_instance_t port_instance;

      //函数
      wifi_state_t      (*pfinit)        (bsp_wifi_driver_t *self, wifi_config_instance_t *wifi_config);
      wifi_state_t      (*deinit)        (bsp_wifi_driver_t *self);
      wifi_state_t      (*send)          (bsp_wifi_driver_t *self,
                                          const uint8_t *data,
                                          const uint32_t data_len);
      int8_t            (*receive)       (bsp_wifi_driver_t *self,
                                          uint8_t *out_data,
                                          uint32_t max_len,
                                          uint32_t timeout_ms);
      wifi_state_t      (*pfconnect)     (bsp_wifi_driver_t *self,
                                          const char *ssid,
                                          const char * password);
      wifi_state_t      (*pfdisconnect)  (bsp_wifi_driver_t *self);
};

//public api

wifi_state_t  wifi_inst(bsp_wifi_driver_t       *self,
                        wifi_port_instance_t    *port_instance,
                        wifi_timebase_os_t      timebase_instance);
#endif // _BSP_WIFI_DRIVER_H_
