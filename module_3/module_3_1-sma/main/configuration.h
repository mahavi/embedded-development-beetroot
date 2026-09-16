#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define SMA_SIZE 5
#define DARK_THRESHOLD   1500
#define LIGHT_THRESHOLD  2500

#define LDR_GPIO GPIO_NUM_5
#define LDR_ADC_UNIT ADC_UNIT_1
#define LDR_ADC_CHANNEL ADC_CHANNEL_4
#define LED_GPIO GPIO_NUM_16

#define PROCESS_INTERVAL_MS 200U

#endif // CONFIGURATION_H_
