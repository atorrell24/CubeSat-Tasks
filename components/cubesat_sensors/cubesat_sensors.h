#ifndef CUBESAT_SENSORS_H
#define CUBESAT_SENSORS_H

#include "esp_err.h"
#include "driver/i2c_master.h"

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


esp_err_t sensors_init(void);


//MLX Variables and Strcuts.
esp_err_t sensors_read_object_temp(float *temp_c);
esp_err_t sensors_read_object_ambient(float *temp_a);

typedef enum {
    SENSOR_NOMINAL,
    SENSOR_WARNING,
    SENSOR_CRITICAL,
} sensor_status_t;

typedef struct {
    float object_temp_c;
    float ambient_temp_c;
    sensor_status_t status;
} mlx_data_t;

esp_err_t sensors_read_mlx(mlx_data_t *out);

// Variables and Structs.

typedef struct {
    uint32_t utc_seconds;
    uint32_t utc_date;
    double lat_deg;
    double lon_deg;
    float altitude_m;
    uint8_t satellites;
    bool valid;
    sensor_status_t status;

} atg_data_t;

esp_err_t sensors_read_bno_accel(
    float *x,
    float *y,
    float *z
);

esp_err_t gps_init(void);
esp_err_t sensors_read_gps(atg_data_t *out);




#ifdef __cplusplus
}

#endif

#endif