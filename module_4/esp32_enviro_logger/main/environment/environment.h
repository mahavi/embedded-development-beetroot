#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "driver/i2c_master.h"
#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    float bme_temperature;
    float bme_humidity;
    float pressure_hpa;

    uint16_t co2_ppm;
    float scd_temperature;
    float scd_humidity;

    bool has_bme_sample;
    bool has_scd_sample;
} environment_data_t;

esp_err_t environment_init(i2c_master_bus_handle_t bus);
void environment_update(void);
const environment_data_t *environment_get_data(void);

#endif // ENVIRONMENT_H