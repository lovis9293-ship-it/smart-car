idf_component_register(SRCS "hcsr05.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio esp_timer esp_rom freertos)
