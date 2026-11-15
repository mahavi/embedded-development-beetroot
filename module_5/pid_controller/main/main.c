#include "configuration.h"
#include "encoder/encoder.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "esp_timer.h"

typedef struct
{
  float Kp;
  float Ki;
  float Kd;
  float dt;
  float integral;
  float previous_error;
} PID_t;

static const char *TAG = "pid_controller";
static PID_t s_pid = {
    .Kp = PID_KP,
    .Ki = PID_KI,
    .Kd = PID_KD,
    .dt = PID_INTERVAL_MS / 1000.0f,
};
static int s_position_steps = 0;
static int s_target_steps = 0;
static bool s_boot_was_pressed = false;
static int64_t s_last_boot_press_us = 0;

static float steps_to_degrees(int steps)
{
  return steps * (360.0f / ENC_STEPS_PER_REVOLUTION);
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

static void setup_boot_button(void)
{
  gpio_config_t io = {
      .pin_bit_mask = 1ULL << BOOT_GPIO,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&io));
}

static float pid_compute(PID_t *pid, float setpoint, float position)
{
  float error = setpoint - position;

  // We can only rotate forward. At or beyond the target,
  // stop the motor and clear accumulated integral error.
  if (error <= 0.0f)
  {
    pid->integral = 0.0f;
    pid->previous_error = error;
    return 0.0f;
  }

  float derivative = (error - pid->previous_error) / pid->dt;
  float next_integral = pid->integral + error * pid->dt;
  float output = pid->Kp * error +
                 pid->Ki * next_integral +
                 pid->Kd * derivative;

  // Do not grow the integral while the output is saturated.
  if (output > 0.0f && output < (float)MOTO_PWM_MAX_DUTY)
  {
    pid->integral = next_integral;
  }

  output = pid->Kp * error +
           pid->Ki * pid->integral +
           pid->Kd * derivative;
  pid->previous_error = error;

  if (output < 0.0f)
  {
    return 0.0f;
  }
  if (output > (float)MOTO_PWM_MAX_DUTY)
  {
    return (float)MOTO_PWM_MAX_DUTY;
  }

  return output;
}

static void process_encoder(void)
{
  int delta = encoder_get_delta();

  if (delta != 0)
  {
    s_position_steps += delta;

    ESP_LOGI(TAG, "delta=%d, position=%d", delta, s_position_steps);
  }
}

static void process_boot_button(void)
{
  bool is_pressed = gpio_get_level(BOOT_GPIO) == 0;

  if (is_pressed && !s_boot_was_pressed)
  {
    int64_t now = esp_timer_get_time();

    if ((now - s_last_boot_press_us) >= BUTTON_DEBOUNCE_US)
    {
      s_target_steps += TARGET_180_DEG_STEPS;
      s_last_boot_press_us = now;

      ESP_LOGI(TAG, "BOOT pressed: target=%d steps (%d degrees)",
               s_target_steps,
               s_target_steps * 360 / ENC_STEPS_PER_REVOLUTION);
    }
  }

  s_boot_was_pressed = is_pressed;
}

static void process_pid(void)
{
  float target_deg = steps_to_degrees(s_target_steps);
  float position_deg = steps_to_degrees(s_position_steps);
  float output = pid_compute(&s_pid, target_deg, position_deg);
  uint32_t duty = (uint32_t)output;

  ESP_ERROR_CHECK(ledc_set_duty(
      MOTO_LEDC_MODE, MOTO_LEDC_CHANNEL, duty));
  ESP_ERROR_CHECK(ledc_update_duty(
      MOTO_LEDC_MODE, MOTO_LEDC_CHANNEL));

  static unsigned log_counter = 0;
  if (++log_counter >= 10U)
  {
    log_counter = 0;
    ESP_LOGI(TAG, "target=%.0f deg, position=%.0f deg, error=%.0f deg, duty=%lu",
             target_deg,
             position_deg,
             target_deg - position_deg,
             (unsigned long)duty);
  }
}

static void loop(void)
{
  TickType_t last_wake_time = xTaskGetTickCount();

  while (1)
  {
    process_encoder();
    process_boot_button();
    process_pid();

    vTaskDelayUntil(
        &last_wake_time,
        pdMS_TO_TICKS(PID_INTERVAL_MS));
  }
}

static void setup(void)
{
  setup_moto_pwm();
  setup_boot_button();
  encoder_pcnt_init_x4(ENC_GPIO_A, ENC_GPIO_B);
}

void app_main(void)
{
  setup();
  loop();
}
