# EnviroLogger

EnviroLogger is an ESP32-S3 environmental monitor built with ESP-IDF. It reads
two sensors, displays live values on a 128x64 SSD1306 OLED, and records the
current run to a CSV file on a microSD card.

## Features

- BME280 temperature, relative humidity, and atmospheric pressure over I2C
- SCD41 CO2, temperature, and relative humidity over I2C
- SCD41 pressure compensation using the BME280 pressure measurement
- Four OLED pages selected with a push button: temperature, humidity, CO2, and
  pressure
- FAT-formatted microSD logging over the SPI bus shared with the OLED
- CRC validation for SCD41 responses and calibration compensation for BME280
- Continued sensor and display operation if SD-card initialization fails