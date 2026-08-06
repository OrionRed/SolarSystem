idf_component_register(
    SRCS
        "main.c"
        "app.c"
    INCLUDE_DIRS "."
    REQUIRES
        system_info
        board
        demo
        hal_uart
        modbus
)
