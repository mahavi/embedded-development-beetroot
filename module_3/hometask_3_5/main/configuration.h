#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"

#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12
#define ADC_GPIO GPIO_NUM_5
#define ADC_MAX_VALUE  4095UL

#define SERVO_GPIO GPIO_NUM_18
#define US_AT_0 500
#define US_AT_180 2400
#define POT_MAX_ANGLE_DEG   300U
#define SERVO_MAX_ANGLE_DEG 180U

#define PROCESS_INTERVAL_MS 20U

#endif // CONFIGURATION_H_