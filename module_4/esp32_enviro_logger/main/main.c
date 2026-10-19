#include "configuration.h"
#include "environment/environment.h"
#include "sd_card/sd_card.h"
#include "ui/ui.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdbool.h>
#include <stdio.h>

static const char *TAG = "enviro_logger";
static i2c_master_bus_handle_t s_i2c_bus;
static bool s_sd_ready;
static int64_t s_session_start_us;

static void record_environment_data(void)
{
  if (!s_sd_ready)
  {
    return;
  }

  const environment_data_t *data =
      environment_get_data();

  if (!data->has_bme_sample ||
      !data->has_scd_sample)
  {
    return;
  }

  int64_t elapsed_ms =
      (esp_timer_get_time() - s_session_start_us) / 1000;

  char line[160];
  int length = snprintf(
      line,
      sizeof(line),
      "%lld,%.2f,%.2f,%.2f,%u,%.2f,%.2f\n",
      (long long)elapsed_ms,
      (double)data->bme_temperature,
      (double)data->bme_humidity,
      (double)data->pressure_hpa,
      (unsigned)data->co2_ppm,
      (double)data->scd_temperature,
      (double)data->scd_humidity);

  if (length < 0 ||
      (size_t)length >= sizeof(line))
  {
    ESP_LOGE(TAG, "Failed to format SD log line");
    return;
  }

  esp_err_t err = sd_card_append_line(line);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG,
             "SD logging disabled after write failure: %s",
             esp_err_to_name(err));
    s_sd_ready = false;
  }
}

static void loop(void)
{
  environment_update();
  record_environment_data();

  esp_err_t err = ui_render(environment_get_data());
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG,
             "Failed to render display: %s",
             esp_err_to_name(err));
  }

  int64_t last_sensor_update_us =
      esp_timer_get_time();

  while (1)
  {
    bool render_needed = ui_process_button();

    int64_t now_us = esp_timer_get_time();

    if ((now_us - last_sensor_update_us) >= PROCESS_INTERVAL_US)
    {
      last_sensor_update_us = now_us;

      environment_update();
      record_environment_data();
      render_needed = true;
    }

    if (render_needed)
    {
      err = ui_render(environment_get_data());

      if (err != ESP_OK)
      {
        ESP_LOGE(TAG,
                 "Failed to render display: %s",
                 esp_err_to_name(err));
      }
    }

    vTaskDelay(pdMS_TO_TICKS(UI_POLL_INTERVAL_MS));
  }
}

static void i2c_bus_init(void)
{
  i2c_master_bus_config_t cfg = {
      .i2c_port = I2C_PORT,
      .sda_io_num = I2C_SDA_GPIO,
      .scl_io_num = I2C_SCL_GPIO,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &s_i2c_bus));
  ESP_LOGI(TAG, "bus SDA=GPIO%d SCL=GPIO%d %d Hz",
           (int)I2C_SDA_GPIO, (int)I2C_SCL_GPIO, I2C_HZ);
}

static void spi_bus_init(void)
{
  spi_bus_config_t cfg = {
      .mosi_io_num = SPI_MOSI_GPIO,
      .miso_io_num = SPI_MISO_GPIO,
      .sclk_io_num = SPI_SCLK_GPIO,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 128 * 64 / 8,
  };

  ESP_ERROR_CHECK(
      spi_bus_initialize(
          SPI_HOST_USED,
          &cfg,
          SPI_DMA_CH_AUTO));

  ESP_LOGI(TAG,
           "SPI bus MOSI=GPIO%d MISO=GPIO%d SCLK=GPIO%d %d Hz",
           (int)SPI_MOSI_GPIO,
           (int)SPI_MISO_GPIO,
           (int)SPI_SCLK_GPIO,
           SPI_HZ);
}

static void setup(void)
{
  i2c_bus_init();
  spi_bus_init();

  /*
   * The SD card must enter SPI mode before any traffic is sent
   * to another device on the shared bus.
   */
  esp_err_t err = sd_card_init();
  if (err == ESP_OK)
  {
    err = sd_card_start_session();
  }

  if (err == ESP_OK)
  {
    s_session_start_us = esp_timer_get_time();
    s_sd_ready = true;
  }
  else
  {
    ESP_LOGW(TAG,
             "SD logging disabled: %s",
             esp_err_to_name(err));
  }

  ESP_ERROR_CHECK(ui_init(SPI_HOST_USED));
  ESP_ERROR_CHECK(environment_init(s_i2c_bus));
}

void app_main(void)
{
  setup();
  loop();
}
