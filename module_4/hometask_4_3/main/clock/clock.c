#include "clock.h"

#include "esp_log.h"

static const char *TAG = "rtc";
static i2c_master_dev_handle_t dev;

/* DS1307 stores calendar values in BCD: the upper nibble is tens, the lower is ones. */
static uint8_t bcd(uint8_t v)
{
  return (uint8_t)((v >> 4) * 10 + (v & 0x0F));
}

/* Write the register address, issue a repeated START, then read: the transaction shown on the slide. */
static esp_err_t rtc_read(i2c_master_dev_handle_t dev, uint8_t *buf, size_t n)
{
  uint8_t reg = 0x00; /* Start at the seconds register; subsequent reads auto-increment the address. */
  return i2c_master_transmit_receive(dev, &reg, 1, buf, n, I2C_TIMEOUT_MS);
}

esp_err_t clock_read_datetime(clock_datetime_t *out)
{
  if (out == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  if (dev == NULL)
  {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t registers[7];
  esp_err_t err = rtc_read(dev, registers, sizeof(registers));
  if (err != ESP_OK)
  {
    return err;
  }

  uint8_t hour;
  if (registers[2] & 0x40) // 12-hour mode
  {
    hour = bcd(registers[2] & 0x1F) % 12;

    if (registers[2] & 0x20) // PM
    {
      hour += 12;
    }
  }
  else // 24-hour mode
  {
    hour = bcd(registers[2] & 0x3F);
  }

  *out = (clock_datetime_t){
      .year = 2000 + bcd(registers[6]),
      .month = bcd(registers[5] & 0x1F),
      .day = bcd(registers[4] & 0x3F),
      .weekday = registers[3] & 0x07,
      .hour = hour,
      .minute = bcd(registers[1] & 0x7F),
      .second = bcd(registers[0] & 0x7F),
  };

  return ESP_OK;
}

void clock_init(i2c_master_bus_handle_t bus_handle)
{
  i2c_device_config_t cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = ADDR_RTC,
      .scl_speed_hz = I2C_HZ,
  };
  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &cfg, &dev));

  ESP_LOGI(TAG, "RTC 0x%02X: Initialized", ADDR_RTC);
}
