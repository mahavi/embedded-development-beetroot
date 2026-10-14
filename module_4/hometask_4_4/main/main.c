#include "configuration.h"
#include "bme280/bme280.h"
#include "sd_card/sd_card.h"

#include "driver/sdspi_host.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <inttypes.h>
#include <stdio.h>

static const char *TAG = "4.4";

static i2c_master_bus_handle_t s_i2c_bus;

static void process_sensor_data(void)
{
  bme280_data_t data;
  esp_err_t err = bme280_read(&data);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read sensor data: %s", esp_err_to_name(err));
    return;
  }

  uint64_t uptime_s = (uint64_t)esp_timer_get_time() / 1000000ULL;
  char line[96];

  snprintf(line,
           sizeof(line),
           "%" PRIu64 ",%.1f,%.1f,%.1f\n",
           uptime_s,
           (double)data.temperature,
           (double)data.humidity,
           (double)data.pressure);

  err = sd_card_append_line(line);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to write to sd card: %s", esp_err_to_name(err));
  }

  ESP_LOGI(TAG,
           "Uptime: %" PRIu64 " s  T: %.1f C  RH: %.1f%%  P: %.1f hPa",
           uptime_s,
           (double)data.temperature,
           (double)data.humidity,
           (double)data.pressure);
}

static void loop(void)
{
  while (1)
  {
    process_sensor_data();
    vTaskDelay(pdMS_TO_TICKS(PROCESS_INTERVAL_MS));
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
  };

  ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST_USED, &cfg, SDSPI_DEFAULT_DMA));
  ESP_LOGI(TAG, "SPI bus init MOSI=GPIO%d MISO=GPIO%d SCLK=GPIO%d %d Hz.",
           (int)SPI_MOSI_GPIO, (int)SPI_MISO_GPIO, (int)SPI_SCLK_GPIO, SPI_HZ);
}

static void setup(void)
{
  i2c_bus_init();

  ESP_ERROR_CHECK(bme280_init(s_i2c_bus));

  spi_bus_init();
  ESP_ERROR_CHECK(sd_card_init());

  //ESP_ERROR_CHECK_WITHOUT_ABORT(sd_card_print_history());
}

void app_main(void)
{
  setup();
  loop();
}
