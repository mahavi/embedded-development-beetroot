#include "oled.h"

#include "esp_log.h"
#include <string.h>

#define OLED_WIDTH 128
#define OLED_PAGES 8

static const uint8_t init_commands[] = {
    0x00, /* Control byte: all following bytes are commands. */
    0xAE, /* Display OFF */
    0xD5, 0x80, /* Clock frequency / divider. */
    0xA8, 0x3F, /* Multiplex ratio: 64 rows. */
    0xD3, 0x00, /* Display offset. */
    0x40, /* RAM display start line. */
    0x8D, 0x14, /* Charge pump ON: required at 3.3 V. */
    0xA1, /* Segment remapping. */
    0xC8, /* COM scan direction. */
    0xDA, 0x12, /* COM pin configuration for 128x64. */
    0x81, 0x7F, /* Contrast. */
    0x20, 0x00, /* Horizontal RAM addressing. */
    0xA6, /* Normal polarity. */
    0xA4, /* Display RAM contents. */
    0xAF, /* Display ON */
};

static const char *TAG = "oled";
static i2c_master_dev_handle_t dev;

/* The first byte, 0x40, indicates data; the rest represents 128x64 pixels, 8 per byte. */
static uint8_t framebuffer[1 + OLED_WIDTH * OLED_PAGES] = {0x40};

/* 5x7 font: each byte represents one column; the least significant bit is the top pixel. */
static const uint8_t font[95][5] = {
    ['.' - ' '] = {0x00, 0x60, 0x60, 0x00, 0x00},
    [':' - ' '] = {0x00, 0x36, 0x36, 0x00, 0x00},
    ['?' - ' '] = {0x02, 0x01, 0x51, 0x09, 0x06},
    ['0' - ' '] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ['1' - ' '] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ['2' - ' '] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3' - ' '] = {0x21, 0x41, 0x45, 0x4B, 0x31},
    ['4' - ' '] = {0x18, 0x14, 0x12, 0x7F, 0x10},
    ['5' - ' '] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ['6' - ' '] = {0x3C, 0x4A, 0x49, 0x49, 0x30},
    ['7' - ' '] = {0x01, 0x71, 0x09, 0x05, 0x03},
    ['8' - ' '] = {0x36, 0x49, 0x49, 0x49, 0x36},
    ['9' - ' '] = {0x06, 0x49, 0x49, 0x29, 0x1E},
    ['F' - ' '] = {0x7F, 0x09, 0x09, 0x09, 0x01},
    ['M' - ' '] = {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    ['S' - ' '] = {0x46, 0x49, 0x49, 0x49, 0x31},
    ['T' - ' '] = {0x01, 0x01, 0x7F, 0x01, 0x01},
    ['W' - ' '] = {0x7F, 0x20, 0x18, 0x20, 0x7F},
    ['a' - ' '] = {0x20, 0x54, 0x54, 0x54, 0x78},
    ['d' - ' '] = {0x38, 0x44, 0x44, 0x48, 0x7F},
    ['e' - ' '] = {0x38, 0x54, 0x54, 0x54, 0x18},
    ['h' - ' '] = {0x7F, 0x08, 0x04, 0x04, 0x78},
    ['i' - ' '] = {0x00, 0x44, 0x7D, 0x40, 0x00},
    ['n' - ' '] = {0x7C, 0x08, 0x04, 0x04, 0x78},
    ['o' - ' '] = {0x38, 0x44, 0x44, 0x44, 0x38},
    ['r' - ' '] = {0x7C, 0x08, 0x04, 0x04, 0x08},
    ['t' - ' '] = {0x04, 0x3F, 0x44, 0x40, 0x20},
    ['u' - ' '] = {0x3C, 0x40, 0x40, 0x20, 0x7C},
};

static esp_err_t oled_flush(void)
{
  const uint8_t window[] = {
      0x00, 0xA4,
      0x21, 0x00, OLED_WIDTH - 1,
      0x22, 0x00, OLED_PAGES - 1,
  };
  esp_err_t err = i2c_master_transmit(dev, window, sizeof(window), I2C_TIMEOUT_MS);
  if (err != ESP_OK)
  {
    return err;
  }
  return i2c_master_transmit(dev, framebuffer, sizeof(framebuffer), I2C_TIMEOUT_MS);
}

static void draw_text(uint8_t page, const char *text)
{
  size_t width = strlen(text) * 6; /* 5 character columns + 1 spacing column. */
  size_t column = width < OLED_WIDTH ? (OLED_WIDTH - width) / 2 : 0;

  while (*text != '\0' && column + 5 <= OLED_WIDTH)
  {
    unsigned char character = (unsigned char)*text++;
    if (character < ' ' || character > '~')
    {
      character = '?';
    }
    memcpy(&framebuffer[1 + page * OLED_WIDTH + column], font[character - ' '], 5);
    column += 6;
  }
}

void oled_init(i2c_master_bus_handle_t bus_handle)
{
  i2c_device_config_t cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = ADDR_OLED,
      .scl_speed_hz = I2C_HZ,
  };
  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &cfg, &dev));
  ESP_ERROR_CHECK(i2c_master_transmit(dev, init_commands, sizeof(init_commands), I2C_TIMEOUT_MS));
  memset(&framebuffer[1], 0, sizeof(framebuffer) - 1);
  ESP_ERROR_CHECK(oled_flush());

  ESP_LOGI(TAG, "OLED 0x%02X: Initialized", ADDR_OLED);
}

esp_err_t oled_show_datetime(const char *time_text, const char *date_text)
{
  if (time_text == NULL || date_text == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }
  if (dev == NULL)
  {
    return ESP_ERR_INVALID_STATE;
  }

  memset(&framebuffer[1], 0, sizeof(framebuffer) - 1);
  draw_text(2, time_text);
  draw_text(4, date_text);
  return oled_flush();
}
