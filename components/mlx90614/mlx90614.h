#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdint.h>

#define MLX90614_DEFAULT_ADDRESS (0x5A)
#define MLX90614_EEPROM_SIZE (0x20)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    i2c_device_config_t mlx90614_device; /*!< Configuration for mlx90614 device */
} mlx90614_config_t;

typedef struct mlx90614_t *mlx90614_handle_t;

/**
 * @brief Init an MLX90614 device.
 *
 * @param[in] bus_handle I2C master bus handle
 * @param[in] mlx90614_config Configuration of MLX90614
 * @param[out] mlx90614_handle Handle of MLX90614
 * @return ESP_OK: Init success. ESP_FAIL: Not success.
 */
esp_err_t mlx90614_init(i2c_master_bus_handle_t bus_handle,
                        const mlx90614_config_t *mlx90614_config,
                        mlx90614_handle_t *mlx90614_handle);

/**
 * @brief Write data to MLX90614
 *
 * @param[in] mlx90614_handle MLX90614 handle
 * @param[in] command MLX90614 command
 * @param[in] data Data to write
 * @return ESP_OK: Write success. Otherwise failed, please check I2C function
 * fail reason.
 */
esp_err_t mlx90614_write(mlx90614_handle_t mlx90614_handle, uint8_t command,
                         const uint16_t data);

/**
 * @brief Read data from MLX90614
 *
 * @param mlx90614_handle MLX90614 handle
 * @param command MLX90614 command
 * @param data Data read from MLX90614
 * @return ESP_OK: Read success. Otherwise failed, please check I2C function
 * fail reason.
 */
esp_err_t mlx90614_read(mlx90614_handle_t mlx90614_handle, uint8_t command,
                        uint16_t *data);

/**
 * @brief Send command to MLX90614
 *
 * @param mlx90614_handle MLX90614 handle
 * @param command MLX90614 command
 * @return ESP_OK: Read success. Otherwise failed, please check I2C function
 * fail reason.
 */
esp_err_t mlx90614_command(mlx90614_handle_t mlx90614_handle, uint8_t command);

esp_err_t mlx90614_dump_eeprom(mlx90614_handle_t mlx90614_handle,
                               uint16_t *eeprom, size_t size);

esp_err_t mlx90614_get_ta(mlx90614_handle_t mlx90614_handle, float *ta);

esp_err_t mlx90614_get_to(mlx90614_handle_t mlx90614_handle, float *to);

esp_err_t mlx90614_get_to2(mlx90614_handle_t mlx90614_handle, float *to2);

esp_err_t mlx90614_get_ir_data1(mlx90614_handle_t mlx90614_handle,
                                uint16_t *ir1);

esp_err_t mlx90614_get_ir_data2(mlx90614_handle_t mlx90614_handle,
                                uint16_t *ir2);

esp_err_t mlx90614_get_emissivity(mlx90614_handle_t mlx90614_handle,
                                  float *emissivity);

esp_err_t mlx90614_set_emissivity(mlx90614_handle_t mlx90614_handle,
                                  float value);

esp_err_t mlx90614_get_fir(mlx90614_handle_t mlx90614_handle, uint8_t *fir);

esp_err_t mlx90614_set_fir(mlx90614_handle_t mlx90614_handle, uint8_t value);

esp_err_t mlx90614_get_iir(mlx90614_handle_t mlx90614_handle, uint8_t *iir);

esp_err_t mlx90614_set_iir(mlx90614_handle_t mlx90614_handle, uint8_t value);

float mlx90614_temperature_in_fahrenheit(float temperature);

int16_t mlx90614_convert_ir_data(uint16_t ir);

#ifdef __cplusplus
}
#endif
