#include "ui.h"
#include "configuration.h"
#include "button/button.h"
#include "display/display.h"

#include "esp_log.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef enum
{
  DISPLAY_PAGE_TEMPERATURE,
  DISPLAY_PAGE_HUMIDITY,
  DISPLAY_PAGE_CO2,
  DISPLAY_PAGE_PRESSURE,
  DISPLAY_PAGE_COUNT,
} display_page_t;

static const char *TAG = "ui";
static display_page_t s_display_page = DISPLAY_PAGE_TEMPERATURE;
static button_t s_button;

esp_err_t ui_init(spi_host_device_t spi_host)
{
  button_service_init();
  button_init(
      &s_button,
      BUTTON_GPIO,
      BUTTON_DEBOUNCE_US,
      "button");

  esp_err_t err = display_init(spi_host);
  if (err != ESP_OK)
  {
    return err;
  }

  return display_clear();
}

bool ui_process_button(void)
{
  if (!button_take_press(&s_button))
  {
    return false;
  }

  uint32_t next_page =
      (uint32_t)s_display_page + 1U;

  s_display_page =
      (display_page_t)(next_page %
                       (uint32_t)DISPLAY_PAGE_COUNT);

  ESP_LOGI(TAG,
           "Display page changed to %u",
           (unsigned)s_display_page);

  return true;
}

esp_err_t ui_render(const environment_data_t *data)
{
  if (data == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  const char *title = NULL;
  char value[24];

  switch (s_display_page)
  {
  case DISPLAY_PAGE_TEMPERATURE:
    title = "TEMPERATURE C";

    if (data->has_bme_sample &&
        data->has_scd_sample)
    {
      snprintf(
          value,
          sizeof(value),
          "%.1f/%.1f",
          (double)data->bme_temperature,
          (double)data->scd_temperature);
    }
    else if (data->has_bme_sample)
    {
      snprintf(
          value,
          sizeof(value),
          "%.1f/--.-",
          (double)data->bme_temperature);
    }
    else
    {
      snprintf(value, sizeof(value), "--.-/--.-");
    }
    break;

  case DISPLAY_PAGE_HUMIDITY:
    title = "HUMIDITY %";

    if (data->has_bme_sample &&
        data->has_scd_sample)
    {
      snprintf(
          value,
          sizeof(value),
          "%.1f/%.1f",
          (double)data->bme_humidity,
          (double)data->scd_humidity);
    }
    else if (data->has_bme_sample)
    {
      snprintf(
          value,
          sizeof(value),
          "%.1f/--.-",
          (double)data->bme_humidity);
    }
    else
    {
      snprintf(value, sizeof(value), "--.-/--.-");
    }
    break;

  case DISPLAY_PAGE_CO2:
    title = "CO2 PPM";

    if (data->has_scd_sample)
    {
      snprintf(
          value,
          sizeof(value),
          "%u",
          (unsigned)data->co2_ppm);
    }
    else
    {
      snprintf(value, sizeof(value), "WAITING");
    }
    break;

  case DISPLAY_PAGE_PRESSURE:
    title = "PRESSURE HPA";

    if (data->has_bme_sample)
    {
      snprintf(
          value,
          sizeof(value),
          "%.1f",
          (double)data->pressure_hpa);
    }
    else
    {
      snprintf(value, sizeof(value), "WAITING");
    }
    break;

  default:
    return ESP_ERR_INVALID_STATE;
  }

  /*
   * Scale 2 fits strings up to 10 characters.
   * Fall back to scale 1 for unusual long values such as
   * negative temperatures on both sensors.
   */
  uint8_t value_scale =
      strlen(value) <= 10U ? 2U : 1U;

  esp_err_t err = display_clear_buffer();
  if (err != ESP_OK)
  {
    return err;
  }

  err = display_draw_text_centered(
      7,
      title,
      1);

  if (err != ESP_OK)
  {
    return err;
  }

  err = display_draw_horizontal_line(23);
  if (err != ESP_OK)
  {
    return err;
  }

  err = display_draw_text_centered(
      34,
      value,
      value_scale);

  if (err != ESP_OK)
  {
    return err;
  }

  return display_flush();
}
