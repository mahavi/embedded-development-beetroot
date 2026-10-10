#ifndef CLOCK_H_
#define CLOCK_H_

#include "configuration.h"

#include <stdint.h>
#include "esp_err.h"

typedef struct
{
  uint16_t year;    // 2000–2099
  uint8_t month;    // 1–12
  uint8_t day;      // 1–31
  uint8_t weekday;  // 1–7
  uint8_t hour;     // 0–23
  uint8_t minute;   // 0–59
  uint8_t second;   // 0–59
} clock_datetime_t;

void clock_init(i2c_master_bus_handle_t bus_handle);
esp_err_t clock_read_datetime(clock_datetime_t *out);

#endif // CLOCK_H_
