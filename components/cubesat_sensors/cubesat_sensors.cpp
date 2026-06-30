#include "cubesat_sensors.h"   
#include "mlx90614.h"           
#include "driver/i2c_master.h"  
#include "esp_log.h"

static const char *TAG = "sensors";

static i2c_master_bus_handle_t bus_handle;
static mlx90614_handle_t        mlx_handle;

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

    ESP_LOGI(TAG, "Sensors initialized");
    return ESP_OK;
}

esp_err_t sensors_read_object_temp(float *temp_c)
{
    return mlx90614_get_to(mlx_handle, temp_c);
}
