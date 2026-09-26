#include "encoder.h"

#include "driver/pulse_cnt.h"

#define ENCODER_COUNTS_PER_STEP 4

static pcnt_unit_handle_t s_pcnt;
static int s_previous_position = 0;

void encoder_pcnt_init_x4(gpio_num_t gpio_a, gpio_num_t gpio_b)
{
    pcnt_unit_config_t unit_cfg = {
        .high_limit = 32767,
        .low_limit = -32768,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_cfg, &s_pcnt));

    pcnt_chan_config_t ch_a_cfg = {
        .edge_gpio_num = gpio_a,
        .level_gpio_num = gpio_b,
    };
    pcnt_channel_handle_t ch_a;
    ESP_ERROR_CHECK(pcnt_new_channel(s_pcnt, &ch_a_cfg, &ch_a));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        ch_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        ch_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    pcnt_chan_config_t ch_b_cfg = {
        .edge_gpio_num = gpio_b,
        .level_gpio_num = gpio_a,
    };
    pcnt_channel_handle_t ch_b;
    ESP_ERROR_CHECK(pcnt_new_channel(s_pcnt, &ch_b_cfg, &ch_b));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        ch_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        ch_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(s_pcnt, &filter_config));

    ESP_ERROR_CHECK(pcnt_unit_enable(s_pcnt));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(s_pcnt));
    ESP_ERROR_CHECK(pcnt_unit_start(s_pcnt));
}

static int encoder_get_position(void)
{
    int raw_count = 0;

    ESP_ERROR_CHECK(pcnt_unit_get_count(s_pcnt, &raw_count));

    return raw_count / ENCODER_COUNTS_PER_STEP;
}

int encoder_get_delta(void)
{
    int position = encoder_get_position();
    int delta = position - s_previous_position;

    s_previous_position = position;

    return delta;
}