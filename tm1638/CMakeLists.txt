set(component_srcs "tm1638.c")

idf_component_register(SRCS "TM1638.c" "${component_srcs}"
                       PRIV_REQUIRES driver
                       INCLUDE_DIRS ".")
