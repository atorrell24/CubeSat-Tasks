#ifndef BME68X_I2C_ESPIDF_H
#define BME68X_I2C_ESPIDF_H

#include "bme68x_i2c_helper.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"

#include "bme68x.h"

/**
 * @brief BME68x I2C configuration structure
 *
 * This structure contains the configuration parameters for communicating with
 * the BME68x sensor over I2C. It specifies the I2C handle/context and the I2C address of
 * the BME68x device.
 */
typedef struct {
    i2c_bus_handle_t i2c_handle;    /*!< I2C handle/context used to connect to the BME68x device */
    uint8_t i2c_addr;    /*!< I2C address of the BME68x device */
} bme68x_i2c_config_t;

/**
 * @brief Handle type for BME68x sensor
 *
 * This is a pointer to a structure representing the BME68x device. It is used
 * as a handle for interacting with the sensor.
 */
typedef struct bme68x_dev *bme68x_handle_t;

/**
 * @brief Create and initialize a BME68x sensor object
 *
 * This function initializes the BME68x sensor and prepares it for use.
 * It configures the I2C interface and creates a handle for further communication.
 *
 * @param[in] i2c_conf Pointer to the I2C configuration structure
 * @param[out] handle_ret Pointer to a variable that will hold the created sensor handle
 * @return
 *      - ESP_OK: Successfully created the sensor object
 *      - ESP_ERR_INVALID_ARG: Invalid arguments were provided
 *      - ESP_FAIL: Failed to initialize the sensor
 */
esp_err_t bme68x_sensor_create(const bme68x_i2c_config_t *i2c_conf, bme68x_handle_t *handle_ret);

/**
 * @brief Delete and release a BME68x sensor object
 *
 * This function releases the resources allocated for the BME68x sensor.
 * It should be called when the sensor is no longer needed.
 *
 * @param[in] handle Handle of the BME68x sensor object
 * @return
 *      - ESP_OK: Successfully deleted the sensor object
 *      - ESP_ERR_INVALID_ARG: Invalid handle was provided
 *      - ESP_FAIL: Failed to delete the sensor object
 */
esp_err_t bme68x_sensor_del(bme68x_handle_t handle);

#endif // BME68X_I2C_ESPIDF_H