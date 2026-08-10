#include "cubesat_sensors.h"   
#include "mlx90614.h"           
#include "driver/i2c_master.h"  
#include "esp_log.h"
#include "driver/uart.h"
#include <esp_err.h>
#include "i2cdev.h"

static const char *TAG = "MLX";
static const char *GPS_TAG = "GPS";

static i2c_master_bus_handle_t bus_handle;
static mlx90614_handle_t        mlx_handle;
static const float MLX_WARN_TEMP_C     = 60.0f;
static const float MLX_CRITICAL_TEMP_C = 80.0f;

esp_err_t sensors_init(void)
{
    i2c_master_bus_config_t bus_cfg = {};   // start with every field zeroed
    bus_cfg.i2c_port          = I2C_NUM_0;
    bus_cfg.sda_io_num        = GPIO_NUM_8;
    bus_cfg.scl_io_num        = GPIO_NUM_9;
    bus_cfg.clk_source        = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;
    bus_cfg.flags.enable_internal_pullup = true;


    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus create failed: %s", esp_err_to_name(err));
        return err;
    }

    mlx90614_config_t mlx_cfg = {};   // start with every field zeroed
    mlx_cfg.mlx90614_device.device_address = MLX90614_DEFAULT_ADDRESS;
    mlx_cfg.mlx90614_device.scl_speed_hz   = 100000;
    
    err = mlx90614_init(bus_handle, &mlx_cfg, &mlx_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MLX90614 init failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "MLX initialized");
    return ESP_OK;
}


esp_err_t sensors_read_object_ambient(float *temp_a)
{
    return mlx90614_get_ta(mlx_handle, temp_a);
}

esp_err_t sensors_read_object_temp(float *temp_c)
{
    return mlx90614_get_to(mlx_handle, temp_c);
}


esp_err_t sensors_read_mlx(mlx_data_t *out)
{
    esp_err_t err = sensors_read_object_temp(&out->object_temp_c);
    if (err != ESP_OK) return err;

    err = sensors_read_object_ambient(&out->ambient_temp_c);
    if (err != ESP_OK) return err;

    if (out->object_temp_c >= MLX_CRITICAL_TEMP_C) {
    out->status = SENSOR_CRITICAL;
    }
    else if (out->object_temp_c >= MLX_WARN_TEMP_C) {
        out->status = SENSOR_WARNING;
    }
    else {
        out->status = SENSOR_NOMINAL;
    }

    return ESP_OK;
}



static const uart_port_t GPS_UART_port =  UART_NUM_1;
static const int GPS_tx_pin = 1;
static const int GPS_rx_pin = 2;
static const int GPS_baud_rate = 9600;
static const int GPS_rx_buffer_size = 2048;



esp_err_t gps_init(void)
{
    uart_config_t cfg = {};
    cfg.baud_rate = GPS_baud_rate;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;

    esp_err_t err = uart_param_config(GPS_UART_port, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(GPS_TAG, "Config failed to initiate: %s", esp_err_to_name(err));
        return err;
    }


    err = uart_set_pin(GPS_UART_port, GPS_tx_pin, GPS_rx_pin, UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(GPS_TAG, "Pins failed to initialize: %s", esp_err_to_name(err));
        return err;
    }


    err = uart_driver_install(GPS_UART_port, GPS_rx_buffer_size, 0, 0, NULL,0);
    if (err != ESP_OK) {
        ESP_LOGE(GPS_TAG, "Driver failed to install: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(GPS_TAG, "GPS initialized");
    return ESP_OK;

    
}

