

idf_component_register(SRCS "timer.cpp" "gpio.cpp" "i2c.cpp" "ex_io.cpp" "ina219.cpp" "mpu6050.cpp" "ws2812.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_i2c esp_driver_spi esp_driver_gpio  esp_driver_gptimer                  
                    )

