idf_component_register(
    SRCS            "main.c"
    INCLUDE_DIRS    "."
    REQUIRES        NTC wifi app_mqtt uart app_weather app_time_sync esp_wifi esp_event nvs_flash lwip log driver mqtt esp_http_client json
)