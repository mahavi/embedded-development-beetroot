#ifndef SERVO_H_
#define SERVO_H_

#include "driver/gpio.h"
#include <stdint.h>

void servo_init(gpio_num_t gpio);
void servo_set_angle(uint32_t angle);

#endif // SERVO_H_