#include "bsp_uart_handler.h"
#include  "bsp_uart_reg.h"
 #include "bsp_utils.h"
#include <esp_log.h>

#define TAG "bsp_uart_handler"
//define 
#define UART_HANDLER_INITED  0
#define UART_HANDLER_NOTINITED  -2

#define IS_INITED   ()

// uart_handler的初始化函数
//uart_handler_state_t (*init)(bsp_uart_handler_t *self, uart_handler_input_instance_t *input_instance);

static uart_handler_state_t uart_handler_init(bsp_uart_handler_t *self ,
                                    uart_handler_input_instance_t * input_instance)
      {

      //检查参数
      if(self==NULL||input_instance==NULL)
      {
 #ifdef HANDLER_DEBUG_ENABLE
            ESP_LOGI(TAG, "[uart_handler_init]Function pointer is null");
 #endif
            return UART_ERR_NULL_PTR;
      }
      //检查uart+driver是否被挂载
      if(input_instance->uart_driver==NULL)
      {
 #ifdef HANDLER_DEBUG_ENABLE
            ESP_LOGI(TAG, "[uart_handler_init]uart_driver is not implemented");
            return ERR_NOT_IMPLEMENTED;
 #endif     
      }
      //依赖注入
      self->queue_ops=input_instance->queue_ops;
      self->time_ops=input_instance->time_ops;
      self->uart_driver=input_instance->uart_driver;

      //信号继承
      self->event_queue_handle=input_instance->uart_driver->event_queue;

      self->rx_index=0;
      self->parse_state=PACKET_WAIT_HEAD;
      memset(self->rx_buffer,0,sizeof(self->rx_buffer));

      //回调函数
      self->frame_parsed_callback=NULL;
      

      //函数指针挂载
 #ifdef HANDLER_DEBUG_ENABLE
            ESP_LOGI(TAG, "[uart_handler_init]success to uart_handler_init");
 #endif 
      return UART_HANDLER_OK;
      }
//uart_handler的逆初始化函数
//static uart_handler_state_t (*deinit)(bsp_uart_handler_t *self);
static uart_handler_state_t uart_handler_deinit(bsp_uart_handler_t *self)
{
    // 1. 参数检查
    if (self == NULL)
    {
 #ifdef HANDLER_DEBUG_ENABLE
      ESP_LOGI(TAG, "[uart_handler_deinit] Function pointer is null");
 #endif  
        return UART_ERR_NULL_PTR; // 根据你之前的宏，这里返回错误码
    }

    // 2. 切断对外的联系（最重要的一步）
    // 防止设备已经被关闭，但底层还在尝试调用回调函数
    self->frame_parsed_callback = NULL;

    // 3. 内部记忆抹除（打扫卫生）
    self->rx_index = 0;
    self->parse_state = PACKET_WAIT_HEAD; // 恢复到初始状态
    memset(self->rx_buffer, 0, sizeof(self->rx_buffer));

    // 4. 断开依赖（归还工具）
    self->uart_driver = NULL;
    self->queue_ops = NULL;
    self->time_ops = NULL;
    self->event_queue_handle = NULL;

 #ifdef HANDLER_DEBUG_ENABLE
      ESP_LOGI(TAG, "[uart_handler_deinit] Deinit Success!");
 #endif 

    return UART_HANDLER_OK;
}
//uart_handler的回调函数
//uart_handler_state_t (*register_rx_callback)(bsp_uart_handler_t *self,
 //                              uart_frame_parsed_cb_t callback);
                               
static uart_handler_state_t register_rx_callback(bsp_uart_handler_t *self,
                               uart_frame_parsed_cb_t callback)
      {
            if(self==NULL||callback==NULL)
            {
 #ifdef HANDLER_DEBUG_ENABLE
            ESP_LOGI(TAG, "[uart_handler_callback] Function pointer is null!");
 #endif
                  return UART_ERR_NULL_PTR;
            }
      //函数注册
      self->frame_parsed_callback=callback;
      
      return UART_HANDLER_OK;
      }                               
                               
//uart_handler的发送函数

/*uart_handler_state_t (*send_frame)(bsp_uart_handler_t *self, 
                                    uint8_t sensor_id,
                                    const uint8_t *data, 
                                    uint16_t data_len);
};*/

