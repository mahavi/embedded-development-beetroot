#include "display.h"
#include "configuration.h"
#include "font5x7.h"

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SSD1306_WIDTH 128U
#define SSD1306_HEIGHT 64U
#define SSD1306_PAGE_HEIGHT 8U
#define SSD1306_PAGE_COUNT \
  (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)

#define SSD1306_FRAMEBUFFER_SIZE \
  (SSD1306_WIDTH * SSD1306_HEIGHT / 8U)

#define SSD1306_CMD_SET_COLUMN_ADDRESS 0x21U
#define SSD1306_CMD_SET_PAGE_ADDRESS 0x22U

#define SSD1306_RESET_DELAY_MS 10U

static const char *TAG = "display";
static spi_device_handle_t s_display;
static bool s_initialized;
DMA_ATTR static uint8_t
    s_framebuffer[SSD1306_FRAMEBUFFER_SIZE];

static esp_err_t display_send_command(uint8_t command)
{
  if (s_display == NULL)
  {
    return ESP_ERR_INVALID_STATE;
  }

  esp_err_t err = gpio_set_level(OLED_DC_GPIO, 0);
  if (err != ESP_OK)
  {
    return err;
  }

  spi_transaction_t transaction = {
      .length = 8,
      .flags = SPI_TRANS_USE_TXDATA,
  };

  transaction.tx_data[0] = command;

  return spi_device_transmit(s_display, &transaction);
}

static esp_err_t display_send_data(
    const uint8_t *data,
    size_t length)
{
  if (data == NULL || length == 0)
  {
    return ESP_ERR_INVALID_ARG;
  }

  if (s_display == NULL || !s_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  esp_err_t err = gpio_set_level(OLED_DC_GPIO, 1);
  if (err != ESP_OK)
  {
    return err;
  }

  spi_transaction_t transaction = {
      .length = length * 8U,
      .tx_buffer = data,
  };

  return spi_device_transmit(
      s_display,
      &transaction);
}

static esp_err_t display_reset(void)
{
  esp_err_t err = gpio_set_level(OLED_RST_GPIO, 0);
  if (err != ESP_OK)
  {
    return err;
  }

  vTaskDelay(pdMS_TO_TICKS(SSD1306_RESET_DELAY_MS));

  err = gpio_set_level(OLED_RST_GPIO, 1);
  if (err != ESP_OK)
  {
    return err;
  }

  vTaskDelay(pdMS_TO_TICKS(SSD1306_RESET_DELAY_MS));

  return ESP_OK;
}

esp_err_t display_init(spi_host_device_t spi_host)
{
  if (s_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  gpio_config_t gpio_cfg = {
      .pin_bit_mask =
          (1ULL << OLED_DC_GPIO) |
          (1ULL << OLED_RST_GPIO),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };

  esp_err_t err = gpio_config(&gpio_cfg);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to configure: %s",
             esp_err_to_name(err));
    return err;
  }

  spi_device_interface_config_t device_cfg = {
      .clock_speed_hz = SPI_HZ,
      .mode = 0,
      .spics_io_num = OLED_CS_GPIO,
      .queue_size = 1,
      .flags = SPI_DEVICE_HALFDUPLEX,
  };

  err = spi_bus_add_device(
      spi_host,
      &device_cfg,
      &s_display);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to add device: %s",
             esp_err_to_name(err));
    return err;
  }

  err = display_reset();
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to reset: %s",
             esp_err_to_name(err));
    return err;
  }

  static const uint8_t init_commands[] = {
      0xAE, // Display off
      0xD5,
      0x80, // Clock divide ratio
      0xA8,
      0x3F, // Multiplex ratio: 64 rows
      0xD3,
      0x00, // Display offset
      0x40, // Display start line 0
      0x8D,
      0x14, // Enable charge pump
      0x20,
      0x00, // Horizontal addressing mode
      0xA1, // Segment remap
      0xC8, // Reverse COM scan direction
      0xDA,
      0x12, // COM pin configuration for 128x64
      0x81,
      0x7F, // Contrast
      0xD9,
      0xF1, // Pre-charge period
      0xDB,
      0x40, // VCOMH deselect level
      0xA4, // Use display RAM contents
      0xA6, // Normal, non-inverted display
      0x2E, // Disable scrolling
      0xAF, // Display on
  };

  for (size_t i = 0;
       i < sizeof(init_commands);
       ++i)
  {
    err = display_send_command(init_commands[i]);

    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to configure: %s",
               esp_err_to_name(err));
      return err;
    }
  }

  s_initialized = true;

  return ESP_OK;
}

