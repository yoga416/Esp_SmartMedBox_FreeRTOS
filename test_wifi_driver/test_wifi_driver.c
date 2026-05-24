#include "unity.h"
#include "bsp_wifi_driver.h"
#include <stdint.h>
#include <string.h>

// 引入 Mock 文件中的接口
extern wifi_port_ops_t mock_wifi_port_ops;
extern int mock_hw_init_call_count;
extern int mock_hw_send_call_count;
extern int mock_hw_connect_call_count;
extern int mock_hw_receive_call_count;

// 声明驱动实例化函数，防止隐式声明警告
wifi_state_t wifi_driver_inst(bsp_wifi_driver_t *driver, wifi_port_ops_t *port_ops, wifi_timebase_os_t *time_ops);

bsp_wifi_driver_t test_driver;
wifi_timebase_os_t mock_os;

void setUp(void) {
    mock_hw_init_call_count = 0;
    mock_hw_send_call_count = 0;
    mock_hw_connect_call_count = 0;
    mock_hw_receive_call_count = 0;
    memset(&mock_os, 0, sizeof(wifi_timebase_os_t));
    wifi_driver_inst(&test_driver, &mock_wifi_port_ops, &mock_os);
    wifi_init_config_t cfg = {
        .ssid = "TestSSID",
        .password = "TestPass",
        .channel = 1,
        .max_connection = 4,
        .authmode = 0
    };
    test_driver.pfinit(&test_driver, &cfg); // 保证每次测试 handler 都已初始化
}

void tearDown(void) {}

void test_WIFI_Init_Success(void) {
    wifi_init_config_t cfg = {
        .ssid = "TestSSID",
        .password = "TestPass",
        .channel = 1,
        .max_connection = 4,
        .authmode = 0
    };
    wifi_state_t ret = test_driver.pfinit(&test_driver, &cfg);
    TEST_ASSERT_EQUAL_INT(WIFI_DRIVER_OK, ret);
    TEST_ASSERT_EQUAL_INT(1, mock_hw_init_call_count);
}

void test_WIFI_Send_Data(void) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    wifi_state_t ret = test_driver.send(&test_driver, data, 3);
    TEST_ASSERT_EQUAL_INT(WIFI_DRIVER_OK, ret);
    TEST_ASSERT_EQUAL_INT(1, mock_hw_send_call_count);
}

void test_WIFI_Connect(void) {
    wifi_state_t ret = test_driver.pfconnect(&test_driver, "TestSSID", "TestPass");
    TEST_ASSERT_EQUAL_INT(WIFI_DRIVER_OK, ret);
    TEST_ASSERT_EQUAL_INT(1, mock_hw_connect_call_count);
}

void test_WIFI_Receive(void) {
    uint8_t rx[8] = {0};
    int rlen = test_driver.receive(&test_driver, rx, 8, 100);
    TEST_ASSERT_TRUE(rlen > 0 && rx[0] == 0xAA);
    TEST_ASSERT_EQUAL_INT(1, mock_hw_receive_call_count);
}

int main(void) {
    UnityBegin("test_wifi_driver.c");
    RUN_TEST(test_WIFI_Init_Success, __LINE__);
    RUN_TEST(test_WIFI_Send_Data, __LINE__);
    RUN_TEST(test_WIFI_Connect, __LINE__);
    RUN_TEST(test_WIFI_Receive, __LINE__);
    UnityEnd();
    return 0;
}
