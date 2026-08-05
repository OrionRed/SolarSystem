idf_component_register(
    SRCS "hal_uart.c"
    INCLUDE_DIRS "."
    REQUIRES driver 
             config
             esp_driver_gpio
             esp_driver_uart
)