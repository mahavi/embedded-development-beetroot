#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

static const char *TAG = "6.1";

typedef struct {
    gpio_num_t gpio;
    uint32_t toggle_interval_ms;
} blinking_led_t;

static blinking_led_t s_leds[] = {
    {.gpio = GPIO_NUM_4, .toggle_interval_ms = 200},
    {.gpio = GPIO_NUM_6, .toggle_interval_ms = 500},
    {.gpio = GPIO_NUM_16, .toggle_interval_ms = 1000},
};

static void blink_led(void *parameter)
{
    const blinking_led_t *led = parameter;
    const TickType_t interval = pdMS_TO_TICKS(led->toggle_interval_ms);
    bool is_on = false;

    ESP_LOGI(TAG, "GPIO %d task started, toggles every %" PRIu32 " ms",
             (int)led->gpio, led->toggle_interval_ms);

    while (true) {
        vTaskDelay(interval);
        is_on = !is_on;
        gpio_set_level(led->gpio, is_on);
        ESP_LOGI(TAG, "GPIO %d is %s", (int)led->gpio, is_on ? "HIGH" : "LOW");
    }
}

static void configure_leds(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(s_leds); i++) {
        blinking_led_t *led = &s_leds[i];
        ESP_ERROR_CHECK(gpio_reset_pin(led->gpio));
        ESP_ERROR_CHECK(gpio_set_level(led->gpio, 0));
        ESP_ERROR_CHECK(gpio_set_direction(led->gpio, GPIO_MODE_OUTPUT));

        BaseType_t result = xTaskCreate(blink_led, "blink_led", 2048, led, 1, NULL);
        ESP_ERROR_CHECK(result == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
    }
}

void app_main(void)
{
    configure_leds();
}
