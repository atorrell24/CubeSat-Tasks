#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"
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

    err = gps_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "GPS init failed: %s", esp_err_to_name(err));
        return;   // temporary: no point running the dump test without UART
    }

    // ================= TEMPORARY RAW NMEA DUMP =================
    // Delete this block once you see "$GN..." sentences.
    ESP_LOGI(TAG, "dumping raw UART1 bytes...");
    uint8_t buf[256];
    while (true)
    {
        int len = uart_read_bytes(UART_NUM_1, buf, sizeof(buf) - 1,
                                  pdMS_TO_TICKS(1000));
        if (len > 0)
        {
            buf[len] = '\0';
            printf("%s", (char *)buf);
        }
        else
        {
            ESP_LOGW(TAG, "no data");
        }
    }
    // =============== END TEMPORARY BLOCK =======================

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

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}