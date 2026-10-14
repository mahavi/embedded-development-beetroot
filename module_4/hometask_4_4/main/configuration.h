#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"

#define I2C_PORT I2C_NUM_0
#define I2C_SDA_GPIO GPIO_NUM_9
#define I2C_SCL_GPIO GPIO_NUM_8
#define I2C_HZ 100000 
#define I2C_TIMEOUT_MS 200

#define ADDR_BME280 0x76

#define SPI_HOST_USED SPI2_HOST
#define SPI_MOSI_GPIO  GPIO_NUM_5
#define SPI_MISO_GPIO  GPIO_NUM_4
#define SPI_SCLK_GPIO  GPIO_NUM_6
#define SPI_HZ 1000000

#define SD_CS_GPIO    GPIO_NUM_7
#define SD_SPI_MODE 0

#define PROCESS_INTERVAL_MS 1000U

#endif // CONFIGURATION_H_
