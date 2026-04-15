idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    # 删除 REQUIRES bsp 这行，或者将其留空
)