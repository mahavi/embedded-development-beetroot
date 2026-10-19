#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "driver/spi_master.h"
#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

esp_err_t display_init(spi_host_device_t spi_host);

esp_err_t display_clear_buffer(void);
esp_err_t display_clear(void);
esp_err_t display_flush(void);

esp_err_t display_draw_pixel(
    uint8_t x,
    uint8_t y,
    bool enabled);

esp_err_t display_draw_char(
    uint8_t x,
    uint8_t y,
    char character,
    uint8_t scale);

esp_err_t display_draw_text(
    uint8_t x,
    uint8_t y,
    const char *text,
    uint8_t scale);

esp_err_t display_draw_text_centered(
    uint8_t y,
    const char *text,
    uint8_t scale);

esp_err_t display_draw_horizontal_line(uint8_t y);

#endif // DISPLAY_H_