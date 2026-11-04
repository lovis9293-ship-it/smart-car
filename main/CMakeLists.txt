idf_component_register(SRCS "PRJCV.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver bt nvs_flash  motor car freertos bluetooth rc safety
                    )
