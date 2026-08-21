#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "cubesat_sensors.h"

static const char *TAG = "main";

extern "C" void app_main(void)
{
    esp_err_t err = sensors_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "sensor init failed, stopping");
        return;
    }

    bool gps_ok = true;
    err = gps_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "GPS init failed: %s", esp_err_to_name(err));
        gps_ok = false;   // keep running — other sensors still work
    }

    bool cam_ok = true;
    err = camera_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "camera init failed: %s", esp_err_to_name(err));
        cam_ok = false;
    }
      if (cam_ok)
    {
        err = camera_capture_to_sd();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "capture failed: %s", esp_err_to_name(err));
        }
    }

    while (true)
    {
        //MLX90614 Logic------------------------
        mlx_data_t mlx = {};

        err = sensors_read_mlx(&mlx);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "MLX  object: %.2f C   ambient: %.2f C",
                     mlx.object_temp_c, mlx.ambient_temp_c);
            if (mlx.status == SENSOR_CRITICAL)
            {
                ESP_LOGE(TAG, "MLX CRITICAL: object temp %.2f C", mlx.object_temp_c);
            }
            else if (mlx.status == SENSOR_WARNING)
            {
                ESP_LOGW(TAG, "MLX WARNING: object temp %.2f C", mlx.object_temp_c);
            }
        }
        else
        {
            ESP_LOGW(TAG, "MLX read failed: %s", esp_err_to_name(err));
        }

        //GPS Logic-----------------------------
        if (gps_ok)
        {
            atg_data_t gps = {};
            err = sensors_read_gps(&gps);
            if (err != ESP_OK)
            {
                ESP_LOGW(TAG, "GPS read failed: %s", esp_err_to_name(err));
            }
            else
            {
                ESP_LOGI(TAG, "GPS valid=%d lat=%.6f lon=%.6f alt=%.1f sats=%u status=%d t=%lu d=%lu",
                         gps.valid, gps.lat_deg, gps.lon_deg, gps.altitude_m,
                         gps.satellites, gps.status, gps.utc_seconds, gps.utc_date);
            }
        }

        //ina logic 
        ina_data_t ina = {};
        err = sensors_read_ina(&ina);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "INA  %.3f V   %.2f mA   %.1f mW",
                     ina.voltage_V, ina.current_mA, ina.power_mW);
        }
        else
        {
            ESP_LOGW(TAG, "INA read failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}