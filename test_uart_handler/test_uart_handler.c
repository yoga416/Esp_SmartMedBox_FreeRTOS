#include "unity.h"
#include "bsp_uart_handler.h"
#include "bsp_utils.h"
#include <string.h>

// --- 全局变量捕获结果 ---
static uint8_t last_parsed_data[128];
static uint16_t last_parsed_len = 0;
static uint8_t last_sensor_id = 0;
static int callback_count = 0;

extern uart_queue_ops_t mock_queue_ops;
extern timebase_os_t mock_time_ops;

// 回调函数：对应你的 uart_parsed_frame_t 结构体
void mock_frame_parsed_callback(const uart_parsed_frame_t *frame) {
    callback_count++;
    last_parsed_len = frame->data_len;
    last_sensor_id = frame->sensor_id;
    if (frame->payload != NULL && frame->data_len > 0) {
        memcpy(last_parsed_data, frame->payload, frame->data_len);
    }
}

void setUp(void) {
    callback_count = 0;
    last_parsed_len = 0;
    last_sensor_id = 0;
    memset(last_parsed_data, 0, sizeof(last_parsed_data));
}

void tearDown(void) {}

// --- 测试用例 1：完整协议解析测试 ---
void test_uart_handler_full_parse_logic(void) {
    bsp_uart_handler_t handler;
    bsp_uart_driver_t dummy_driver;
    uart_handler_input_instance_t input = {
        .queue_ops = &mock_queue_ops,
        .time_ops = &mock_time_ops,
        .uart_driver = &dummy_driver
    };

    // 初始化
    uart_handler_inst(&handler, &input);
    handler.frame_parsed_callback = mock_frame_parsed_callback;

    // 构造数据包
    // 协议：[HEAD:5A] [LEN:03] [ID:01] [DATA:11 22 33] [CRC:??] [TAIL:FF]
    uint8_t payload[] = {0x11, 0x22, 0x33};
    uint8_t sensor_id = 0x01;
    uint8_t data_len = sizeof(payload);

    // 计算 CRC：对应你代码中 bsp_utils_calc_crc8(self->rx_buffer, self->rx_index)
    // 此时 rx_buffer 存的是 [LEN] [ID] [DATA...]
    uint8_t crc_buf[5];
    crc_buf[0] = data_len;
    crc_buf[1] = sensor_id;
    memcpy(&crc_buf[2], payload, data_len);
    uint8_t expected_crc = bsp_utils_calc_crc8(crc_buf, 5);

    uint8_t raw_stream[] = {0x5A, data_len, sensor_id, 0x11, 0x22, 0x33, expected_crc, 0xFF};

    // 模拟输入
    extern void process_rx_data(bsp_uart_handler_t *self, uint8_t byte);
    for (int i = 0; i < sizeof(raw_stream); i++) {
        process_rx_data(&handler, raw_stream[i]);
    }

    // 验证
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, callback_count, "Callback fail: Check state machine transition");
    TEST_ASSERT_EQUAL_INT(3, last_parsed_len);
    TEST_ASSERT_EQUAL_INT(0x01, last_sensor_id);
    TEST_ASSERT_EQUAL_HEX8(0x11, last_parsed_data[0]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_uart_handler_full_parse_logic);
    return UNITY_END();
}