idf_component_register(
    SRCS "bsp_wifi_bridge.c" "bsp_wifi_os_layer.c" "bsp_wifi_driver.c" "bsp_wifi_handler.c" "bsp_wifi_port.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi nvs_flash freertos json mqtt log bsp_utils
)
