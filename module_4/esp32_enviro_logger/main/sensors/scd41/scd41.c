#include "scd41.h"
#include "configuration.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <inttypes.h>
#include <stddef.h>

/* SCD41 commands */
#define SCD41_CMD_START_PERIODIC_MEASUREMENT 0x21B1U
#define SCD41_CMD_GET_SERIAL_NUMBER 0x3682U
#define SCD41_CMD_GET_DATA_READY_STATUS 0xE4B8U
#define SCD41_CMD_STOP_PERIODIC_MEASUREMENT 0x3F86U
#define SCD41_CMD_READ_MEASUREMENT 0xEC05U
#define SCD41_CMD_SET_AMBIENT_PRESSURE 0xE000U

/* SCD41 response format */
#define SCD41_SERIAL_WORD_COUNT 3U
#define SCD41_SERIAL_RESPONSE_SIZE 9U
#define SCD41_DATA_READY_RESPONSE_SIZE 3U
#define SCD41_MEASUREMENT_WORD_COUNT 3U
#define SCD41_MEASUREMENT_RESPONSE_SIZE 9U

/* SCD41 status fields */
#define SCD41_DATA_READY_MASK 0x07FFU

/* Sensirion CRC-8 */
#define SCD41_CRC_INITIAL_VALUE 0xFFU
#define SCD41_CRC_POLYNOMIAL 0x31U

#define SCD41_MIN_AMBIENT_PRESSURE_HPA 700.0f
#define SCD41_MAX_AMBIENT_PRESSURE_HPA 1200.0f

/* Driver timing */
#define SCD41_READ_COMMAND_WAIT_MS 10U
#define SCD41_STOP_COMMAND_WAIT_MS 500U
#define SCD41_WRITE_COMMAND_WAIT_MS 10U

static const char *TAG = "scd41";
static i2c_master_dev_handle_t dev;
static bool s_initialized;

static esp_err_t scd41_send_command(uint16_t command)
{
  if (dev == NULL)
  {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t data[2] = {
      (uint8_t)(command >> 8),
      (uint8_t)(command & 0xFFU),
  };

  return i2c_master_transmit(
      dev,
      data,
      sizeof(data),
      I2C_TIMEOUT_MS);
}

static uint8_t scd41_calculate_crc(
    const uint8_t *data,
    size_t length)
{
  uint8_t crc = SCD41_CRC_INITIAL_VALUE;

  for (size_t byte = 0; byte < length; ++byte)
  {
    crc ^= data[byte];

    for (unsigned bit = 0; bit < 8; ++bit)
    {
      if ((crc & 0x80U) != 0)
      {
        crc = (uint8_t)((crc << 1) ^ SCD41_CRC_POLYNOMIAL);
      }
      else
      {
        crc <<= 1;
      }
    }
  }

  return crc;
}

static esp_err_t scd41_send_command_with_word(uint16_t command,
                                              uint16_t word)
{
  if (dev == NULL)
  {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t data[5] = {
      (uint8_t)(command >> 8),
      (uint8_t)(command & 0xFF),
      (uint8_t)(word >> 8),
      (uint8_t)(word & 0xFF),
      0};

  data[4] = scd41_calculate_crc(&data[2], 2);

  return i2c_master_transmit(
      dev,
      data,
      sizeof(data),
      I2C_TIMEOUT_MS);
}

static esp_err_t scd41_start_periodic_measurement(void)
{
  esp_err_t err = scd41_send_command(SCD41_CMD_START_PERIODIC_MEASUREMENT);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to start periodic measurement: %s", esp_err_to_name(err));
  }

  return err;
}

static esp_err_t scd41_read_serial_number(uint64_t *serial_number)
{
  if (serial_number == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t err =
      scd41_send_command(SCD41_CMD_GET_SERIAL_NUMBER);

  if (err != ESP_OK)
  {
    return err;
  }

  /*
   * The datasheet specifies 1 ms. Waiting 10 ms ensures at
   * least one FreeRTOS tick with common ESP-IDF configurations.
   */
  vTaskDelay(pdMS_TO_TICKS(SCD41_READ_COMMAND_WAIT_MS));

  uint8_t response[SCD41_SERIAL_RESPONSE_SIZE];

  err = i2c_master_receive(
      dev,
      response,
      sizeof(response),
      I2C_TIMEOUT_MS);

  if (err != ESP_OK)
  {
    return err;
  }

  uint16_t words[SCD41_SERIAL_WORD_COUNT];

  for (size_t i = 0; i < SCD41_SERIAL_WORD_COUNT; ++i)
  {
    size_t offset = i * 3;

    uint8_t expected_crc =
        scd41_calculate_crc(&response[offset], 2);

    if (response[offset + 2] != expected_crc)
    {
      return ESP_ERR_INVALID_CRC;
    }

    words[i] =
        ((uint16_t)response[offset] << 8) |
        (uint16_t)response[offset + 1];
  }

  *serial_number =
      ((uint64_t)words[0] << 32) |
      ((uint64_t)words[1] << 16) |
      (uint64_t)words[2];

  return ESP_OK;
}

static esp_err_t scd41_stop_periodic_measurement(void)
{
  esp_err_t err =
      scd41_send_command(SCD41_CMD_STOP_PERIODIC_MEASUREMENT);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG,
             "Failed to stop periodic measurement: %s",
             esp_err_to_name(err));
    return err;
  }

  /*
   * The SCD41 needs up to 500 ms to return to idle mode.
   * No other command may be sent during this time.
   */
  vTaskDelay(pdMS_TO_TICKS(SCD41_STOP_COMMAND_WAIT_MS));

  return ESP_OK;
}

