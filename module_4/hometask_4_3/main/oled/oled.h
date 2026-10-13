#ifndef OLED_H_
#define OLED_H_

#include "configuration.h"
#include "esp_err.h"

void oled_init(i2c_master_bus_handle_t bus_handle);
esp_err_t oled_show_datetime(const char *time_text, const char *date_text);

#endif // OLED_H_
