#include "cubesat_sensors.h"   
#include "mlx90614.h"       
#include "driver/i2c_master.h"  
#include "esp_log.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include <esp_err.h>
#include "i2cdev.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "MLX";
static const char *GPS_TAG = "GPS";
static const char *INA_TAG = "INA";
static const char *CAM_TAG = "CAM";

static i2c_master_bus_handle_t bus_handle;
static mlx90614_handle_t        mlx_handle;
static const float MLX_WARN_TEMP_C     = 60.0f;
static const float MLX_CRITICAL_TEMP_C = 80.0f;

static i2c_master_dev_handle_t ina_handle;


static esp_err_t ina_read_reg(uint8_t reg, uint16_t *out)
{
    uint8_t write_buf[1] = {reg};
    uint8_t read_buf[2] = {0};

    esp_err_t err = i2c_master_transmit_receive(ina_handle, write_buf, 1, read_buf, 2, 1000);
    if (err != ESP_OK)
    {
        return err;
    }
    *out = ((uint16_t)read_buf[0] << 8) | read_buf[1];
    return ESP_OK;
}

static esp_err_t ina_write_reg(uint8_t reg, uint16_t value)
{
   uint8_t buf[3];
    buf[0] = reg;
    buf[1] = (uint8_t)(value >> 8);
    buf[2] = (uint8_t)(value & 0xFF);

    return i2c_master_transmit(ina_handle, buf, 3, 1000);
    
}

static const uint16_t INA_CAL_VALUE     = 13429;
static const float    INA_CURRENT_LSB_A = 1.0f / 32768.0f;

esp_err_t sensors_init(void)
{
    i2c_master_bus_config_t bus_cfg = {};   // start with every field zeroed
    bus_cfg.i2c_port          = I2C_NUM_0;
    bus_cfg.sda_io_num        = GPIO_NUM_1;
    bus_cfg.scl_io_num        = GPIO_NUM_14;
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
    


    i2c_device_config_t ina_cfg = {};
    ina_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    ina_cfg.device_address = 0x40;
    ina_cfg.scl_speed_hz = 100000;

    err = i2c_master_bus_add_device(bus_handle, &ina_cfg, &ina_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(INA_TAG, "INA219 was unsuccessfully added to the I2C Bus: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(INA_TAG, "INA added to I2C Bus.");

    err = ina_write_reg(0x05, INA_CAL_VALUE);
    if (err != ESP_OK)
    {
        ESP_LOGE(INA_TAG, "INA219 calibration write failed: %s", esp_err_to_name(err));
        return err;
    }
    uint16_t cal_check = 0;
    err = ina_read_reg(0x05, &cal_check);
    if (err != ESP_OK)
    {
        ESP_LOGE(INA_TAG, "INA219 calibration read-back failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(INA_TAG, "INA219 calibrated, register reads %u (expected %u)",
             cal_check, INA_CAL_VALUE);
    
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


esp_err_t sensors_read_ina(ina_data_t *out)
{
    uint16_t raw_bus = 0;
    uint16_t raw_cur = 0;

    esp_err_t err = ina_read_reg(0x02, &raw_bus);
    if (err != ESP_OK) return err;

    err = ina_read_reg(0x04, &raw_cur);
    if (err != ESP_OK) return err;

    // Bus voltage: bits 15-3 hold the value, bits 2-0 are status flags.
    // Each count is 4 mV.
    out->voltage_V = (float)(raw_bus >> 3) * 0.004f;

    // Current register is SIGNED — cast before scaling or reverse
    // current reads as a huge positive number.
    out->current_mA = (float)(int16_t)raw_cur * INA_CURRENT_LSB_A * 1000.0f;

    out->power_mW = out->voltage_V * out->current_mA;

    out->status = SENSOR_NOMINAL;   // thresholds once you have a baseline

    return ESP_OK;
}

static const uart_port_t GPS_UART_port =  UART_NUM_1;
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


    err = uart_set_pin(GPS_UART_port, UART_PIN_NO_CHANGE, GPS_rx_pin, UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE);
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


esp_err_t camera_init(void)
{
    camera_config_t cfg = {};
    
    cfg.xclk_freq_hz = 20000000;
    cfg.pixel_format = PIXFORMAT_JPEG;
    cfg.frame_size   = FRAMESIZE_VGA;
    cfg.jpeg_quality = 12;
    cfg.fb_count     = 1;
    cfg.fb_location  = CAMERA_FB_IN_PSRAM;
    cfg.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
    cfg.ledc_timer   = LEDC_TIMER_0;
    cfg.ledc_channel = LEDC_CHANNEL_0;
    
    cfg.pin_pwdn = -1;
    cfg.pin_reset = -1;
    cfg.pin_xclk = 15;
    cfg.pin_sccb_scl = 5;
    cfg.pin_sccb_sda = 4;
    cfg.pin_d7 = 16;
    cfg.pin_d6 = 17;
    cfg.pin_d5 = 18;
    cfg.pin_d4 = 12;
    cfg.pin_d3 = 10;
    cfg.pin_d2 = 8;
    cfg.pin_d1 = 9;
    cfg.pin_d0 = 11;
    cfg.pin_vsync = 6;
    cfg.pin_href = 7;
    cfg.pin_pclk = 13;

    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(CAM_TAG, "camera init failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(CAM_TAG, "camera initialized");
    return ESP_OK;
}

esp_err_t camera_capture_to_sd (void)
{
    camera_fb_t * fb = esp_camera_fb_get();
        if (fb == NULL) {
            ESP_LOGE(CAM_TAG, "camera capture failed");
            return ESP_FAIL;
        }
    
    ESP_LOGI(CAM_TAG, "captured %u bytes", fb->len);

        
    esp_camera_fb_return(fb);

    return ESP_OK;

}

