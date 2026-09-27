#include "configuration.h"
#include "pwm_hw.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "3.5";
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

static void process_adc(void)
{
  int adc_raw = adc_read_raw();
  uint32_t pot_angle = adc_raw * POT_MAX_ANGLE_DEG / ADC_MAX_VALUE;

  uint32_t servo_angle = pot_angle;
  if (servo_angle > SERVO_MAX_ANGLE_DEG)
  {
    servo_angle = SERVO_MAX_ANGLE_DEG;
  }

  uint32_t pulse_us = US_AT_0 + (servo_angle * (US_AT_180 - US_AT_0)) / SERVO_MAX_ANGLE_DEG;
  pwm_hw_set_pulse_us(pulse_us);

  ESP_LOGI(
        TAG,
        "angle: %u deg, adc: %u, pulse: %u us",
        (unsigned)servo_angle,
        (unsigned)adc_raw,
        (unsigned)pulse_us);
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
  pwm_hw_init(SERVO_GPIO);
}

void app_main(void)
{
  setup();
  loop();
}
