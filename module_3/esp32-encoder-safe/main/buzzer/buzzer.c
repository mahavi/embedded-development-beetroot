#include "buzzer.h"
#include "configuration.h"
#include "notes.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include <stdbool.h>

static const note_t *s_melody = NULL;
static size_t s_melody_length;
static uint32_t s_note_index;
static int64_t s_deadline_us = 0;
static bool s_note_active = false;

static void start_note(note_t note)
{
  if (note.hz == REST)
  {
    buzzer_stop();
  }
  else
  {
    ESP_ERROR_CHECK(
        ledc_set_freq(
            BUZZ_LEDC_MODE, BUZZ_LEDC_TIMER, note.hz));

    ESP_ERROR_CHECK(
        ledc_set_duty(
            BUZZ_LEDC_MODE, BUZZ_LEDC_CHANNEL, BUZZ_PWM_DUTY_HALF));

    ESP_ERROR_CHECK(
        ledc_update_duty(
          BUZZ_LEDC_MODE, BUZZ_LEDC_CHANNEL));
  }
  
  s_note_active = true;
  s_deadline_us =
      esp_timer_get_time() + (int64_t)note.ms * 1000;
}

static void start_melody(
    const note_t *melody,
    size_t melody_length)
{
  buzzer_stop();

  s_melody = melody;
  s_melody_length = melody_length;
  s_note_index = 0;

  if (s_melody_length > 0)
  {
    start_note(s_melody[s_note_index]);
  }
}

void buzzer_init(void)
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

void buzzer_stop(void)
{
  ESP_ERROR_CHECK(ledc_set_duty(BUZZ_LEDC_MODE, BUZZ_LEDC_CHANNEL, 0));
  ESP_ERROR_CHECK(ledc_update_duty(BUZZ_LEDC_MODE, BUZZ_LEDC_CHANNEL));
}

void buzzer_update(void)
{
  if (s_melody == NULL)
  {
    return;
  }

  int64_t now_us = esp_timer_get_time();
  if (now_us < s_deadline_us)
  {
    return;
  }

  if (s_note_active)
  {
    buzzer_stop();
    
    s_note_active = false;
    s_deadline_us = now_us + NOTE_GAP_MS * 1000;
    
    return;
  }

  s_note_index++;
  if (s_melody_length > s_note_index)
  {
    start_note(s_melody[s_note_index]);
  }
  else
  {
    s_melody = NULL;
    s_melody_length = 0;
  }
}

void buzzer_play_alarm(void)
{
  start_melody(
      k_melody_alarm,
      sizeof(k_melody_alarm) / sizeof(k_melody_alarm[0]));
}

void buzzer_play_success(void)
{
  start_melody(
      k_melody_success,
      sizeof(k_melody_success) / sizeof(k_melody_success[0]));
}