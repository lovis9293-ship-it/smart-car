idf_component_register(SRCS "blue.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver bt nvs_flash  motor car freertos
                    )
