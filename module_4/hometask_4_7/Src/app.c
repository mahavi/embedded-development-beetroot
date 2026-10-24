#include <stdbool.h>
#include <stdio.h>

#include "app.h"
#include "configuration.h"

static volatile uint32_t app_time_ms;
static UART_HandleTypeDef *log_uart;
static uint32_t last_led_toggle_ms;
static uint32_t led_toggle_interval_ms;
static bool led_is_on;

static uint32_t random_state = 0xA5A5A5A5UL;

static bool button_raw_state;
static bool button_stable_state;
static uint32_t button_raw_change_ms;

static void log_led_interval(uint32_t interval_ms)
{
    char message[48];
    const int message_length = snprintf(
        message,
        sizeof(message),
        "LED toggle interval: %lu ms\r\n",
        (unsigned long)interval_ms);

    if ((message_length > 0) && (log_uart != NULL))
    {
        (void)HAL_UART_Transmit(
            log_uart,
            (uint8_t *)message,
            (uint16_t)message_length,
            100U);
    }
}

static void led_write(bool turn_on)
{
    if (turn_on)
    {
        APP_LED_GPIO_PORT->BSRR = APP_LED_PIN_MASK;
    }
    else
    {
        APP_LED_GPIO_PORT->BSRR = APP_LED_PIN_MASK << 16U;
    }
}

static bool button_is_pressed(void)
{
    return (APP_BUTTON_GPIO_PORT->IDR & APP_BUTTON_PIN_MASK) == 0U;
}

static uint32_t next_led_interval(uint32_t entropy)
{
    const uint32_t interval_count =
        APP_LED_MAX_INTERVAL_MS - APP_LED_MIN_INTERVAL_MS + 1U;

    /* Mix the unpredictable moment of the button press into a small PRNG. */
    random_state ^= entropy;
    random_state = random_state * 1664525UL + 1013904223UL;

    uint32_t interval_ms =
        APP_LED_MIN_INTERVAL_MS + (random_state % interval_count);

    if (interval_ms == led_toggle_interval_ms)
    {
        interval_ms = APP_LED_MIN_INTERVAL_MS +
            ((interval_ms - APP_LED_MIN_INTERVAL_MS + 1U) % interval_count);
    }

    return interval_ms;
}

static void button_process(uint32_t now_ms)
{
    const bool current_raw_state = button_is_pressed();

    if (current_raw_state != button_raw_state)
    {
        button_raw_state = current_raw_state;
        button_raw_change_ms = now_ms;
    }

    if ((button_raw_state != button_stable_state) &&
        ((uint32_t)(now_ms - button_raw_change_ms) >= APP_BUTTON_DEBOUNCE_MS))
    {
        button_stable_state = button_raw_state;

        if (button_stable_state)
        {
            led_toggle_interval_ms = next_led_interval(now_ms ^ SysTick->VAL);
            last_led_toggle_ms = now_ms;
            log_led_interval(led_toggle_interval_ms);
        }
    }
}

void app_init(UART_HandleTypeDef *uart)
{
    const uint32_t now_ms = app_time_ms;

    log_uart = uart;

    led_is_on = false;
    led_write(led_is_on);
    led_toggle_interval_ms = APP_LED_INITIAL_INTERVAL_MS;
    last_led_toggle_ms = now_ms;
    log_led_interval(led_toggle_interval_ms);

    button_raw_state = button_is_pressed();
    button_stable_state = button_raw_state;
    button_raw_change_ms = now_ms;
}

void app_process(void)
{
    const uint32_t now_ms = app_time_ms;

    button_process(now_ms);

    if ((uint32_t)(now_ms - last_led_toggle_ms) >= led_toggle_interval_ms)
    {
        last_led_toggle_ms += led_toggle_interval_ms;
        led_is_on = !led_is_on;
        led_write(led_is_on);
    }
}

void app_tick(void)
{
    ++app_time_ms;
}
