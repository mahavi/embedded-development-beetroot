#include "configuration.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "3.3";
static adc_oneshot_unit_handle_t adc_handle;
static adc_channel_t adc_channel;

static int adc_read_raw(void)
{
  int raw_value = 0;

  ESP_ERROR_CHECK(
      adc_oneshot_read(
          adc_handle,
          adc_channel,
          &raw_value));

  return raw_value;
}

void process_adc(void)
{
  int adc_raw = adc_read_raw();

  uint32_t led_duty = ((uint32_t)adc_raw * LED_PWM_MAX_DUTY) / ADC_MAX_VALUE;
  ESP_ERROR_CHECK(ledc_set_duty(LED_LEDC_MODE, LED_LEDC_CHANNEL, led_duty));
  ESP_ERROR_CHECK(ledc_update_duty(LED_LEDC_MODE, LED_LEDC_CHANNEL));

  uint32_t moto_duty = ((uint32_t)adc_raw * MOTO_PWM_MAX_DUTY) / ADC_MAX_VALUE;
  ESP_ERROR_CHECK(ledc_set_duty(MOTO_LEDC_MODE, MOTO_LEDC_CHANNEL, moto_duty));
  ESP_ERROR_CHECK(ledc_update_duty(MOTO_LEDC_MODE, MOTO_LEDC_CHANNEL));

  ESP_LOGI(TAG, "raw: %d led_duty: %lu motor_duty: %lu",
           adc_raw,
           led_duty,
           moto_duty);
}

static void setup_adc(void)
{
  adc_unit_t unit = 0;
  ESP_ERROR_CHECK(adc_oneshot_io_to_channel(ADC_GPIO, &unit, &adc_channel));
  adc_oneshot_unit_init_cfg_t unit_config = {
      .unit_id = unit,
  };

  ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_config, &adc_handle));
  adc_oneshot_chan_cfg_t channel_config = {
      .bitwidth = ADC_BITWIDTH,
      .atten = ADC_ATTEN,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, adc_channel, &channel_config));

  ESP_LOGI(TAG, "GPIO%d -> ADC%d_CH%d", (int)ADC_GPIO, (int)unit + 1, (int)adc_channel);
}

static void setup_led_pwm(void)
{
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LED_LEDC_MODE,
      .duty_resolution = LED_LEDC_DUTY_RES,
      .timer_num = LED_LEDC_TIMER,
      .freq_hz = LED_LEDC_FREQUENCY};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = {
      .gpio_num = LED_GPIO,
      .speed_mode = LED_LEDC_MODE, // must match the timer
      .channel = LED_LEDC_CHANNEL,
      .timer_sel = LED_LEDC_TIMER,
      .duty = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static void setup_moto_pwm(void)
{
  ledc_timer_config_t ledc_timer = {
      .speed_mode = MOTO_LEDC_MODE,
      .duty_resolution = MOTO_LEDC_DUTY_RES,
      .timer_num = MOTO_LEDC_TIMER,
      .freq_hz = MOTO_LEDC_FREQUENCY};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = {
      .gpio_num = MOTO_GPIO,
      .speed_mode = MOTO_LEDC_MODE, // must match the timer
      .channel = MOTO_LEDC_CHANNEL,
      .timer_sel = MOTO_LEDC_TIMER,
      .duty = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static void loop(void)
{
  while (1)
  {
    process_adc();
    vTaskDelay(pdMS_TO_TICKS(PROCESS_INTERVAL_MS));
  }
}

static void setup(void)
{
  setup_adc();
  setup_led_pwm();
  setup_moto_pwm();
}

void app_main(void)
{
  setup();
  loop();
}
