#include "button/button.h"
#include "buzzer/buzzer.h"
#include "configuration.h"
#include "encoder/encoder.h"
#include "pin_console/pin_console.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <stdbool.h>
#include <string.h>

static const char *TAG = "encoder-safe";

typedef enum
{
  SAFE_STATE_ENTERING,
  SAFE_STATE_ACCESS_GRANTED,
  SAFE_STATE_LOCKED,
} safe_state_t;

static button_t s_reset_button;
static safe_state_t s_safe_state = SAFE_STATE_ENTERING;
static encoder_direction_t s_digit_direction = ENCODER_DIRECTION_NONE;
static uint8_t s_current_digit = 0;
static uint8_t s_digit_index = 0;
static uint8_t s_attempts_remaining = PIN_MAX_ATTEMPTS;
static uint8_t s_entered_pin[PIN_COUNT];

static const uint8_t k_correct_pin[PIN_COUNT] = {2, 2, 2};

static void reset_pin_entry(void)
{
  s_current_digit = 0;
  s_digit_index = 0;
  s_digit_direction = ENCODER_DIRECTION_NONE;
  memset(s_entered_pin, 0, sizeof(s_entered_pin));

  pin_console_begin();
}

static void register_failed_attempt(void)
{
  if (s_attempts_remaining == 0)
  {
    return;
  }

  s_attempts_remaining--;

  ESP_LOGI(TAG, "Attempts left: %d", s_attempts_remaining);

  if (s_attempts_remaining == 0)
  {
    s_safe_state = SAFE_STATE_LOCKED;
    buzzer_play_alarm();
  }
  else
  {
    reset_pin_entry();
  }
}

static bool pin_is_correct(void)
{
  for (size_t i = 0; i < PIN_COUNT; i++)
  {
    if (k_correct_pin[i] != s_entered_pin[i])
    {
      return false;
    }
  }
  return true;
}

static void handle_entered_pin(void)
{
  if (pin_is_correct())
  {
    ESP_LOGI(TAG, "PIN is correct! SUCCESS!");
    s_safe_state = SAFE_STATE_ACCESS_GRANTED;
    buzzer_play_success();
  }
  else
  {
    register_failed_attempt();
  }
}

static bool handle_encoder_step(int step)
{
  encoder_direction_t direction =
      step > 0
          ? ENCODER_DIRECTION_CLOCKWISE
          : ENCODER_DIRECTION_COUNTERCLOCKWISE;

  if (s_digit_direction == ENCODER_DIRECTION_NONE)
  {
    s_digit_direction = direction;
    s_current_digit = (s_current_digit + 1) % 10;
    pin_console_update_digit(s_current_digit);
    return false;
  }

  if (s_digit_direction == direction)
  {
    s_current_digit = (s_current_digit + 1) % 10;
    pin_console_update_digit(s_current_digit);
    return false;
  }

  if (s_digit_index >= PIN_COUNT)
  {
    return true;
  }

  s_entered_pin[s_digit_index++] = s_current_digit;

  if (s_digit_index == PIN_COUNT)
  {
    pin_console_end();
    handle_entered_pin();
    return true;
  }

  s_digit_direction = direction;
  s_current_digit = 0;

  pin_console_next_digit();

  return false;
}

static void process_reset_button(void)
{
  bool pressed = button_take_press(&s_reset_button);
  if (!pressed || s_safe_state != SAFE_STATE_ENTERING)
  {
    return;
  }

  pin_console_end();
  ESP_LOGI(TAG, "PIN entry reset");

  register_failed_attempt();
}

static void process_encoder(void)
{
  if (s_safe_state != SAFE_STATE_ENTERING)
  {
    return;
  }

  int delta = encoder_get_delta();

  while (delta != 0)
  {
    int step = delta > 0 ? 1 : -1;

    if (handle_encoder_step(step))
    {
      break;
    }
    delta -= step;
  }
}

static void loop(void)
{
  while (1)
  {
    buzzer_update();
    process_reset_button();
    process_encoder();

    vTaskDelay(pdMS_TO_TICKS(PROCESS_INTERVAL_MS));
  }
}

static void setup(void)
{
  buzzer_init();

  button_service_init();
  button_init(
      &s_reset_button,
      ENC_GPIO_SW,
      BUTTON_DEBOUNCE_US,
      "reset_button");

  encoder_pcnt_init_x4(ENC_GPIO_A, ENC_GPIO_B);

  ESP_LOGI(TAG, "PIN attempts remaining: %d", s_attempts_remaining);
  pin_console_begin();
}

void app_main(void)
{
  setup();
  loop();
}
