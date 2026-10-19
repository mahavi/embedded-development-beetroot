#ifndef UI_H
#define UI_H

#include "environment/environment.h"
#include "driver/spi_master.h"
#include "esp_err.h"

#include <stdbool.h>

esp_err_t ui_init(spi_host_device_t spi_host);
bool ui_process_button(void);
esp_err_t ui_render(const environment_data_t *data);

#endif // UI_H