idf_component_register(
    SRCS "energy.c"
    INCLUDE_DIRS "."
    REQUIRES pzem esp_timer nvs_flash
)
