# 定义当前组件编译所需的源文件目录
# （假设 WIFISTA, LCD, SPI, LED 是当前组件目录下的子文件夹）
idf_component_register(
    SRC_DIRS        NTC
                    uart
                    wifi
                    mqtt

    # 如果这些头文件目录需要暴露给其他组件使用（公共接口）
    INCLUDE_DIRS    NTC
                    uart
                    wifi
                    mqtt
    # 如果某些目录只是组件内部自己用，建议放在这里（私有接口，可选）
    # PRIV_INCLUDE_DIRS "some_private_dir"
    
    # 组件依赖的其他 ESP-IDF 官方组件
    REQUIRES 
                        esp_wifi
                        esp_event
                        esp_netif
                        nvs_flash
                        lwip
                        log
                        driver
                        mqtt
)

# 针对当前组件的编译优化与警告控制
# -O3 优化开启，同时关闭 format 警告（避免某些第三方驱动报非致命错误导致中断）
component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)