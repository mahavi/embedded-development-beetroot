#include <stdio.h>

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TASK_1 1
#define TASK_2 2
#define TASK_3 3
#define TASK_4 4

/* Change this one value to select the implementation to build. */
#ifndef ACTIVE_TASK
#define ACTIVE_TASK TASK_2
#endif

#define RC_FILTER_ENABLED 0
#define TOGGLE_INTERVAL_MS_TASK_2 50
#define BTN_GPIO GPIO_NUM_4
#define BUTTON_POLL_MS         10
#define BUTTON_DEBOUNCE_SAMPLES 3  // Approximately 30 ms

static const char *TAG = "2.4-irq";

typedef enum {
  BUTTON_RELEASED,
  BUTTON_DEBOUNCE_PRESS,
  BUTTON_PRESSED,
  BUTTON_DEBOUNCE_RELEASE,
} button_state_t;

static button_state_t button_state = BUTTON_RELEASED;
static uint8_t debounce_samples = 0;

static volatile bool btn_irq_pending = false;
static volatile uint32_t button_press_counter = 0;
static volatile uint64_t last_toggle_ms = 0;

static uint64_t millis(void)
{
    return (uint64_t)esp_timer_get_time() / 1000;
}

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
  (void)arg;
  btn_irq_pending = true;
  #if ACTIVE_TASK == TASK_1
  button_press_counter++;
  #endif
}

static void setup_button_irq(void)
{
  #if RC_FILTER_ENABLED
  gpio_pullup_t pull_up_en = GPIO_PULLUP_DISABLE;
  #else
  gpio_pullup_t pull_up_en = GPIO_PULLUP_ENABLE;
  #endif

  gpio_config_t io = {
    .pin_bit_mask = 1ULL << BTN_GPIO,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = pull_up_en,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_NEGEDGE,
  };
  ESP_ERROR_CHECK(gpio_config(&io));

  #if ACTIVE_TASK == TASK_1 || ACTIVE_TASK == TASK_2 || ACTIVE_TASK == TASK_3
  ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
  ESP_ERROR_CHECK(gpio_isr_handler_add(BTN_GPIO, gpio_isr_handler, NULL));
  #endif
}

#if ACTIVE_TASK == TASK_1 || ACTIVE_TASK == TASK_2
static void handle_button_if_needed(void)
{
  if (!btn_irq_pending) {
    return;
  }
  btn_irq_pending = false;

  #if ACTIVE_TASK == TASK_2
  uint64_t now = millis();
  if (now - last_toggle_ms < TOGGLE_INTERVAL_MS_TASK_2)
  {
    return;
  }
  else
  {
    button_press_counter++;
    last_toggle_ms = now;
  }
  #endif

  ESP_LOGI(TAG, "counter %d", button_press_counter);
}
#endif

#if ACTIVE_TASK == TASK_3
static void handle_button_if_needed(void)
{
  if (!btn_irq_pending) {
    return;
  }
  btn_irq_pending = false;

  if (gpio_get_level(BTN_GPIO) != 0) {
    return;
  }
  
  button_press_counter++;
  ESP_LOGI(TAG, "counter %d", button_press_counter);
}
#endif

#if ACTIVE_TASK == TASK_4
static void handle_button_if_needed(void)
{
  const bool pressed = gpio_get_level(BTN_GPIO) == 0;

  switch (button_state) {
    case BUTTON_RELEASED:
      if (pressed) {
        debounce_samples = 0;
        button_state = BUTTON_DEBOUNCE_PRESS;
      }
      break;

    case BUTTON_DEBOUNCE_PRESS:
      if (!pressed) {
        // The low signal was contact bounce.
        debounce_samples = 0;
        button_state = BUTTON_RELEASED;
      } else if (++debounce_samples >= BUTTON_DEBOUNCE_SAMPLES) {
        debounce_samples = 0;
        button_state = BUTTON_PRESSED;

        button_press_counter++;
        ESP_LOGI(
          TAG,
          "button pressed, counter: %" PRIu32,
          button_press_counter
        );
      }
      break;

    case BUTTON_PRESSED:
      if (!pressed) {
        debounce_samples = 0;
        button_state = BUTTON_DEBOUNCE_RELEASE;
      }
      break;

    case BUTTON_DEBOUNCE_RELEASE:
      if (pressed) {
        // The high signal was contact bounce.
        debounce_samples = 0;
        button_state = BUTTON_PRESSED;
      } else if (++debounce_samples >= BUTTON_DEBOUNCE_SAMPLES) {
        debounce_samples = 0;
        button_state = BUTTON_RELEASED;

        ESP_LOGI(TAG, "button released");
      }
      break;
  }
}
#endif

void loop(void)
{
  while(1)
  {
    handle_button_if_needed();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

static void setup(void)
{
  setup_button_irq();
}

void app_main(void)
{
  setup();
  loop();
}
