#include "bme280.h"
#include "configuration.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define BME280_REG_CHIP_ID 0xD0
#define BME280_CHIP_ID 0x60
#define BME280_REG_RESET 0xE0
#define BME280_RESET_CMD 0xB6
#define BME280_REG_STATUS 0xF3
#define BME280_STATUS_IM_UPDATE_MASK 0x01

#define BME280_REG_CALIB_TP_START 0x88
#define BME280_CALIB_TP_SIZE 26
#define BME280_REG_CALIB_H_START 0xE1
#define BME280_CALIB_H_SIZE 7

#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_DATA_START 0xF7
#define BME280_DATA_SIZE 8

#define BME280_RAW_20_BIT_INVALID 0x80000
#define BME280_RAW_HUMIDITY_INVALID 0x8000

#define BME280_OVERSAMPLING_X1 0x01
#define BME280_MODE_NORMAL 0x03
#define BME280_STANDBY_500_MS 0x04
#define BME280_FILTER_OFF 0x00

#define BME280_RESET_WAIT_MS 5
#define BME280_STATUS_POLL_MS 2
#define BME280_STATUS_MAX_POLLS 10

typedef struct
{
  uint16_t dig_t1;
  int16_t dig_t2;
  int16_t dig_t3;

  uint16_t dig_p1;
  int16_t dig_p2;
  int16_t dig_p3;
  int16_t dig_p4;
  int16_t dig_p5;
  int16_t dig_p6;
  int16_t dig_p7;
  int16_t dig_p8;
  int16_t dig_p9;

  uint8_t dig_h1;
  int16_t dig_h2;
  uint8_t dig_h3;
  int16_t dig_h4;
  int16_t dig_h5;
  int8_t dig_h6;
} bme280_calibration_t;

static const char *TAG = "bme280";
static bme280_calibration_t calibration;
static i2c_master_dev_handle_t dev;

