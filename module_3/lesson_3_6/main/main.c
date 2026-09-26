#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "3.6-pcnt";

#define GPIO_A GPIO_NUM_6
#define GPIO_B GPIO_NUM_7
#define PPR 20
#define DECODE_X 4
#define STEPS_PER_REV (PPR * DECODE_X)
#define SAMPLE_MS 200
#define GLITCH_NS 10000

static pcnt_unit_handle_t s_pcnt;

static void pcnt_encoder_init(void)
{
    pcnt_unit_config_t unit_cfg = {
        .high_limit = 32767,
        .low_limit = -32768,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_cfg, &s_pcnt));

    pcnt_chan_config_t ch_a_cfg = {
        .edge_gpio_num = GPIO_A,
        .level_gpio_num = GPIO_B,
    };
    pcnt_channel_handle_t ch_a;
    ESP_ERROR_CHECK(pcnt_new_channel(s_pcnt, &ch_a_cfg, &ch_a));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        ch_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        ch_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    pcnt_chan_config_t ch_b_cfg = {
        .edge_gpio_num = GPIO_B,
        .level_gpio_num = GPIO_A,
    };
    pcnt_channel_handle_t ch_b;
    ESP_ERROR_CHECK(pcnt_new_channel(s_pcnt, &ch_b_cfg, &ch_b));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        ch_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        ch_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_unit_enable(s_pcnt));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(s_pcnt));
    ESP_ERROR_CHECK(pcnt_unit_start(s_pcnt));
}

void app_main(void)
{
    pcnt_encoder_init();
    ESP_LOGI(TAG, "PCNT X4 CLK=GPIO%d DT=GPIO%d PPR=%d", (int)GPIO_A, (int)GPIO_B, PPR);

    int pos = 0;
    for (;;)
    {
        int d = 0;
        ESP_ERROR_CHECK(pcnt_unit_get_count(s_pcnt, &d));
        ESP_ERROR_CHECK(pcnt_unit_clear_count(s_pcnt));
        pos += d;
        
        const char *dir = (d > 0) ? "+" : (d < 0) ? "-"
                                                  : "0";
        const int rpm = (d * 60 * 1000) / (STEPS_PER_REV * SAMPLE_MS);
        ESP_LOGI(TAG, "pos=%d d=%+d dir=%s rpm=%d", pos, d, dir, rpm);

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_MS));
    }
}