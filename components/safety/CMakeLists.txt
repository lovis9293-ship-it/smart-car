idf_component_register(SRCS "safety.c"
    INCLUDE_DIRS "."
    REQUIRES car hcsr05 freertos log)
