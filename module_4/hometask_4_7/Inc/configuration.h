#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "stm32f4xx.h"

#define APP_LED_GPIO_PORT             GPIOB
#define APP_LED_PIN_NUMBER            6U
#define APP_LED_PIN_MASK              (1UL << APP_LED_PIN_NUMBER)

#define APP_BUTTON_GPIO_PORT          GPIOB
#define APP_BUTTON_PIN_NUMBER         4U
#define APP_BUTTON_PIN_MASK           (1UL << APP_BUTTON_PIN_NUMBER)

#define APP_LED_INITIAL_INTERVAL_MS   500U
#define APP_LED_MIN_INTERVAL_MS       100U
#define APP_LED_MAX_INTERVAL_MS       1000U

#define APP_BUTTON_DEBOUNCE_MS        30U

#endif /* CONFIGURATION_H_ */
