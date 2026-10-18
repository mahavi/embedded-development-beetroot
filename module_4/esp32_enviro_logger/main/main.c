#include "configuration.h"
#include "sensors/bme280/bme280.h"
#include "sensors/scd41/scd41.h"

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>

static const char *TAG = "enviro_logger";

static i2c_master_bus_handle_t s_i2c_bus;

static void process_sensor_data(void)
{
  bme280_data_t bme_data;
  esp_err_t err = bme280_read(&bme_data);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read sensor data: %s", esp_err_to_name(err));
    return;
  }

  scd41_data_t scd_data;
  bool ready = false;
  scd41_is_data_ready(&ready);

  if (ready)
  {
    scd41_read(&scd_data);
  }

  ESP_LOGI(TAG,
           "T: %.1f C  RH: %.1f%%  P: %.1f hPa",
           (double)bme_data.temperature,
           (double)bme_data.humidity,
           (double)bme_data.pressure);
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
  // spi_bus_config_t cfg = {
  //     .mosi_io_num = SPI_MOSI_GPIO,
  //     .miso_io_num = SPI_MISO_GPIO,
  //     .sclk_io_num = SPI_SCLK_GPIO,
  //     .quadwp_io_num = -1,
  //     .quadhd_io_num = -1,
  // };

  // ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST_USED, &cfg, SDSPI_DEFAULT_DMA));
  // ESP_LOGI(TAG, "SPI bus init MOSI=GPIO%d MISO=GPIO%d SCLK=GPIO%d %d Hz.",
  //          (int)SPI_MOSI_GPIO, (int)SPI_MISO_GPIO, (int)SPI_SCLK_GPIO, SPI_HZ);
}

static void setup(void)
{
  i2c_bus_init();

  ESP_ERROR_CHECK(bme280_init(s_i2c_bus));

  spi_bus_init();
}

void app_main(void)
{
  setup();
  loop();
}