//crc检验函数实现


 

static uart_handler_state_t uart_handler_send_frame(
                                    bsp_uart_handler_t *self, 
                                    uint8_t sensor_id,
                                    const uint8_t *data, 
                                    uint16_t data_len)
      {
      //参数检查
      uart_handler_state_t ret=UART_HANDLER_OK;
      if(self==NULL)
      {
 #ifdef HANDLER_DEBUG_ENABLE
            ESP_LOGI(TAG, "[uart_handler_send] Function pointer is null!");
 #endif    
            return UART_ERR_NULL_PTR;
      }
      
      uint8_t tx_temp_buffer[256];
      if(data_len+5>=sizeof(tx_temp_buffer))
      {
 #ifdef HANDLER_DEBUG_ENABLE
      ESP_LOGI(TAG, "failed to buffer is full");
 #endif
            return UART_HANDLER_ERROR;
      }

      //开始装填
      uint32_t buffer_index=0;

      tx_temp_buffer[buffer_index++]=PACKET_HEAD_VAL;//数据帧头
      tx_temp_buffer[buffer_index++]=(uint8_t )(data_len&0xff);
      tx_temp_buffer[buffer_index++]=sensor_id;
      
      if(data_len>0)
      {
      memcpy(&tx_temp_buffer[buffer_index],data,data_len );
      buffer_index+=data_len;
      }
      //crc检验算法
      tx_temp_buffer[buffer_index]=bsp_utils_calc_crc8(&tx_temp_buffer[1],
                                                      buffer_index-1);
      buffer_index++;
      tx_temp_buffer[buffer_index++]=PACKET_TAIL_VAL;

      //发送到缓冲区
      ret=self->uart_driver->send(self->uart_driver,
                              tx_temp_buffer,
                              buffer_index
                        );
            if(ret!=UART_HANDLER_OK)
            {
 #ifdef HANDLER_DEBUG_ENABLE
                  ESP_LOGI(TAG, "failed to send uart");
 #endif
                  return UART_HANDLER_ERROR;
            }

            return UART_HANDLER_OK;
      }

//uart_hanlder的构造函数
/*API 函数声明
uart_handler_state_t uart_handler_inst(
    bsp_uart_handler_t *handler_instance,
    uart_handler_input_instance_t *input_instance
);*/
// API 函数声明
uart_handler_state_t uart_handler_inst(
    bsp_uart_handler_t *handler_instance,
    uart_handler_input_instance_t *input_instance
)
{
      uart_handler_state_t ret=UART_HANDLER_OK;
      //参数检查
      if(NULL==handler_instance||
         NULL==input_instance)
         {
 #ifdef HANDLER_DEBUG_ENABLE
       ESP_LOGI(TAG, "[uart_handler_inst] Function pointer is null!");
 #endif
            return UART_ERR_NULL_PTR;
         }

      //内部函数挂载
      handler_instance->init=uart_handler_init;
      handler_instance->deinit=uart_handler_deinit;
      handler_instance->send_frame=uart_handler_send_frame;
      handler_instance->register_rx_callback=register_rx_callback;

      //input提供的函数挂载
      handler_instance->queue_ops=input_instance->queue_ops;
      handler_instance->time_ops=input_instance->time_ops;
      handler_instance->uart_driver=input_instance->uart_driver;
      ret=handler_instance->init(handler_instance,input_instance);

      if(ret!=UART_HANDLER_OK)
      {
 #ifdef HANDLER_DEBUG_ENABLE
       ESP_LOGI(TAG, "[uart_handler_inst] failed to handler init!");
 #endif   
            return UART_HANDLER_ERROR;
      }
 #ifdef HANDLER_DEBUG_ENABLE
       ESP_LOGI(TAG, "[uart_handler_inst] success to handler_inst!");
 #endif        
      return UART_HANDLER_OK;
}

