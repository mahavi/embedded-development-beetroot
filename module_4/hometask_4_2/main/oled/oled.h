#ifndef OLED_H_
#define OLED_H_

#include "configuration.h"

void oled_init(i2c_master_bus_handle_t bus_handle);
void oled_test(void);

#endif // OLED_H_