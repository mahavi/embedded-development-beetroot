#include "configuration.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "3.2";
static adc_oneshot_unit_handle_t adc_handle;
static adc_channel_t adc_channel;
static adc_cali_handle_t cali_handle;

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
  int raw = adc_read_raw();
  float u_manual_mv = (float)raw * U_FS_VOLTS * 1000.0f / (float)ADC_MAX_CODE;

  if (raw == 0)
  {
    return;
  }

  int u_cali_mv = 0;
  ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &u_cali_mv));

  float error_pct =
        fabsf(u_manual_mv - (float)u_cali_mv) * 100.0f
        / (float)u_cali_mv;

  ESP_LOGI(TAG, "RAW: %d  U_manual: %.1f mV  U_cali: %d mV  Error: %.2f%%",
             raw, u_manual_mv, u_cali_mv, error_pct);
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

  adc_cali_curve_fitting_config_t cali_config = {
      .unit_id = unit,
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH};

  ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle));
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
}

void app_main(void)
{
  setup();
  loop();
}
