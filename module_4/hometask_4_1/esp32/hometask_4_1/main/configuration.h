#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "driver/gpio.h"
#include "driver/uart.h"

#define LINK_UART UART_NUM_2
#define LINK_TX_GPIO 17
#define LINK_RX_GPIO 16
#define LINK_BAUD 115200

#define CMD_TOGGLE_LED "Toggle LED"
#define CMD_MAX            96

#define BUTTON_GPIO GPIO_NUM_4
#define LED_GPIO GPIO_NUM_6

#define BUTTON_DEBOUNCE_US 40000U
#define PROCESS_INTERVAL_MS 10U

#endif // CONFIGURATION_H_
