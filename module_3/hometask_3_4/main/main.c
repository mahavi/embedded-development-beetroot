#include "configuration.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "notes.h"
#include "esp_log.h"

static const char *TAG = "3.4";

static void buzz_off(void)
{
  ESP_ERROR_CHECK(ledc_set_duty(BUZZ_LEDC_MODE, BUZZ_LEDC_CHANNEL, 0));
  ESP_ERROR_CHECK(ledc_update_duty(BUZZ_LEDC_MODE, BUZZ_LEDC_CHANNEL));
}

static void play_note(uint32_t hz, int ms)
{
  if (hz <= 0)
  {
    buzz_off();
  }
  else
  {
    ESP_ERROR_CHECK(ledc_set_freq(BUZZ_LEDC_MODE, BUZZ_LEDC_TIMER, hz));
    ESP_ERROR_CHECK(ledc_set_duty(BUZZ_LEDC_MODE, BUZZ_LEDC_CHANNEL, PWM_DUTY_HALF));
    ESP_ERROR_CHECK(ledc_update_duty(BUZZ_LEDC_MODE, BUZZ_LEDC_CHANNEL));
    ESP_LOGI(TAG, "%d Hz %d ms", hz, ms);
  }
  vTaskDelay(pdMS_TO_TICKS(ms));
  buzz_off();
  vTaskDelay(pdMS_TO_TICKS(20));
}

static void play_melody(void)
{
  for (size_t i = 0; i < sizeof(k_melody) / sizeof(k_melody[0]); i++)
  {
    play_note(k_melody[i].hz, k_melody[i].ms);
  }
}

static void loop(void)
{
  while (1)
  {
    play_melody();
    vTaskDelay(pdMS_TO_TICKS(PROCESS_INTERVAL_MS));
  }
}

static void setup_buzz_pwm(void)
{
  ledc_timer_config_t ledc_timer = {
      .speed_mode = BUZZ_LEDC_MODE,
      .duty_resolution = BUZZ_LEDC_DUTY_RES,
      .timer_num = BUZZ_LEDC_TIMER,
      .freq_hz = BUZZ_LEDC_FREQUENCY,
      .clk_cfg = BUZZ_LEDC_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = {
      .gpio_num = BUZZ_GPIO,
      .speed_mode = BUZZ_LEDC_MODE,
      .channel = BUZZ_LEDC_CHANNEL,
      .timer_sel = BUZZ_LEDC_TIMER,
      .duty = 0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static void setup(void)
{
  setup_buzz_pwm();
  buzz_off();
}

void app_main(void)
{
  setup();

  ESP_LOGI(TAG, "==== Playing melody at GPIO %d ====", (int)BUZZ_GPIO);
  loop();
}
