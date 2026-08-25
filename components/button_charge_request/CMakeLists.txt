idf_component_register(
    SRCS "button_charge_request.c"
    INCLUDE_DIRS "."
    REQUIRES board
            charge_request_iface
            charge_abort_iface
)