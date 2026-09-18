#include "configuration.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "3.1";
static adc_oneshot_unit_handle_t adc_handle;
static adc_channel_t adc_channel;

static bool led_on = false;
static int samples[SMA_SIZE] = {0};
static uint8_t next = 0;  // position to write the next reading
static uint8_t count = 0; // number of valid readings so far

void add_sample(int raw)
{
  samples[next] = raw;
  next = (next + 1) % SMA_SIZE;

  if (count < SMA_SIZE)
  {
    count++;
  }
}

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

int calculate_sma(void)
{
  if (count == 0)
  {
    return 0; // no readings yet
  }

  int sum = 0;
  for (int i = 0; i < count; i++)
  {
    sum += samples[i];
  }

  return sum / count;
}

void update_led(int sma)
{
  if (!led_on && sma < DARK_THRESHOLD)
  {
    led_on = true;
    gpio_set_level(LED_GPIO, 1);
    ESP_LOGI(TAG, "LED On");
  }
  else if (led_on && sma > LIGHT_THRESHOLD)
  {
    led_on = false;
    gpio_set_level(LED_GPIO, 0);
    ESP_LOGI(TAG, "LED Off");
  }
}

void process_ldr()
{
  int light_raw = adc_read_raw();

  add_sample(light_raw);
  int sma = calculate_sma();

  ESP_LOGI(TAG, "raw: %d    sma: %d", light_raw, sma);
  update_led(sma);
}

static void setup_adc(void)
{
  adc_unit_t unit = 0;
  ESP_ERROR_CHECK(adc_oneshot_io_to_channel(LDR_GPIO, &unit, &adc_channel));
  adc_oneshot_unit_init_cfg_t unit_config = {
      .unit_id = unit,
  };

  ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_config, &adc_handle));
  adc_oneshot_chan_cfg_t channel_config = {
      .bitwidth = ADC_BITWIDTH,
      .atten = ADC_ATTEN,
  };

  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, adc_channel, &channel_config));
}

static void setup_led(void)
{
  gpio_reset_pin(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_GPIO, 0);
}

void loop(void)
{
  while (1)
  {
    process_ldr();
    vTaskDelay(pdMS_TO_TICKS(PROCESS_INTERVAL_MS));
  }
}

void setup(void)
{
  setup_adc();
  setup_led();
}

void app_main(void)
{
  setup();
  loop();
}
