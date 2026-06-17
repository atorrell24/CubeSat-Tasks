#include "bme68x_i2c_esp_idf.h"

const static char *TAG = "bme68x";

esp_err_t bme68x_sensor_create(const bme68x_i2c_config_t *i2c_conf, bme68x_handle_t *handle_ret)
{
    esp_err_t ret = ESP_OK;
    int8_t rslt;

    struct bme68x_dev *bme = (struct bme68x_dev *)calloc(1, sizeof(struct bme68x_dev));
    ESP_RETURN_ON_FALSE(bme, ESP_ERR_NO_MEM, TAG, "memory allocation for device handler failed");

    rslt = bme68x_interface_init(bme, BME68X_I2C_INTF, i2c_conf->i2c_addr, i2c_conf->i2c_handle);
    bme68x_check_rslt("bme68x_sensor_create", rslt);
    ESP_LOGI(TAG, "bme68x_check_rslt done");

    ESP_GOTO_ON_FALSE((BME68X_OK == rslt), ESP_ERR_INVALID_STATE, err, TAG, "bme68x_interface_init failed");

    // Initialize BME68x
    rslt = bme68x_init(bme);
    ESP_GOTO_ON_FALSE((rslt == BME68X_OK), ESP_ERR_INVALID_STATE, err, TAG, "bme68x_init failed");

    ESP_LOGI(TAG, " Create %-15s", "BME68x");

    *handle_ret = bme;
    return ret;

    err:
    bme68x_sensor_del(bme);
    return ret;
}

esp_err_t bme68x_sensor_del(bme68x_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle pointer");
    free(handle);

    return ESP_OK;
}
