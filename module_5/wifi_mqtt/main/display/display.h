#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "driver/spi_master.h"
#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// The OLED fits 21 characters at scale 1, plus a null terminator in the buffer.
#define DISPLAY_LINE_SIZE 22

esp_err_t display_init(spi_host_device_t spi_host);

esp_err_t display_clear_buffer(void);
esp_err_t display_clear(void);
esp_err_t display_flush(void);

esp_err_t display_draw_text(uint8_t x, uint8_t y, const char *text, uint8_t scale);
esp_err_t display_show_mqtt_data(const char *topic, const char *readings);

#endif // DISPLAY_H_
