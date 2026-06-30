#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "cubesat_sensors.h"

static const char *TAG = "main";


extern "C" void app_main(void)
{
    // Set up the I2C bus + sensors
    esp_err_t err = sensors_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "sensor init failed, stopping");
        return;
    }

    
    while (true) {
        float object_temp = 0.0f;

        err = sensors_read_object_temp(&object_temp);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Object temperature: %.2f C", object_temp);
        } else {
            ESP_LOGW(TAG, "read failed: %s", esp_err_to_name(err));
        }

        // ---- add more sensor reads / build your telemetry packet here ----

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}