esp_err_t display_clear_buffer(void)
{
  if (!s_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  memset(s_framebuffer, 0, sizeof(s_framebuffer));

  return ESP_OK;
}

esp_err_t display_clear(void)
{
  esp_err_t err = display_clear_buffer();

  if (err != ESP_OK)
  {
    return err;
  }

  return display_flush();
}

esp_err_t display_flush(void)
{
  if (!s_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  static const uint8_t address_commands[] = {
      SSD1306_CMD_SET_COLUMN_ADDRESS,
      0U,
      SSD1306_WIDTH - 1U,

      SSD1306_CMD_SET_PAGE_ADDRESS,
      0U,
      SSD1306_PAGE_COUNT - 1U,
  };

  for (size_t i = 0; i < sizeof(address_commands); ++i)
  {
    esp_err_t err = display_send_command(address_commands[i]);

    if (err != ESP_OK)
    {
      return err;
    }
  }

  return display_send_data(s_framebuffer, sizeof(s_framebuffer));
}

esp_err_t display_draw_pixel(
    uint8_t x,
    uint8_t y,
    bool enabled)
{
  if (!s_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  if (x >= SSD1306_WIDTH ||
      y >= SSD1306_HEIGHT)
  {
    return ESP_ERR_INVALID_ARG;
  }

  size_t index =
      (size_t)x +
      ((size_t)y / SSD1306_PAGE_HEIGHT) *
          SSD1306_WIDTH;

  uint8_t mask =
      (uint8_t)(1U << (y % SSD1306_PAGE_HEIGHT));

  if (enabled)
  {
    s_framebuffer[index] |= mask;
  }
  else
  {
    s_framebuffer[index] &= (uint8_t)~mask;
  }

  return ESP_OK;
}

esp_err_t display_draw_char(
    uint8_t x,
    uint8_t y,
    char character,
    uint8_t scale)
{
  if (!s_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  if (scale == 0U)
  {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t code = (uint8_t)character;

  if (code < FONT5X7_FIRST_CHARACTER ||
      code > FONT5X7_LAST_CHARACTER)
  {
    return ESP_ERR_INVALID_ARG;
  }

  uint16_t glyph_width =
      FONT5X7_GLYPH_WIDTH * scale;

  uint16_t glyph_height =
      FONT5X7_GLYPH_HEIGHT * scale;

  if ((uint16_t)x + glyph_width > SSD1306_WIDTH ||
      (uint16_t)y + glyph_height > SSD1306_HEIGHT)
  {
    return ESP_ERR_INVALID_ARG;
  }

  const uint8_t *glyph =
      FONT5X7[code - FONT5X7_FIRST_CHARACTER];

  for (uint8_t column = 0;
       column < FONT5X7_GLYPH_WIDTH;
       ++column)
  {
    for (uint8_t row = 0;
         row < FONT5X7_GLYPH_HEIGHT;
         ++row)
    {
      bool enabled =
          (glyph[column] & (1U << row)) != 0;

      for (uint8_t dx = 0; dx < scale; ++dx)
      {
        for (uint8_t dy = 0; dy < scale; ++dy)
        {
          uint8_t pixel_x =
              (uint8_t)(x + column * scale + dx);

          uint8_t pixel_y =
              (uint8_t)(y + row * scale + dy);

          esp_err_t err =
              display_draw_pixel(
                  pixel_x,
                  pixel_y,
                  enabled);

          if (err != ESP_OK)
          {
            return err;
          }
        }
      }
    }
  }

  return ESP_OK;
}

static size_t display_get_text_width(
    const char *text,
    uint8_t scale)
{
  size_t length = strlen(text);

  if (length == 0U)
  {
    return 0U;
  }

  return ((length * FONT5X7_ADVANCE) - 1U) * scale;
}

esp_err_t display_draw_text(
    uint8_t x,
    uint8_t y,
    const char *text,
    uint8_t scale)
{
  if (text == NULL || scale == 0U)
  {
    return ESP_ERR_INVALID_ARG;
  }

  size_t length = strlen(text);

  if (length == 0U)
  {
    return ESP_OK;
  }

  size_t text_width = display_get_text_width(text, scale);

  if ((size_t)x + text_width > SSD1306_WIDTH ||
      (size_t)y +
              FONT5X7_GLYPH_HEIGHT * scale >
          SSD1306_HEIGHT)
  {
    return ESP_ERR_INVALID_ARG;
  }

  for (size_t i = 0; i < length; ++i)
  {
    uint8_t character_x =
        (uint8_t)(x +
                  i * FONT5X7_ADVANCE * scale);

    esp_err_t err =
        display_draw_char(
            character_x,
            y,
            text[i],
            scale);

    if (err != ESP_OK)
    {
      return err;
    }
  }

  return ESP_OK;
}

esp_err_t display_draw_text_centered(
    uint8_t y,
    const char *text,
    uint8_t scale)
{
  if (text == NULL || scale == 0U)
  {
    return ESP_ERR_INVALID_ARG;
  }

  size_t text_width =
      display_get_text_width(text, scale);

  if (text_width > SSD1306_WIDTH)
  {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t x =
      (uint8_t)((SSD1306_WIDTH - text_width) / 2U);

  return display_draw_text(
      x,
      y,
      text,
      scale);
}

esp_err_t display_draw_horizontal_line(uint8_t y)
{
  if (!s_initialized)
  {
    return ESP_ERR_INVALID_STATE;
  }

  if (y >= SSD1306_HEIGHT)
  {
    return ESP_ERR_INVALID_ARG;
  }

  for (uint8_t x = 0; x < SSD1306_WIDTH; ++x)
  {
    esp_err_t err = display_draw_pixel(x, y, true);

    if (err != ESP_OK)
    {
      return err;
    }
  }

  return ESP_OK;
}