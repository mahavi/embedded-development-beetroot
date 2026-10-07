#ifndef APP_H_
#define APP_H_

#include "stm32f4xx_hal.h"

void app_init(UART_HandleTypeDef *uart);
void app_process(void);

#endif // APP_H_
