#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

typedef struct {
    gpio_num_t gpio;
    uint32_t toggle_interval_ms;
    uint64_t last_toggle_ms;
    bool state;
} blinking_led_t;

static blinking_led_t s_leds[] = {
    {.gpio = GPIO_NUM_4, .toggle_interval_ms = 200},
    {.gpio = GPIO_NUM_6, .toggle_interval_ms = 500},
    {.gpio = GPIO_NUM_16, .toggle_interval_ms = 1000},
};

static uint64_t millis(void)
{
    return (uint64_t)esp_timer_get_time() / 1000;
}

static void update_led(blinking_led_t *led, uint64_t now_ms)
{
    if (now_ms - led->last_toggle_ms < led->toggle_interval_ms) {
        return;
    }

    led->state = !led->state;
    gpio_set_level(led->gpio, led->state);
    led->last_toggle_ms = now_ms;
}

static void update_leds(void)
{
    const uint64_t now_ms = millis();

    for (size_t i = 0; i < ARRAY_SIZE(s_leds); i++) {
        update_led(&s_leds[i], now_ms);
    }
}

static void configure_leds(void)
{
    const uint64_t now_ms = millis();

    for (size_t i = 0; i < ARRAY_SIZE(s_leds); i++) {
        gpio_reset_pin(s_leds[i].gpio);
        gpio_set_direction(s_leds[i].gpio, GPIO_MODE_OUTPUT);
        gpio_set_level(s_leds[i].gpio, s_leds[i].state);
        s_leds[i].last_toggle_ms = now_ms;
    }
}

static void loop(void)
{
    while (true) {
        update_leds();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    configure_leds();
    loop();
}