//包处理函数
 uart_handler_state_t process_rx_data(
                        bsp_uart_handler_t *self,
                        uint8_t byte )
      {
      //参数检查
      if(NULL==self)
      {
 #ifdef HANDLER_DEBUG_ENABLE
       ESP_LOGI(TAG, "[process_rx_data] Function pointer is null!");
 #endif
            return UART_ERR_NULL_PTR;          
      }

      //检查缓冲区是否溢出
      if(self->rx_index>sizeof(self->rx_buffer))
      {
            self->parse_state=PACKET_WAIT_HEAD;
            self->rx_index=0;
      }

      switch(self->parse_state)
      {
            case PACKET_WAIT_HEAD:
            if(byte==PACKET_HEAD_VAL)
            {
            self->rx_index=0;
            self->parse_state=PACKET_LENGTH;
            }
            break;
            
            case PACKET_LENGTH:
            self->rx_buffer[self->rx_index++]=byte;
            self->parse_state=PACKET_SENSOR_ID;
            break;

            case PACKET_SENSOR_ID:
            self->rx_buffer[self->rx_index++]=byte;
            //检查是否有数据
            if(self->rx_buffer[0]==0)
            {
                  self->parse_state=PACKET_CRC;
            }
            else{
                  self->parse_state=PACKET_DATA;
            }
            break;

            case PACKET_DATA:
            self->rx_buffer[self->rx_index++]=byte;
            if(self->rx_index>=(self->rx_buffer[0]+2))
            {
            self->parse_state=PACKET_CRC;
            }
            break;

            case PACKET_CRC:
            if(bsp_utils_calc_crc8(self->rx_buffer,self->rx_index)==byte)
            {
              self->parse_state = PACKET_WAIT_TAIL;    
            }
            else
            {
 #ifdef HANDLER_DEBUG_ENABLE
      ESP_LOGI(TAG, "[process_rx_data] failed to CRC!");
 #endif
            self->parse_state=PACKET_WAIT_HEAD;  
            }
            break;

            case PACKET_WAIT_TAIL:
            if(PACKET_TAIL_VAL==byte)
            {
                if (self->frame_parsed_callback != NULL) {
                    uart_parsed_frame_t parsed_frame;
                    parsed_frame.data_len  = self->rx_buffer[0];
                    parsed_frame.sensor_id = self->rx_buffer[1];
                    // 如果长度大于0，传数据首地址，否则传 NULL
                    parsed_frame.payload = (parsed_frame.data_len > 0) ? 
                                    &self->rx_buffer[2] : NULL;
                    
                    // 触发回调函数，把解析干净的结构体扔给应用层
                    self->frame_parsed_callback(&parsed_frame);
            }
      }
      else
      {
 #ifdef HANDLER_DEBUG_ENABLE
      ESP_LOGI(TAG, "[process_rx_data] failed to tail!");
 #endif

      }
      // 无论包尾对不对，一包已经结束，复位等下一包
            self->parse_state = PACKET_WAIT_HEAD;
            break;

            default:
            self->parse_state = PACKET_WAIT_HEAD;
            break;
      }
      return UART_HANDLER_OK;
}

//uart_handler的线程函数
void uart_handler_task(void *argument)
{
      bsp_uart_handler_t * self=(bsp_uart_handler_t*)argument;

      //参数检查
      if(self==NULL||self->uart_driver==NULL||self->queue_ops==NULL)
      {
 #ifdef HANDLER_DEBUG_ENABLE
      ESP_LOGI(TAG, "[uart_handler_task] Function pointer is null!!");
 #endif 
      while(1) { self->time_ops->pfdelay_ms(1000); } // 挂起防止死机
      }

      // 2. 局部变量准备
      uint32_t event_msg;       // 用于接收队列消息
      uint8_t  rx_tmp_buf[128]; // 从底层搬运数据的“盆”
      int      read_len = 0;    // 本次搬运到的字节数

      for(;;)
      {
            //检查队列消息
            if(self->queue_ops->pfqueue_receive(self->event_queue_handle,
                              &event_msg,
                              0xffffffff)==UART_HANDLER_OK)
            {

            read_len=self->uart_driver->receive(self->uart_driver,
                                    rx_tmp_buf,
                                    sizeof(rx_tmp_buf),
                                    0);
            
            for(int i=0;i<read_len;i++)
            {
                  process_rx_data(self,rx_tmp_buf[i]);
            }

            }
      }
      
}
