#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "driver/gpio.h"
#include "driver/ledc.h"

#define ENC_GPIO_A GPIO_NUM_6
#define ENC_GPIO_B GPIO_NUM_7
#define ENC_STEPS_PER_REVOLUTION 20

#define BOOT_GPIO GPIO_NUM_0
#define TARGET_90_DEG_STEPS  (ENC_STEPS_PER_REVOLUTION / 4)
#define TARGET_180_DEG_STEPS (ENC_STEPS_PER_REVOLUTION / 2)

#define PID_KP 4.5f
#define PID_KI 2.0f
#define PID_KD 0.0f
#define PID_INTERVAL_MS 20U

#define MOTO_GPIO GPIO_NUM_18
#define MOTO_LEDC_CHANNEL LEDC_CHANNEL_0           
#define MOTO_LEDC_TIMER LEDC_TIMER_0               
#define MOTO_LEDC_MODE LEDC_LOW_SPEED_MODE         
#define MOTO_LEDC_DUTY_RES LEDC_TIMER_10_BIT       // PWM duty cycle resolution (10-bit, 0-1023)
#define MOTO_LEDC_FREQUENCY 5000                   // PWM frequency (5 kHz)
#define MOTO_PWM_MAX_DUTY    ((1UL << MOTO_LEDC_DUTY_RES) - 1UL)

#define BUTTON_DEBOUNCE_US 40000U

#endif // CONFIGURATION_H_
