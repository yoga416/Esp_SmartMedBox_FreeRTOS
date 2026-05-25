# 定义主组件编译所需的源文件
idf_component_register(
    SRCS            main.c

    # 公共头文件目录（暴露给其他组件）
    INCLUDE_DIRS    .
                 
    # 组件依赖的其他 ESP-IDF 官方组件
    REQUIRES 
                        components
                        esp_wifi
                        esp_event
                        nvs_flash
                        lwip
                        log
                        driver
                        mqtt 
)