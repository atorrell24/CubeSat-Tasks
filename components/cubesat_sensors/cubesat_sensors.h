#ifndef CUBESAT_SENSORS_H
#define CUBESAT_SENSORS_H

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif


esp_err_t sensors_init(void);
esp_err_t sensors_read_object_temp(float *temp_c);

#ifdef __cplusplus
}

#endif

#endif