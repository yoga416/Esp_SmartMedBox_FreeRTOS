# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
# cmake_minimum_required(VERSION 3.5)


# include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# project(Esp_SmartMedBox_FreeRTOS)
cmake_minimum_required(VERSION 3.5)

# 如果检测到系统环境变量不存在，就强行指定你昨天 idfcmd 成功的绝对路径
if(NOT DEFINED ENV{IDF_PATH})
    set(ENV{IDF_PATH} "D:/Embedded_Systems_app_install/Espressif/frameworks/esp-idf-v5.1.2")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Esp_SmartMedBox_FreeRTOS)