#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "bme68x_i2c_esp_idf.h"
#include "i2c_bus.h"
#include "ina219.h"
#include "driver/uart.h"
#include "mlx90614.h"
#include "bno08x_driver.h"








#define LED_GPIO GPIO_NUM_41

extern "C" void app_main(void)

{
    //int bme68xSensor;
    //uint8_t i2c_addr = 0x77;
    //i2c_bus_handle_t i2c_handle = NULL;
    //const bme68x_i2c_config_t *i2c_conf;
    while (true)
    {
        
        //bme68xSensor = bme68x_sensor_create()
        printf("Hello World");

    }
}