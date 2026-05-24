#include "bsp_wifi_driver.h"
#include "bsp_wifi_port.h"
#include "bsp_wifi_os_layer.h"
#include "bsp_wifi_reg.h"
//define 
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// WiFi 驱动实现(int函数实现)
static wifi_state_t wifi_driver_init(bsp_wifi_driver_t *self, wifi_config_instance_t *config)
{
      if (self == NULL || config == NULL) {
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_driver_init] INPUT NULL PTR!\n");
#endif      
        return WIFI_DRIVER_NULL_PTR;
    }
   
    wifi_state_t RET=WIFI_DRIVER_OK;
     // 调用底层硬件接口进行初始化
     self->wifi_handler=NULL;
     RET=self->port_instance.hw_init(config->ssid, config->password,
                                     config->channel,
                                     config->max_connection,
                                     config->authmode,
                                     &self->wifi_handler);
      if(RET!=WIFI_DRIVER_OK)
      {
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_driver_init] HARDWARE INIT FAILED!\n");
#endif
            return WIFI_DRIVER_ERROR;
      }
      return WIFI_DRIVER_OK;
}

//wifi驱动（deinit)函数实现
static wifi_state_t wifi_driver_deinit(bsp_wifi_driver_t *self)
{
      if (self == NULL || self->wifi_handler == NULL) {
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_driver_deinit] INPUT NULL PTR!\n");
#endif
            return WIFI_DRIVER_NULL_PTR;
      }

      // 调用底层硬件接口进行反初始化
      wifi_state_t RET = self->port_instance.hw_deinit(self->wifi_handler);
      if (RET != WIFI_DRIVER_OK) {
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_driver_deinit] HARDWARE DEINIT FAILED!\n");
#endif
            return WIFI_DRIVER_ERROR;
      }
      self->wifi_handler = NULL;
      return WIFI_DRIVER_OK;
}

//wifi驱动（send)函数实现
static wifi_state_t wifi_driver_send(bsp_wifi_driver_t *self,
                                     const uint8_t *data,
                                     uint32_t len)
{
      if (self == NULL || self->wifi_handler == NULL || data == NULL) { 
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_driver_send] INPUT NULL PTR!\n");
#endif
            return WIFI_DRIVER_NULL_PTR;
      }     
      // 调用底层硬件接口进行数据发送
      wifi_state_t RET = self->port_instance.hw_send(self->wifi_handler, data, len);      
      if (RET != WIFI_DRIVER_OK) {
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_driver_send] HARDWARE SEND FAILED!\n");
#endif      
            return WIFI_DRIVER_ERROR;
      }
      return WIFI_DRIVER_OK;
}

//wifi驱动（receive)函数实现
static int8_t wifi_driver_receive(bsp_wifi_driver_t *self,
                                                  uint8_t *out_data,
                                                  uint32_t max_len,
                                                  uint32_t timeout_ms)
{
      if (self == NULL || self->wifi_handler == NULL || out_data == NULL) {
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_driver_receive] INPUT NULL PTR!\n");
#endif
            return WIFI_DRIVER_NULL_PTR;
      }
      // 调用底层硬件接口进行数据接收
      int8_t RET = self->port_instance.hw_receive(self->wifi_handler,
                                                  out_data,
                                                  max_len,
                                                  timeout_ms);
      if (RET < 0) {
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_driver_receive] HARDWARE RECEIVE FAILED! RET=%d\n", RET); 
#endif
            return RET; // 直接返回底层错误码
      }
      return RET; // 返回实际接收的数据长度
}

//wifi驱动（connect)函数实现
static wifi_state_t wifi_driver_connect(bsp_wifi_driver_t *self,
                                       const char *ssid,
                                       const char *password)
{
      if (self == NULL || self->wifi_handler == NULL || ssid == NULL || password == NULL) {     
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_driver_connect] INPUT NULL PTR!\n");    
#endif      
            return WIFI_DRIVER_NULL_PTR;
      }
      // 调用底层硬件接口进行连接
      wifi_state_t RET = self->port_instance.hw_connect(self->wifi_handler, ssid, password);
      if (RET != WIFI_DRIVER_OK) {  
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_driver_connect] HARDWARE CONNECT FAILED!\n"); 
#endif
            return WIFI_DRIVER_ERROR;     
      }
      return WIFI_DRIVER_OK;
}     


//wifi驱动（disconnect)函数实现
static wifi_state_t wifi_driver_disconnect(bsp_wifi_driver_t *self)
{
      if (self == NULL || self->wifi_handler == NULL) {     
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_driver_disconnect] INPUT NULL PTR!\n"); 
#endif
            return WIFI_DRIVER_NULL_PTR;  
      }           
      // 调用底层硬件接口进行断开连接     
      wifi_state_t RET = self->port_instance.hw_disconnect(self->wifi_handler);
      if (RET != WIFI_DRIVER_OK) {
#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_driver_disconnect] HARDWARE DISCONNECT FAILED!\n");
#endif      
            return WIFI_DRIVER_ERROR;     
      }
      return WIFI_DRIVER_OK;
}

//构造函数：初始化驱动对象并关联 OS 接口
wifi_state_t wifi_inst(bsp_wifi_driver_t *driver,
                              wifi_port_instance_t *port_ops,
                              wifi_timebase_os_t time_ops)
{
      if (driver == NULL || port_ops == NULL) {


#ifdef WIFI_DEBUG_ENABLE
            printf("[wifi_driver_inst] INPUT NULL PTR!\n"); 
#endif
            return WIFI_DRIVER_NULL_PTR;
      }
      // 初始化属性
      driver->wifi_handler = NULL;
      driver->port_instance = *port_ops;
      driver->timebase_instance = time_ops;
      // 挂载方法
      driver->pfinit = wifi_driver_init;
      driver->deinit = wifi_driver_deinit;
      driver->send = wifi_driver_send;
      driver->receive = wifi_driver_receive;
      driver->pfconnect = wifi_driver_connect;
      driver->pfdisconnect = wifi_driver_disconnect;
      return WIFI_DRIVER_OK;
}

