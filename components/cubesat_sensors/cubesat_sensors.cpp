#include "cubesat_sensors.h"   
#include "mlx90614.h"           
#include "driver/i2c_master.h"  
#include "esp_log.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include <esp_err.h>
#include "i2cdev.h"
#include <string.h>
#include <stdlib.h>

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

static char nmea_buf[100];
static int  nmea_len = 0;
static const int64_t GPS_STALE_TIMEOUT_US = 5000000;
static int64_t gps_last_valid_us = 0;



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


static bool nmea_field(const char *sentence,int index, char *out, size_t out_size )
{
    int comma_count = 0;
    size_t written = 0;
    
    for (int i = 0; sentence[i] != '\0'; i++)
    {
        if (comma_count == index)
        {
            while(sentence[i] != ','&& sentence[i] != '*' && sentence[i] != '\0' && written < out_size - 1)
            {
                out[written] = sentence[i];
                written++;
                i++;
                
            }
            out[written] = '\0';
            return true;
            
        }
        if (sentence[i] == ',')
        {
            comma_count++;
        }
        
    }
    return false;
}


static void parse_rmc(const char *sentence, atg_data_t *out)
{
    char field[16];
    if (!nmea_field(sentence, 2, field, sizeof(field)))
    {
        return;
    }
    out->valid = (field[0] == 'A');
    if (!out->valid)
    {
        return;
    }
    gps_last_valid_us = esp_timer_get_time();
    if (nmea_field(sentence, 1, field, sizeof(field)))
    {
        int hh = (field[0] - '0') * 10 + (field[1] - '0');
        int mm = (field[2] - '0') * 10 + (field[3] - '0');
        int ss = (field[4] - '0') * 10 + (field[5] - '0');
        out->utc_seconds = hh * 3600 + mm * 60 + ss;
    }
    if (nmea_field(sentence, 9, field, sizeof(field)))
    {
        out->utc_date = (uint32_t)atoi(field);
    }
    if (nmea_field(sentence, 3, field, sizeof(field)) && field[0] != '\0')
    {
        double raw = atof(field);              
        int    deg = (int)(raw / 100);         
        double min = raw - (deg * 100);       
        out->lat_deg = deg + (min / 60.0);     

        char hemi[4];
        if (nmea_field(sentence, 4, hemi, sizeof(hemi)) && hemi[0] == 'S')
            out->lat_deg = -out->lat_deg;
    }
    if (nmea_field(sentence, 5, field, sizeof(field)) && field[0] != '\0')
    {
        double raw = atof(field);              
        int    deg = (int)(raw / 100);         
        double min = raw - (deg * 100);        
        out->lon_deg = deg + (min / 60.0);     

        char hemi[4];
        if (nmea_field(sentence, 6, hemi, sizeof(hemi)) && hemi[0] == 'W')
            out->lon_deg = -out->lon_deg;
    }

}

static void parse_gga(const char *sentence, atg_data_t *out)                //// status is owned by parse_gga (satellite count lives there)
{
    char field [16];

    if (!nmea_field(sentence, 6, field, sizeof(field))) { return; }

    
    if (atoi(field) == 0)
    {
        out->status = SENSOR_CRITICAL;
        return;
    }
    
    if (nmea_field(sentence, 2, field, sizeof(field)) && field[0] != '\0')
    {
        double raw = atof(field);              
        int    deg = (int)(raw / 100);         
        double min = raw - (deg * 100);       
        out->lat_deg = deg + (min / 60.0);     

        char hemi[4];
        if (nmea_field(sentence, 3, hemi, sizeof(hemi)) && hemi[0] == 'S')
            out->lat_deg = -out->lat_deg;
    }
    if (nmea_field(sentence, 4, field, sizeof(field)) && field[0] != '\0')
    {
        double raw = atof(field);              
        int    deg = (int)(raw / 100);         
        double min = raw - (deg * 100);        
        out->lon_deg = deg + (min / 60.0);     

        char hemi[4];
        if (nmea_field(sentence, 5, hemi, sizeof(hemi)) && hemi[0] == 'W')
            out->lon_deg = -out->lon_deg;
    }

    if (nmea_field(sentence, 7, field, sizeof(field)) && field[0] != '\0')
    {
        
        out->satellites = (uint8_t)atoi(field);
        if (out->satellites < 4)
        {out->status = SENSOR_CRITICAL;}
        else if (out->satellites < 6)
        {out->status = SENSOR_WARNING;}
        else
        {out->status = SENSOR_NOMINAL;}
    }
    if (nmea_field(sentence, 9, field, sizeof(field)) && field[0] != '\0')
    {
        out->altitude_m = (float)atof(field);
    }

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
                                if (strncmp(nmea_buf + 3, "RMC",3 ) == 0)
                                {
                                    parse_rmc(nmea_buf, out);
                                }
                                if(strncmp(nmea_buf+3, "GGA", 3)==0){
                                    parse_gga(nmea_buf,out);
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
    if ((esp_timer_get_time() - gps_last_valid_us) > GPS_STALE_TIMEOUT_US)
    {
        out->valid = false;
        out->status = SENSOR_CRITICAL;
    }
    return ESP_OK;
}

