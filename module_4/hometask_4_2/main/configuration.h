#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "driver/gpio.h"
#include "driver/i2c_master.h"

#define I2C_PORT I2C_NUM_0
#define I2C_SDA_GPIO GPIO_NUM_8
#define I2C_SCL_GPIO GPIO_NUM_9
#define I2C_HZ 100000 
#define I2C_TIMEOUT_MS 200

#define ADDR_OLED 0x3C

#define PROCESS_INTERVAL_MS 1000U

#endif // CONFIGURATION_H_
