idf_component_register(
    SRCS "espnow_easy.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_timer nvs_flash
)
