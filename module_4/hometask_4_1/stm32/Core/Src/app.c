#include "app.h"
#include "configuration.h"
#include "main.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static UART_HandleTypeDef *link_uart;
static uint8_t rx_byte;

/* Debounce state for the active-low button. */
static GPIO_PinState button_last_raw;
static GPIO_PinState button_stable;
static uint32_t button_changed_at;

/* Used only by the receive callback. */
static char working_cmd[CMD_MAX];
static size_t working_len = 0;
static bool discarding = false;

/* Shared with app_process(). */
static char completed_cmd[CMD_MAX];
static volatile bool command_ready = false;

static void send_toggle_command(void)
{
  static const uint8_t message[] = CMD_TOGGLE_LED "\n";

  if (HAL_UART_Transmit(link_uart, message,
                        (uint16_t)(sizeof(message) - 1U),
                        LINK_TX_TIMEOUT_MS) != HAL_OK)
  {
    Error_Handler();
  }
}

static void process_button(void)
{
  GPIO_PinState raw = HAL_GPIO_ReadPin(BUTTON_GPIO_PORT, BUTTON_GPIO_PIN);
  uint32_t now = HAL_GetTick();

  if (raw != button_last_raw)
  {
    button_last_raw = raw;
    button_changed_at = now;
  }

  if (raw == button_stable ||
      (uint32_t)(now - button_changed_at) < BUTTON_DEBOUNCE_MS)
  {
    return;
  }

  button_stable = raw;
  if (button_stable == GPIO_PIN_RESET)
  {
    send_toggle_command();
  }
}

static void receive_character(uint8_t ch)
{
  if (ch == '\n' || ch == '\r')
  {
    if (!discarding && working_len > 0 && !command_ready)
    {
      working_cmd[working_len] = '\0';
      memcpy(completed_cmd, working_cmd, working_len + 1);

      /* Finish copying before publishing the ready flag. */
      __DMB();
      command_ready = true;
    }

    working_len = 0;
    discarding = false;
    return;
  }

  if (discarding)
  {
    return;
  }

  /* Reject embedded null bytes and oversized commands. */
  if (ch == '\0' || working_len + 1 >= sizeof(working_cmd))
  {
    working_len = 0;
    discarding = true;
    return;
  }

  working_cmd[working_len++] = (char)ch;
}

void app_init(UART_HandleTypeDef *uart)
{
  link_uart = uart;

  /* Treat the initial level as the starting state, not a new press. */
  button_last_raw = HAL_GPIO_ReadPin(BUTTON_GPIO_PORT, BUTTON_GPIO_PIN);
  button_stable = button_last_raw;
  button_changed_at = HAL_GetTick();

  if (HAL_UART_Receive_IT(link_uart, &rx_byte, 1) != HAL_OK)
  {
    Error_Handler();
  }
}

void app_process(void)
{
  process_button();

  if (!command_ready)
  {
    return;
  }

  /* Read the buffer after observing the ready flag. */
  __DMB();

  if (strcmp(completed_cmd, CMD_TOGGLE_LED) == 0)
  {
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_6);
  }

  /* Finish reading before allowing another command to replace it. */
  __DMB();
  command_ready = false;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart != link_uart)
  {
    return;
  }

  receive_character(rx_byte);

  if (HAL_UART_Receive_IT(link_uart, &rx_byte, 1) != HAL_OK)
  {
    Error_Handler();
  }
}
