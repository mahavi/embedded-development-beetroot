#ifndef PWMHW_H_
#define PWMHW_H_

#include <stdint.h>

void pwm_hw_init(int gpio);
void pwm_hw_set_pulse_us(uint32_t pulse_us);

#endif // PWMHW_H_