esp_err_t scd41_init(i2c_master_bus_handle_t bus_handle)
{
  if (bus_handle == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  if (s_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  i2c_device_config_t cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = ADDR_SCD41,
      .scl_speed_hz = I2C_HZ,
  };

  esp_err_t err = i2c_master_bus_add_device(bus_handle, &cfg, &dev);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to add device: %s", esp_err_to_name(err));
    return err;
  }

  err = scd41_stop_periodic_measurement();
  if (err != ESP_OK)
  {
    return err;
  }

  uint64_t serial_number = 0;
  err = scd41_read_serial_number(&serial_number);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG,
             "Failed to read serial number: %s",
             esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG,
           "Detected at 0x%02X, serial: %012" PRIX64,
           ADDR_SCD41,
           serial_number);

  err = scd41_start_periodic_measurement();
  if (err != ESP_OK)
  {
    return err;
  }

  s_initialized = true;

  return err;
}

esp_err_t scd41_is_data_ready(bool *ready)
{
  if (ready == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  if (!s_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  esp_err_t err =
      scd41_send_command(SCD41_CMD_GET_DATA_READY_STATUS);

  if (err != ESP_OK)
  {
    return err;
  }

  vTaskDelay(pdMS_TO_TICKS(SCD41_READ_COMMAND_WAIT_MS));

  uint8_t response[SCD41_DATA_READY_RESPONSE_SIZE];

  err = i2c_master_receive(
      dev,
      response,
      sizeof(response),
      I2C_TIMEOUT_MS);

  if (err != ESP_OK)
  {
    return err;
  }

  uint8_t expected_crc =
      scd41_calculate_crc(response, 2);

  if (response[2] != expected_crc)
  {
    return ESP_ERR_INVALID_CRC;
  }

  uint16_t status =
      ((uint16_t)response[0] << 8) |
      (uint16_t)response[1];

  *ready = (status & SCD41_DATA_READY_MASK) != 0;

  return ESP_OK;
}

esp_err_t scd41_read(scd41_data_t *out)
{
  if (out == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  if (!s_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  esp_err_t err =
      scd41_send_command(SCD41_CMD_READ_MEASUREMENT);

  if (err != ESP_OK)
  {
    return err;
  }

  vTaskDelay(pdMS_TO_TICKS(SCD41_READ_COMMAND_WAIT_MS));

  uint8_t response[SCD41_MEASUREMENT_RESPONSE_SIZE];

  err = i2c_master_receive(
      dev,
      response,
      sizeof(response),
      I2C_TIMEOUT_MS);

  if (err != ESP_OK)
  {
    return err;
  }

  uint16_t words[SCD41_MEASUREMENT_WORD_COUNT];

  for (size_t i = 0;
       i < SCD41_MEASUREMENT_WORD_COUNT;
       ++i)
  {
    size_t offset = i * 3U;

    uint8_t expected_crc =
        scd41_calculate_crc(&response[offset], 2U);

    if (response[offset + 2U] != expected_crc)
    {
      return ESP_ERR_INVALID_CRC;
    }

    words[i] =
        ((uint16_t)response[offset] << 8) |
        (uint16_t)response[offset + 1U];
  }

  uint16_t raw_co2 = words[0];
  uint16_t raw_temperature = words[1];
  uint16_t raw_humidity = words[2];

  out->co2_ppm = raw_co2;

  out->temperature =
      -45.0f +
      175.0f * (float)raw_temperature / 65535.0f;

  out->humidity =
      100.0f * (float)raw_humidity / 65535.0f;

  return ESP_OK;
}

esp_err_t scd41_set_ambient_pressure(uint16_t pressure_hpa)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (pressure_hpa < 700U || pressure_hpa > 1200U) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = scd41_send_command_with_word(
        SCD41_CMD_SET_AMBIENT_PRESSURE,
        pressure_hpa
    );

    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(SCD41_WRITE_COMMAND_WAIT_MS));

    ESP_LOGI(TAG, "Pressure compensation set: %u hPa", pressure_hpa);

    return ESP_OK;
}