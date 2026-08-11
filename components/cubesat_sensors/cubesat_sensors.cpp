#include "cubesat_sensors.h"   
#include "mlx90614.h"           
#include "driver/i2c_master.h"  
#include "esp_log.h"
#include "driver/uart.h"
#include <esp_err.h>
#include "i2cdev.h"
#include <string.h>
#include "bme68x.h"
#include "bme68x_i2c_helper.h"

static const char *TAG = "MLX";
static const char *GPS_TAG = "GPS";

static i2c_master_bus_handle_t bus_handle;
static mlx90614_handle_t        mlx_handle;
static const float MLX_WARN_TEMP_C     = 60.0f;
static const float MLX_CRITICAL_TEMP_C = 80.0f;
static i2c_master_dev_handle_t bme_i2c_handle;
static struct bme68x_dev bme_dev;

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
    

    printf("BME: Adding device to I2C bus\n");

    i2c_device_config_t bme_i2c_cfg = {};

    bme_i2c_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    bme_i2c_cfg.device_address = 0x76;
    bme_i2c_cfg.scl_speed_hz = 100000;

    err = i2c_master_bus_add_device(
        bus_handle,
        &bme_i2c_cfg,
        &bme_i2c_handle
    );

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "BME: Failed to add device: %s",
            esp_err_to_name(err)
        );

        return err;
    }

    printf("BME: add device returned %d\n", err);
    ESP_LOGI(TAG, "BME: Setting up Bosch driver");


    bme_dev.intf = BME68X_I2C_INTF;
    bme_dev.intf_ptr = (void *)bme_i2c_handle;

    bme_dev.read = bme68x_i2c_read;
    bme_dev.write = bme68x_i2c_write;
    bme_dev.delay_us = bme68x_delay_us;

    ESP_LOGI(TAG, "BME: Starting initialization");

    int8_t bme_result = bme68x_init(&bme_dev);

    ESP_LOGI(TAG, "BME: bme68x_init returned %d", bme_result);

    if (bme_result != BME68X_OK)
    {
        ESP_LOGE(TAG, "BME: Initialization failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "BME: Sensor communication successful");

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

static char nmea_buf[100];
static int  nmea_len = 0;



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

esp_err_t sensors_read_gps(atg_data_t *out)
{
    uint8_t chunk[256];
    
    while (true)
    {
        int len = uart_read_bytes(GPS_UART_port, chunk, sizeof(chunk), pdMS_TO_TICKS(20));
        if (len <= 0 ) break;
        for(int i = 0; i < len; i++)
        {
            char c = (char)chunk[i];
            if (c == '$')
            {
                nmea_len = 0;
                nmea_buf[nmea_len] = c;
                nmea_len++;  
            }
            else if (c == '\n' || c == '\r')
            {
                
                nmea_buf[nmea_len] = '\0';
                    
                if (nmea_len >= 6)
                    {
                        char *star = strchr(nmea_buf, '*'); 
                        if ( star != NULL)
                        {
                            uint8_t computed = 0;
                            for (int k = 1; k < (star - nmea_buf); k++)
                            {
                                computed ^= (uint8_t)nmea_buf[k];
                            }

                            uint8_t received = (uint8_t)strtol(star + 1, NULL, 16);
                            if (computed == received)
                            {
                                if (strncmp(nmea_buf + 3, "RMC", 3) == 0 ||
                                    strncmp(nmea_buf + 3, "GGA", 3) == 0)
                                {
                                    printf("%s\n", nmea_buf);
                                }
                            }
                        }
                        

                        
                    }
                    
                nmea_len = 0;
                
                
            }
            else
            {
                if (nmea_len < (int)sizeof(nmea_buf) - 1)
                {
                    nmea_buf[nmea_len] = c;
                    nmea_len++;
                }
                else
                {
                    nmea_len = 0; 
                }
            }
        }
    }
    return ESP_OK;
}

