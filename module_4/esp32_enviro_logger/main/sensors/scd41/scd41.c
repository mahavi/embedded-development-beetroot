#include "scd41.h"
#include "configuration.h"

#include "esp_log.h"

static const char *TAG = "scd41";
static i2c_master_dev_handle_t dev;

esp_err_t scd41_init(i2c_master_bus_handle_t bus_handle)
{
  if (bus_handle == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  i2c_device_config_t cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = ADDR_SCD41,
      .scl_speed_hz = I2C_HZ,
  };

  esp_err_t err = i2c_master_bus_add_device(bus_handle, &cfg, &dev);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to add BME280: %s", esp_err_to_name(err));
    return err;
  }

  return ESP_OK;
}

esp_err_t scd41_is_data_ready(bool *ready)
{
  return ESP_OK;
}

esp_err_t scd41_read(scd41_data_t *out)
{
  return ESP_OK;
}