static uint16_t read_u16_le(const uint8_t *data)
{
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static int16_t read_s16_le(const uint8_t *data)
{
  return (int16_t)read_u16_le(data);
}

static int16_t sign_extend_12(uint16_t value)
{
  value &= 0x0FFFU;

  if ((value & 0x0800U) != 0)
  {
    value |= 0xF000U;
  }

  return (int16_t)value;
}

static esp_err_t bme280_read_registers(
    uint8_t start_reg,
    uint8_t *buffer,
    size_t length)
{
  if (buffer == NULL || length == 0)
  {
    return ESP_ERR_INVALID_ARG;
  }

  return i2c_master_transmit_receive(
      dev,
      &start_reg,
      sizeof(start_reg),
      buffer,
      length,
      I2C_TIMEOUT_MS);
}

static esp_err_t bme280_read_register(uint8_t reg, uint8_t *value)
{
  return bme280_read_registers(reg, value, 1);
}

static esp_err_t bme280_write_register(uint8_t reg, uint8_t value)
{
  uint8_t data[] = {reg, value};

  return i2c_master_transmit(
      dev,
      data,
      sizeof(data),
      I2C_TIMEOUT_MS);
}

static esp_err_t bme280_soft_reset(void)
{
  esp_err_t err =
      bme280_write_register(BME280_REG_RESET, BME280_RESET_CMD);

  if (err != ESP_OK)
  {
    return err;
  }

  vTaskDelay(pdMS_TO_TICKS(BME280_RESET_WAIT_MS));

  for (unsigned attempt = 0;
       attempt < BME280_STATUS_MAX_POLLS;
       ++attempt)
  {
    uint8_t status = 0;

    err = bme280_read_register(BME280_REG_STATUS, &status);
    if (err != ESP_OK)
    {
      return err;
    }

    if ((status & BME280_STATUS_IM_UPDATE_MASK) == 0)
    {
      return ESP_OK;
    }

    vTaskDelay(pdMS_TO_TICKS(BME280_STATUS_POLL_MS));
  }

  return ESP_ERR_TIMEOUT;
}

static esp_err_t bme280_read_calibration(void)
{
  uint8_t tp[BME280_CALIB_TP_SIZE];
  uint8_t h[BME280_CALIB_H_SIZE];

  esp_err_t err = bme280_read_registers(
      BME280_REG_CALIB_TP_START,
      tp,
      sizeof(tp));

  if (err != ESP_OK)
  {
    return err;
  }

  err = bme280_read_registers(
      BME280_REG_CALIB_H_START,
      h,
      sizeof(h));

  if (err != ESP_OK)
  {
    return err;
  }

  calibration.dig_t1 = read_u16_le(&tp[0]);
  calibration.dig_t2 = read_s16_le(&tp[2]);
  calibration.dig_t3 = read_s16_le(&tp[4]);

  calibration.dig_p1 = read_u16_le(&tp[6]);
  calibration.dig_p2 = read_s16_le(&tp[8]);
  calibration.dig_p3 = read_s16_le(&tp[10]);
  calibration.dig_p4 = read_s16_le(&tp[12]);
  calibration.dig_p5 = read_s16_le(&tp[14]);
  calibration.dig_p6 = read_s16_le(&tp[16]);
  calibration.dig_p7 = read_s16_le(&tp[18]);
  calibration.dig_p8 = read_s16_le(&tp[20]);
  calibration.dig_p9 = read_s16_le(&tp[22]);

  calibration.dig_h1 = tp[25];
  calibration.dig_h2 = read_s16_le(&h[0]);
  calibration.dig_h3 = h[2];

  uint16_t raw_h4 =
      ((uint16_t)h[3] << 4) | (h[4] & 0x0FU);

  uint16_t raw_h5 =
      ((uint16_t)h[5] << 4) | (h[4] >> 4);

  calibration.dig_h4 = sign_extend_12(raw_h4);
  calibration.dig_h5 = sign_extend_12(raw_h5);
  calibration.dig_h6 = (int8_t)h[6];

  if (calibration.dig_t1 == 0 || calibration.dig_p1 == 0)
  {
    return ESP_ERR_INVALID_RESPONSE;
  }

  ESP_LOGI(TAG,
           "Calibration loaded: T1=%u P1=%u H1=%u",
           (unsigned)calibration.dig_t1,
           (unsigned)calibration.dig_p1,
           (unsigned)calibration.dig_h1);

  return ESP_OK;
}

static esp_err_t bme280_configure(void)
{
  /*
   * ctrl_hum:
   * bits 2:0 = humidity oversampling
   */
  uint8_t ctrl_hum = BME280_OVERSAMPLING_X1;

  esp_err_t err =
      bme280_write_register(BME280_REG_CTRL_HUM, ctrl_hum);

  if (err != ESP_OK)
  {
    return err;
  }

  /*
   * config:
   * bits 7:5 = standby time
   * bits 4:2 = filter coefficient
   */
  uint8_t config =
      (BME280_STANDBY_500_MS << 5) |
      (BME280_FILTER_OFF << 2);

  err = bme280_write_register(BME280_REG_CONFIG, config);
  if (err != ESP_OK)
  {
    return err;
  }

  /*
   * ctrl_meas:
   * bits 7:5 = temperature oversampling
   * bits 4:2 = pressure oversampling
   * bits 1:0 = sensor mode
   */
  uint8_t ctrl_meas =
      (BME280_OVERSAMPLING_X1 << 5) |
      (BME280_OVERSAMPLING_X1 << 2) |
      BME280_MODE_NORMAL;

  return bme280_write_register(BME280_REG_CTRL_MEAS, ctrl_meas);
}

static double bme280_compensate_temperature(
    int32_t raw_temperature,
    double *t_fine)
{
  double var1 =
      ((double)raw_temperature / 16384.0 -
       (double)calibration.dig_t1 / 1024.0) *
      (double)calibration.dig_t2;

  double temperature_delta =
      (double)raw_temperature / 131072.0 -
      (double)calibration.dig_t1 / 8192.0;

  double var2 =
      temperature_delta * temperature_delta *
      (double)calibration.dig_t3;

  *t_fine = var1 + var2;
  return *t_fine / 5120.0;
}

static double bme280_compensate_pressure(
    int32_t raw_pressure,
    double t_fine)
{
  double var1 = t_fine / 2.0 - 64000.0;
  double var2 = var1 * var1 * (double)calibration.dig_p6 / 32768.0;

  var2 += var1 * (double)calibration.dig_p5 * 2.0;
  var2 = var2 / 4.0 + (double)calibration.dig_p4 * 65536.0;

  var1 =
      ((double)calibration.dig_p3 * var1 * var1 / 524288.0 +
       (double)calibration.dig_p2 * var1) /
      524288.0;

  var1 =
      (1.0 + var1 / 32768.0) *
      (double)calibration.dig_p1;

  if (var1 == 0.0)
  {
    return 0.0;
  }

  double pressure = 1048576.0 - (double)raw_pressure;
  pressure = (pressure - var2 / 4096.0) * 6250.0 / var1;

  var1 =
      (double)calibration.dig_p9 * pressure * pressure /
      2147483648.0;
  var2 = pressure * (double)calibration.dig_p8 / 32768.0;

  return pressure +
         (var1 + var2 + (double)calibration.dig_p7) / 16.0;
}

static double bme280_compensate_humidity(
    int32_t raw_humidity,
    double t_fine)
{
  double humidity = t_fine - 76800.0;

  humidity =
      ((double)raw_humidity -
       ((double)calibration.dig_h4 * 64.0 +
        (double)calibration.dig_h5 / 16384.0 * humidity)) *
      ((double)calibration.dig_h2 / 65536.0 *
       (1.0 +
        (double)calibration.dig_h6 / 67108864.0 * humidity *
            (1.0 +
             (double)calibration.dig_h3 / 67108864.0 * humidity)));

  humidity *=
      1.0 - (double)calibration.dig_h1 * humidity / 524288.0;

  if (humidity > 100.0)
  {
    return 100.0;
  }

  if (humidity < 0.0)
  {
    return 0.0;
  }

  return humidity;
}

esp_err_t bme280_init(i2c_master_bus_handle_t bus_handle)
{
  if (bus_handle == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  i2c_device_config_t cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = ADDR_BME280,
      .scl_speed_hz = I2C_HZ,
  };

  esp_err_t err = i2c_master_bus_add_device(bus_handle, &cfg, &dev);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to add BME280: %s", esp_err_to_name(err));
    return err;
  }

  uint8_t chip_id = 0;
  err = bme280_read_register(BME280_REG_CHIP_ID, &chip_id);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read chip ID: %s", esp_err_to_name(err));
    return err;
  }

  if (chip_id != BME280_CHIP_ID)
  {
    ESP_LOGE(TAG,
             "Unexpected chip ID: 0x%02X, expected 0x%02X",
             (unsigned)chip_id,
             BME280_CHIP_ID);

    return ESP_ERR_INVALID_RESPONSE;
  }

  ESP_LOGI(TAG,
           "Detected at 0x%02X, chip ID: 0x%02X",
           ADDR_BME280,
           (unsigned)chip_id);

  err = bme280_soft_reset();
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to reset: %s", esp_err_to_name(err));
    return err;
  }

  err = bme280_read_calibration();
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read calibration: %s", esp_err_to_name(err));
    return err;
  }

  err = bme280_configure();
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to configure: %s",
             esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "BME280 initialized");
  return ESP_OK;
}

