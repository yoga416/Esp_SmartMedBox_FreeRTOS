#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"

// 引入 UART 组件头文件
#include "bsp_uart_handler.h"
#include "bsp_uart_driver.h"
#include "bsp_os_layer.h"
#include "bsp_uart_port.h"
#include "bsp_uart_reg.h"

// 引入 Wi-Fi / 网络组件头文件
#include "bsp_wifi_bridge.h"
#include "bsp_wifi_handler.h"
#include "bsp_wifi_driver.h"
#include "bsp_wifi_port.h"

static const char *TAG = "APP_MAIN";

// 1. 定义全局实例
static bsp_uart_handler_t g_uart_handler;
static bsp_uart_driver_t  g_uart_driver;
static bsp_wifi_driver_t  g_wifi_driver; // WiFi 驱动实例

// 引入底层端口硬件操作接口
extern uart_port_ops_t esp32_uart_port_ops;
extern int wifi_hw_init(const char* ssid, const char* password, uint8_t channel, 
                        uint8_t max_connection, wifi_auth_mode_t authmode, void **wifi_handle);
extern int wifi_hw_connect(void* wifi_handle, const char* ssid, const char* password);

// 定义 WiFi 端口操作结构体
static wifi_port_instance_t my_wifi_port = {
    .hw_init = wifi_hw_init,
    .hw_connect = wifi_hw_connect,
    // 其他接口根据 bsp_wifi_port.c 中的实现挂载
};

// 定义 WiFi OS 时间基准
static uint32_t get_os_tick(void) { return (uint32_t)xTaskGetTickCount(); }
static void os_delay(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

static wifi_timebase_os_t my_wifi_timebase = {
    .pfget_count = get_os_tick,
    .pfdelay_ms = os_delay
};

// 全局 MQTT 句柄，供 bsp_wifi_handler.c 调用
esp_mqtt_client_handle_t mqtt_client = NULL;

// =========================================================================
// 2. 串口接收回调：直接把数据丢给 Bridge
// =========================================================================
void on_uart_packet_received(const uart_parsed_frame_t *frame) {
    ESP_LOGI(TAG, "=> [1. 串口层] 成功捕获数据帧! Sensor ID: 0x%02X, Len: %d", frame->sensor_id, frame->data_len);

    if (frame->data_len > 0) {
        esp_log_buffer_hex("原始 HEX", frame->payload, frame->data_len);
    }

    // 调用 Bridge 接口进行深拷贝并入队
    if (bsp_wifi_bridge_publish_raw(frame->sensor_id, frame->payload, frame->data_len) == 0) {
        ESP_LOGI(TAG, "=> [2. Bridge层] 数据已推入后台发送队列");
    }
}

// =========================================================================
// 3. 封装 UART 初始化流程
// =========================================================================
static bool init_uart_and_handler(void) {
    if (uart_inst(&g_uart_driver, &esp32_uart_port_ops, 1, &os_time_ops) != UART_OK) return false;

    uart_init_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .tx_pin    = UART_TX_PIN,
        .rx_pin    = UART_RX_PIN
    };

    if (g_uart_driver.init(&g_uart_driver, &uart_config, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE) != UART_OK) 
    {
    return false;
    }

    uart_handler_input_instance_t handler_input = {
        .uart_driver = &g_uart_driver,
        .queue_ops   = &os_queue_ops,
        .time_ops    = &os_time_ops
    };

    if (uart_handler_inst(&g_uart_handler, &handler_input) != UART_HANDLER_OK){

    return false;
    }

    // 注册串口数据接收回调
    g_uart_handler.register_rx_callback(&g_uart_handler, on_uart_packet_received);

    // 创建串口处理任务
    xTaskCreate(uart_handler_task, "uart_rx_task", 4096, &g_uart_handler, 5, NULL);
    
    return true;
}


// =========================================================================
// 4. MQTT 启动函数
// =========================================================================
static void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://10.73.212.52", // 替换为你的服务器 IP
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(mqtt_client);
    ESP_LOGI(TAG, "MQTT 客户端已启动");
}

// =========================================================================
// 5. 主函数
// =========================================================================
void app_main(void) {
    ESP_LOGW(TAG, "SYSTEM BOOTING.../****************************************/");
    // A. 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    // 检查 NVS 初始化结果
    ESP_ERROR_CHECK(ret);

    // B. 初始化 WiFi 架构
    bsp_wifi_bridge_init();
    bsp_wifi_handler_init();

    // C. WiFi 驱动实例化与硬件初始化
    wifi_inst(&g_wifi_driver, &my_wifi_port, my_wifi_timebase);
    
    wifi_config_instance_t wifi_config = {
        .ssid = "yoga",
        .password = "qwertyui",
        .authmode = WIFI_AUTH_WPA2_PSK
    };

    if (g_wifi_driver.pfinit(&g_wifi_driver, &wifi_config) == WIFI_DRIVER_OK) 
    {
         ESP_LOGI(TAG, "WiFi 驱动初始化成功，正在连接 WiFi...");
        g_wifi_driver.pfconnect(&g_wifi_driver, wifi_config.ssid, wifi_config.password);
    }
        // 给 WiFi 3-5 秒时间进行扫描、握手和获取 IP
    ESP_LOGI(TAG, "等待 WiFi 连接并获取 IP...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // D. 启动 MQTT
    mqtt_app_start();

    // E. 初始化底层串口自发自收环境
    ESP_LOGW(TAG, "请短接 GPIO18 (TX) 和 GPIO17 (RX)");
    if (!init_uart_and_handler())
    {
        ESP_LOGE(TAG, "串口或处理器初始化失败，系统无法继续运行！");
        return;
    }

    // F. 测试数据循环
    uint8_t payload_temp_humi[] = {0x09, 0x29, 0x11, 0xD7}; 
    uint8_t payload_health[] = {0x4B, 0x62}; 
    uint8_t payload_face[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03}; // 模拟变长特征码
    while(1) {
        ESP_LOGI(TAG, "\n--- 发送测试数据 ---");
        g_uart_handler.send_frame(&g_uart_handler, UPLOAD_TYPE_HUMI_TEMP, payload_temp_humi, sizeof(payload_temp_humi));
        vTaskDelay(pdMS_TO_TICKS(3000));
        
        g_uart_handler.send_frame(&g_uart_handler, UPLOAD_TYPE_HEART_RATE, payload_health, sizeof(payload_health));
        vTaskDelay(pdMS_TO_TICKS(3000));
        
        g_uart_handler.send_frame(&g_uart_handler, UPLOAD_EVENT_FACE_RECOGNIZED, payload_face, sizeof(payload_face));
        vTaskDelay(pdMS_TO_TICKS(3000));
        vTaskDelay(pdMS_TO_TICKS(100)); // 主循环每10秒打印一次日志，保持任务活跃
    }
}