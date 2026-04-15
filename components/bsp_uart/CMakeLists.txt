idf_component_register(
    SRCS "bsp_os_layer.c" "bsp_uart_driver.c" "bsp_uart_handler.c" "bsp_uart_port.c"
    INCLUDE_DIRS "."
    REQUIRES driver bsp_utils
)
