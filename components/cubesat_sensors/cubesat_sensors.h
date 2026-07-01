#ifndef CUBESAT_SENSORS_H
#define CUBESAT_SENSORS_H

#include "esp_err.h"
#include "driver/i2c_master.h"


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

//INA Variables and Structs.






#ifdef __cplusplus
}

#endif

#endif