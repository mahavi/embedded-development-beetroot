#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"

#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12
#define ADC_GPIO GPIO_NUM_5
#define ADC_MAX_VALUE  4095UL

#define LED_GPIO GPIO_NUM_9
#define LED_LEDC_CHANNEL LEDC_CHANNEL_0           
#define LED_LEDC_TIMER LEDC_TIMER_0               
#define LED_LEDC_MODE LEDC_LOW_SPEED_MODE         
#define LED_LEDC_DUTY_RES LEDC_TIMER_10_BIT       // PWM duty cycle resolution (10-bit, 0-1023)
#define LED_LEDC_FREQUENCY 1000                   // PWM frequency (1 kHz)
#define LED_PWM_MAX_DUTY    ((1UL << LED_LEDC_DUTY_RES) - 1UL)

#define MOTO_GPIO GPIO_NUM_16
#define MOTO_LEDC_CHANNEL LEDC_CHANNEL_1           
#define MOTO_LEDC_TIMER LEDC_TIMER_1               
#define MOTO_LEDC_MODE LEDC_LOW_SPEED_MODE         
#define MOTO_LEDC_DUTY_RES LEDC_TIMER_10_BIT       // PWM duty cycle resolution (10-bit, 0-1023)
#define MOTO_LEDC_FREQUENCY 20000                   // PWM frequency (20 kHz)
#define MOTO_PWM_MAX_DUTY    ((1UL << MOTO_LEDC_DUTY_RES) - 1UL)

#define PROCESS_INTERVAL_MS 10U

#endif // CONFIGURATION_H_
