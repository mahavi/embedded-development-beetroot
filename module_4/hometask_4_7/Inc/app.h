#ifndef APP_H_
#define APP_H_

#include "stm32f4xx_hal.h"

void app_init(UART_HandleTypeDef *uart);
void app_process(void);
void app_tick(void);

#endif /* APP_H_ */
