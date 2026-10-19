#ifndef SCD41_H_
#define SCD41_H_

#include "driver/i2c_master.h"
#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  uint16_t co2_ppm;
  float temperature;
  float humidity;
} scd41_data_t;

esp_err_t scd41_init(i2c_master_bus_handle_t bus_handle);
esp_err_t scd41_is_data_ready(bool *ready);
esp_err_t scd41_read(scd41_data_t *out);
esp_err_t scd41_set_ambient_pressure(uint16_t pressure_hpa);

#endif // SCD41_H_