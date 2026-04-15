#include "unity.h"
#include "bsp_uart_driver.h"
#include <stdint.h>  // 必须包含，确保 uint8_t 识别
#include <string.h>

// 引入 Mock 文件中的变量和接口
// 注意：确保 mock_uart_port.c 中这些变量没有加 static
extern uart_port_ops_t esp32_uart_port_ops;
extern int mock_hw_init_call_count;
extern int mock_hw_write_call_count;
extern uint32_t last_configured_baud;

// 定义测试对象
bsp_uart_driver_t test_driver;
timebase_os_t mock_os;

// Unity 每个测试用例前的初始化 
void setUp(void) {
    // 1. 重置 Mock 状态，确保测试用例之间独立
    mock_hw_init_call_count = 0;
    mock_hw_write_call_count = 0;
    last_configured_baud = 0;
    
    // 2. 清零 OS 接口（如果暂不测试延时）
    memset(&mock_os, 0, sizeof(timebase_os_t));
    
    // 3. 构造驱动实例：这会把函数指针（init, send等）挂载到 test_driver 上
    uart_inst(&test_driver, &esp32_uart_port_ops, 1, &mock_os);
}

void tearDown(void) {
    // 每个测试后的清理工作，此处可留空
}

// 测试 1：初始化流程是否正确透传参数到底层 
void test_UART_Init_Success(void) {
    uart_init_config_t cfg = {
        .baud_rate = 115200, 
        .tx_pin = 4, 
        .rx_pin = 5
    };
    
    // 调用驱动层的 init，内部应调用 port_ops->hw_init
    uart_state_t ret = test_driver.init(&test_driver, &cfg, 1024, 128);
    
    TEST_ASSERT_EQUAL_INT(UART_OK, ret);           // 验证驱动返回状态
    TEST_ASSERT_EQUAL_INT(1, mock_hw_init_call_count); // 验证底层硬件被初始化次数
    TEST_ASSERT_EQUAL_UINT32(115200, last_configured_baud); // 验证波特率传递
}

// 测试 2：发送数据是否成功触发硬件写入 
void test_UART_Send_Data(void) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    
    // 发送数据
    uart_state_t ret = test_driver.send(&test_driver, data, 3);
    
    TEST_ASSERT_EQUAL_INT(UART_OK, ret);
    TEST_ASSERT_EQUAL_INT(1, mock_hw_write_call_count); // 验证是否调用了 hw_write
}

// 测试 3：缓冲区长度获取逻辑 
void test_UART_Get_Buffered_Len(void) {
    // 直接调用驱动接口，检查是否拿到了 Mock 中写死的 10
    uint32_t len = test_driver.get_rx_buffered_len(&test_driver);
    
    TEST_ASSERT_EQUAL_UINT32(10, len); 
}

// Unity 运行入口 
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_UART_Init_Success);
    RUN_TEST(test_UART_Send_Data);
    RUN_TEST(test_UART_Get_Buffered_Len);
    return UNITY_END();
}