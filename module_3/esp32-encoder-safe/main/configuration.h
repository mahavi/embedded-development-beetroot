#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "driver/gpio.h"
#include "driver/ledc.h"

#define BUZZ_GPIO GPIO_NUM_18
#define BUZZ_LEDC_CHANNEL LEDC_CHANNEL_0           
#define BUZZ_LEDC_TIMER LEDC_TIMER_0               
#define BUZZ_LEDC_MODE LEDC_LOW_SPEED_MODE         
#define BUZZ_LEDC_DUTY_RES LEDC_TIMER_8_BIT       // PWM duty cycle resolution (8-bit, 0-255)
#define BUZZ_LEDC_FREQUENCY 2000                  // PWM frequency (2 kHz)
#define BUZZ_LEDC_CLK LEDC_AUTO_CLK
#define BUZZ_PWM_DUTY_HALF    ((1UL << BUZZ_LEDC_DUTY_RES) / 2)

#define ENC_GPIO_A GPIO_NUM_6
#define ENC_GPIO_B GPIO_NUM_7
#define ENC_GPIO_SW GPIO_NUM_16

#define PIN_COUNT 3
#define PIN_MAX_ATTEMPTS 3

#define BUTTON_DEBOUNCE_US 40000U
#define PROCESS_INTERVAL_MS 10U

#endif // CONFIGURATION_H_
