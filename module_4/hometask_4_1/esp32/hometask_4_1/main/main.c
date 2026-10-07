#include "button/button.h"
#include "configuration.h"

#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static bool is_on = false;
static button_t s_button;
static char cmd[CMD_MAX];
static size_t cmd_len = 0;
static bool cmd_discarding = false;

static const char *TAG = "4.1";

static void led_toggle(void)
{
  is_on = !is_on;
  gpio_set_level(LED_GPIO, is_on ? 1 : 0);

  ESP_LOGI(TAG, "LED status: %s", is_on ? "On" : "Off");
}

static void send_command(const char *cmd)
{
  uart_write_bytes(LINK_UART, cmd, strlen(cmd));
  uart_write_bytes(LINK_UART, "\n", 1);

  ESP_LOGI(TAG, "Sent command: %s", cmd);

  uart_wait_tx_done(LINK_UART, pdMS_TO_TICKS(100));
}

static int link_read_command(char *out, size_t cap)
{
  for (;;)
  {
    uint8_t ch;
    if (uart_read_bytes(LINK_UART, &ch, 1, 0) != 1)
    {
      return 0;
    }

    if (ch == '\n' || ch == '\r')
    {
      if (cmd_discarding)
      {
        cmd_discarding = false;
        cmd_len = 0;
        continue;
      }

      out[cmd_len] = '\0';
      if (cmd_len == 0)
      {
        continue;
      }
      
      int completed_len = (int)cmd_len;
      cmd_len = 0;
      return completed_len;
    }

    if (cmd_discarding)
    {
      continue;
    }

    if (cmd_len + 1 < cap)
    {
      out[cmd_len++] = (char)ch;
    }
    else
    {
      cmd_discarding = true;
      cmd_len = 0;
      ESP_LOGW(TAG, "Command too long; discarding.");
    }
  }
}

static void process_button(void)
{
  bool pressed = button_take_press(&s_button);
  if (!pressed)
  {
    return;
  }

  ESP_LOGI(TAG, "Button pressed.");
  send_command(CMD_TOGGLE_LED);
}

static void process_link(void)
{
  if (link_read_command(cmd, sizeof(cmd)) < 1)
  {
    return;
  }

  ESP_LOGI(TAG, "Received command: %s", cmd);

  if (strcmp(cmd, CMD_TOGGLE_LED) == 0)
  {
    led_toggle();
  }
}

static void loop(void)
{
  while (1)
  {
    process_button();
    process_link();
    vTaskDelay(pdMS_TO_TICKS(PROCESS_INTERVAL_MS));
  }
}

static void setup_button(void)
{
  button_service_init();
  button_init(
      &s_button,
      BUTTON_GPIO,
      BUTTON_DEBOUNCE_US,
      "button");
}

static void setup_led(void)
{
  gpio_reset_pin(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_GPIO, 0);
}

static void setup_link(void)
{
  const uart_config_t cfg = {
      .baud_rate = LINK_BAUD,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };
  ESP_ERROR_CHECK(uart_driver_install(LINK_UART, 1024, 512, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(LINK_UART, &cfg));
  ESP_ERROR_CHECK(uart_set_pin(LINK_UART, LINK_TX_GPIO, LINK_RX_GPIO,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static void setup(void)
{
  setup_button();
  setup_led();
  setup_link();
}

void app_main(void)
{
  setup();
  loop();
}
