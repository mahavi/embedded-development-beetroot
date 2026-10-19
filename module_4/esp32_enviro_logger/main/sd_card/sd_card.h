#ifndef SD_CARD_H_
#define SD_CARD_H_

#include "esp_err.h"

esp_err_t sd_card_init(void);
esp_err_t sd_card_append_line(const char *line);
esp_err_t sd_card_start_session(void);

#endif