idf_component_register(SRCS "app_weather.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_http_client json uart)
