idf_component_register(
    SRCS "cubesat_sensors.cpp"
    INCLUDE_DIRS "."
    REQUIRES mlx90614 i2cdev driver esp_driver_i2c esp_driver_uart esp_timer espressif__esp32-camera
)




