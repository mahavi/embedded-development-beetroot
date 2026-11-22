#ifndef WIFI_STA_H
#define WIFI_STA_H

#include "esp_err.h"

#define WIFI_STA_CONNECTED_BIT      BIT0
#define WIFI_STA_IPV4_OBTAINED_BIT  BIT1
#define WIFI_STA_IPV6_OBTAINED_BIT  BIT2

esp_err_t wifi_sta_connect(void);
esp_err_t wifi_sta_stop(void);
esp_err_t wifi_sta_reconnect(void);

#endif // WIFI_STA_H