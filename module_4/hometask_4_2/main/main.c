#include "configuration.h"
#include "oled/oled.h"

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "4.2";

static i2c_master_bus_handle_t s_bus;

static void loop(void)
{
  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(PROCESS_INTERVAL_MS));
  }
}

static void bus_init(void)
{
  i2c_master_bus_config_t cfg = {
      .i2c_port = I2C_PORT,
      .sda_io_num = I2C_SDA_GPIO,
      .scl_io_num = I2C_SCL_GPIO,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &s_bus));
  ESP_LOGI(TAG, "bus SDA=GPIO%d SCL=GPIO%d %d Hz",
           (int)I2C_SDA_GPIO, (int)I2C_SCL_GPIO, I2C_HZ);
}

static void setup(void)
{
  bus_init();
  oled_init(s_bus);

  oled_test();
}

void app_main(void)
{
  setup();
  loop();
}
