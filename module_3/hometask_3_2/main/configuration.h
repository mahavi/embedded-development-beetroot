#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12
#define ADC_GPIO GPIO_NUM_6
#define ADC_MAX_CODE 4095
#define U_FS_VOLTS 3.3f

#define PROCESS_INTERVAL_MS 100U

#endif // CONFIGURATION_H_
