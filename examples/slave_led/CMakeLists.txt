idf_component_register(
    SRCS "main_slave_led.c" "led_strip_rmt.c"
    INCLUDE_DIRS "."
    REQUIRES espnow_easy driver
)
