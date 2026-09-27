idf_component_register(SRCS "pwm_hw.c" "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_driver_gpio esp_driver_ledc esp_adc)
