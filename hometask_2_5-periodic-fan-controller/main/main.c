#include <stdio.h>

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_GPIO GPIO_NUM_4
#define RELAY_GPIO GPIO_NUM_5
#define TIMER_RESOLUTION_HZ 1000000u       /* 1 tick = 1 us */
#define FAN_PERIOD_US (5ULL * 1000000ULL)  // 5s
#define FAN_ON_TIME_US (2ULL * 1000000ULL) // 2s

static const char *TAG = "2.5-tmr";
static gptimer_handle_t fan_period_timer; // periodically starts the fan
static gptimer_handle_t fan_off_timer;    // stops the fan after its run time

typedef enum
{
  On,
  Off
} FanState;

static volatile FanState fan_state = Off;

static void setFanState(FanState state)
{
  uint32_t level = state == On ? 1 : 0;
  gpio_set_level(LED_GPIO, level);
  gpio_set_level(RELAY_GPIO, level);
  fan_state = state;
}

static bool IRAM_ATTR on_fan_period_alarm(gptimer_handle_t timer,
                                          const gptimer_alarm_event_data_t *edata,
                                          void *user_ctx)
{
  (void)timer;
  (void)edata;
  (void)user_ctx;

  if (fan_state == On)
  {
    return false;
  }

  setFanState(On);

  gptimer_stop(fan_off_timer);
  gptimer_set_raw_count(fan_off_timer, 0);
  gptimer_start(fan_off_timer);

  return false;
}

static bool IRAM_ATTR on_fan_off_alarm(gptimer_handle_t timer,
                                       const gptimer_alarm_event_data_t *edata,
                                       void *user_ctx)
{
  (void)timer;
  (void)edata;
  (void)user_ctx;

  setFanState(Off);
  gptimer_stop(timer);

  return false;
}

static void setup_fan_period_timer()
{
  gptimer_config_t config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = TIMER_RESOLUTION_HZ,
  };
  ESP_ERROR_CHECK(gptimer_new_timer(&config, &fan_period_timer));

  gptimer_event_callbacks_t cbs = {
      .on_alarm = on_fan_period_alarm,
  };
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(fan_period_timer, &cbs, NULL));

  gptimer_alarm_config_t alarm = {
      .reload_count = 0,
      .alarm_count = FAN_PERIOD_US,
      .flags.auto_reload_on_alarm = true,
  };
  ESP_ERROR_CHECK(gptimer_set_alarm_action(fan_period_timer, &alarm));
  ESP_ERROR_CHECK(gptimer_enable(fan_period_timer));
  ESP_ERROR_CHECK(gptimer_start(fan_period_timer));
}

static void setup_fan_off_timer()
{
  gptimer_config_t config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = TIMER_RESOLUTION_HZ,
  };
  ESP_ERROR_CHECK(gptimer_new_timer(&config, &fan_off_timer));

  gptimer_event_callbacks_t cbs = {
      .on_alarm = on_fan_on_time_alarm,
  };
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(fan_off_timer, &cbs, NULL));

  gptimer_alarm_config_t alarm = {
      .reload_count = 0,
      .alarm_count = FAN_ON_TIME_US,
      .flags.auto_reload_on_alarm = false,
  };
  ESP_ERROR_CHECK(gptimer_set_alarm_action(fan_off_timer, &alarm));
  ESP_ERROR_CHECK(gptimer_enable(fan_off_timer));
}

static void setup_relay(void)
{
  ESP_ERROR_CHECK(gpio_reset_pin(RELAY_GPIO));
  ESP_ERROR_CHECK(gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT));
}

static void setup_led(void)
{
  gpio_reset_pin(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_GPIO, 0);
}

static void setup(void)
{
  setup_led();
  setup_relay();
  setup_fan_period_timer();
  setup_fan_off_timer();
}

static void loop(void)
{
  FanState last_logged_state = fan_state;
  ESP_LOGI(TAG, "Fan state: %s", last_logged_state == On ? "ON" : "OFF");

  while (1)
  {
    FanState current_state = fan_state;
    if (current_state != last_logged_state)
    {
      ESP_LOGI(TAG, "Fan state: %s", current_state == On ? "ON" : "OFF");
      last_logged_state = current_state;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void app_main(void)
{
  setup();
  loop();
}
