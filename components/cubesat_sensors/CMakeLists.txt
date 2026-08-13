idf_component_register(
    SRCS "cubesat_sensors.cpp"
    INCLUDE_DIRS "."
    REQUIRES mlx90614 i2cdev driver esp_driver_i2c esp_driver_uart bno085 bme68x
)




