#include "servo.h"
#include "configuration.h"

#include "driver/gpio.h"

#define PWM_PERIOD_TICKS (1u << SERVO_LEDC_DUTY_RES)

static void servo_set_pulse_us(uint32_t pulse_us)
{
  const uint32_t duty = pulse_us * PWM_PERIOD_TICKS / SERVO_PERIOD_US;

  ESP_ERROR_CHECK(
      ledc_set_duty(
          SERVO_LEDC_MODE,
          SERVO_LEDC_CHANNEL,
          duty));

  ESP_ERROR_CHECK(
      ledc_update_duty(
          SERVO_LEDC_MODE,
          SERVO_LEDC_CHANNEL));
}

void servo_init(gpio_num_t gpio)
{
  ledc_timer_config_t timer = {
      .speed_mode = SERVO_LEDC_MODE,
      .duty_resolution = SERVO_LEDC_DUTY_RES,
      .timer_num = SERVO_LEDC_TIMER,
      .freq_hz = SERVO_LEDC_FREQUENCY,
      .clk_cfg = LEDC_AUTO_CLK,
  };

  ESP_ERROR_CHECK(ledc_timer_config(&timer));

  ledc_channel_config_t channel = {
      .gpio_num = gpio,
      .speed_mode = SERVO_LEDC_MODE,
      .channel = SERVO_LEDC_CHANNEL,
      .timer_sel = SERVO_LEDC_TIMER,
      .duty = 0,
      .hpoint = 0,
  };

  ESP_ERROR_CHECK(ledc_channel_config(&channel));
}

void servo_set_angle(uint32_t angle)
{
  if (angle > SERVO_MAX_ANGLE)
  {
    angle = SERVO_MAX_ANGLE;
  }

  uint32_t pulse_us =
      SERVO_MIN_PULSE_US +
      angle * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) / SERVO_MAX_ANGLE;

  servo_set_pulse_us(pulse_us);
}