esp_err_t bme280_read(bme280_data_t *out)
{
  if (out == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  if (dev == NULL)
  {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t data[BME280_DATA_SIZE];
  esp_err_t err = bme280_read_registers(
      BME280_REG_DATA_START,
      data,
      sizeof(data));

  if (err != ESP_OK)
  {
    return err;
  }

  int32_t raw_pressure =
      ((int32_t)data[0] << 12) |
      ((int32_t)data[1] << 4) |
      ((int32_t)data[2] >> 4);

  int32_t raw_temperature =
      ((int32_t)data[3] << 12) |
      ((int32_t)data[4] << 4) |
      ((int32_t)data[5] >> 4);

  int32_t raw_humidity =
      ((int32_t)data[6] << 8) |
      (int32_t)data[7];

  if (raw_temperature == BME280_RAW_20_BIT_INVALID ||
      raw_pressure == BME280_RAW_20_BIT_INVALID ||
      raw_humidity == BME280_RAW_HUMIDITY_INVALID)
  {
    return ESP_ERR_INVALID_RESPONSE;
  }

  double t_fine = 0.0;
  double temperature =
      bme280_compensate_temperature(raw_temperature, &t_fine);
  double pressure_pa =
      bme280_compensate_pressure(raw_pressure, t_fine);
  double humidity =
      bme280_compensate_humidity(raw_humidity, t_fine);

  out->temperature = (float)temperature;
  out->pressure = (float)(pressure_pa / 100.0);
  out->humidity = (float)humidity;

  return ESP_OK;
}
