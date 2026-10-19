#include "environment.h"

#include "sensors/bme280/bme280.h"
#include "sensors/scd41/scd41.h"

#include "esp_log.h"

#define SCD_PRESSURE_UPDATE_THRESHOLD_HPA 2U

static const char *TAG = "environment";

static environment_data_t s_data;
static bool s_initialized;
static bool s_scd_pressure_set;
static uint16_t s_last_scd_pressure_hpa;

static void update_scd_pressure(float pressure_hpa)
{
  uint16_t rounded_pressure_hpa =
      (uint16_t)(pressure_hpa + 0.5f);

  uint16_t pressure_difference =
      rounded_pressure_hpa > s_last_scd_pressure_hpa
          ? rounded_pressure_hpa - s_last_scd_pressure_hpa
          : s_last_scd_pressure_hpa - rounded_pressure_hpa;

  if (s_scd_pressure_set &&
      pressure_difference < SCD_PRESSURE_UPDATE_THRESHOLD_HPA)
  {
    return;
  }

  esp_err_t err =
      scd41_set_ambient_pressure(rounded_pressure_hpa);

  if (err != ESP_OK)
  {
    ESP_LOGW(TAG,
             "Failed to update SCD41 pressure: %s",
             esp_err_to_name(err));
    return;
  }

  s_last_scd_pressure_hpa = rounded_pressure_hpa;
  s_scd_pressure_set = true;
}

static bool update_bme(void)
{
  bme280_data_t sample;
  esp_err_t err = bme280_read(&sample);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG,
             "Failed to read BME280: %s",
             esp_err_to_name(err));
    return false;
  }

  s_data.bme_temperature = sample.temperature;
  s_data.bme_humidity = sample.humidity;
  s_data.pressure_hpa = sample.pressure;
  s_data.has_bme_sample = true;

  update_scd_pressure(sample.pressure);
  return true;
}

static bool update_scd(void)
{
  bool ready = false;
  esp_err_t err = scd41_is_data_ready(&ready);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG,
             "Failed to get SCD41 data status: %s",
             esp_err_to_name(err));
    return false;
  }

  if (!ready)
  {
    return false;
  }

  scd41_data_t sample;
  err = scd41_read(&sample);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG,
             "Failed to read SCD41: %s",
             esp_err_to_name(err));
    return false;
  }

  s_data.co2_ppm = sample.co2_ppm;
  s_data.scd_temperature = sample.temperature;
  s_data.scd_humidity = sample.humidity;
  s_data.has_scd_sample = true;

  return true;
}

static void log_data(bool scd_updated)
{
  if (!s_data.has_bme_sample)
  {
    return;
  }

  if (!s_data.has_scd_sample)
  {
    ESP_LOGI(TAG,
             "BME: T %.1f C, RH %.1f%%, P %.1f hPa | "
             "SCD41: waiting for first measurement",
             (double)s_data.bme_temperature,
             (double)s_data.bme_humidity,
             (double)s_data.pressure_hpa);
    return;
  }

  ESP_LOGI(TAG,
           "BME: T %.1f C, RH %.1f%%, P %.1f hPa | "
           "SCD41: CO2 %u ppm, T %.1f C, RH %.1f%%%s",
           (double)s_data.bme_temperature,
           (double)s_data.bme_humidity,
           (double)s_data.pressure_hpa,
           (unsigned)s_data.co2_ppm,
           (double)s_data.scd_temperature,
           (double)s_data.scd_humidity,
           scd_updated ? "" : " (cached)");
}

esp_err_t environment_init(i2c_master_bus_handle_t bus)
{
  if (bus == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  if (s_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  esp_err_t err = bme280_init(bus);
  if (err != ESP_OK)
  {
    return err;
  }

  err = scd41_init(bus);
  if (err != ESP_OK)
  {
    return err;
  }

  s_initialized = true;
  return ESP_OK;
}

void environment_update(void)
{
  if (!s_initialized)
  {
    ESP_LOGE(TAG, "Environment is not initialized");
    return;
  }

  update_bme();
  bool scd_updated = update_scd();
  log_data(scd_updated);
}

const environment_data_t *environment_get_data(void)
{
  return &s_data;
}
