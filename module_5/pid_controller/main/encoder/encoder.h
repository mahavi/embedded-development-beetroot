#ifndef ENCODER_H_
#define ENCODER_H_

#include "driver/gpio.h"

typedef enum
{
    ENCODER_DIRECTION_NONE,
    ENCODER_DIRECTION_CLOCKWISE,
    ENCODER_DIRECTION_COUNTERCLOCKWISE,
} encoder_direction_t;

void encoder_pcnt_init_x4(gpio_num_t gpio_a, gpio_num_t gpio_b);
int encoder_get_delta(void);

#endif // ENCODER_H_