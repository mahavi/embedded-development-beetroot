#ifndef BME280_H_
#define BME280_H_

#include "driver/i2c_master.h"
#include "esp_err.h"

typedef struct
{
  float humidity;     // 10% - 90% at 0°C-65°C
  float pressure;     // 300hPa - 1100hPa at 0°C-65°C
  float temperature;  // -40°C - 85°C
} bme280_data_t;

esp_err_t bme280_init(i2c_master_bus_handle_t bus_handle);
esp_err_t bme280_read(bme280_data_t *out);

#endif // BME280_H_