#ifndef APP_H_
#define APP_H_

#include "stm32f4xx_hal.h"

void app_init(ADC_HandleTypeDef *hadc, UART_HandleTypeDef *huart);
void app_process(void);

#endif /* APP_H_ */
