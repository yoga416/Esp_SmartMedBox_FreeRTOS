idf_component_register(SRCS "uart.c" "sensor_parser.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver esp_common log app_mqtt mqtt json)