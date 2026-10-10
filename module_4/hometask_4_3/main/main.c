#include "configuration.h"
#include "oled/oled.h"
#include "clock/clock.h"

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *TAG = "4.3";

static i2c_master_bus_handle_t s_bus;

static void display_datetime(void)
{
  clock_datetime_t now;
  esp_err_t err = clock_read_datetime(&now);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read datetime: %s", esp_err_to_name(err));
    return;
  }

  /* RTC: 1 = Sunday. */
  static const char *const weekdays[] = {
      "???", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
  };
  const char *weekday = now.weekday >= 1 && now.weekday <= 7
                            ? weekdays[now.weekday]
                            : weekdays[0];
  char time_text[16];
  char date_text[24];
  snprintf(time_text, sizeof(time_text), "%02u:%02u:%02u",
           (unsigned)now.hour, (unsigned)now.minute, (unsigned)now.second);
  snprintf(date_text, sizeof(date_text), "%s %02u.%02u.%04u",
           weekday, (unsigned)now.day, (unsigned)now.month, (unsigned)now.year);

  err = oled_show_datetime(time_text, date_text);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to update display: %s", esp_err_to_name(err));
  }
}

static void loop(void)
{
  while (1)
  {
    display_datetime();
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
  clock_init(s_bus);
}

void app_main(void)
{
  setup();
  loop();
}
