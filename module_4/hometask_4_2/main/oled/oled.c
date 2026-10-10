#include "oled.h"

#include "esp_log.h"

uint8_t cmd[] = {
    0x00, /* керувальний байт: усі наступні — команди */
    0xAE, /* Display OFF */
    0xD5, 0x80, /* тактова частота / подільник */
    0xA8, 0x3F, /* multiplex 64 рядки */
    0xD3, 0x00, /* зсув дисплея */
    0x40, /* стартовий рядок RAM */
    0x8D, 0x14, /* charge pump ON — обов'язково при 3,3 В */
    0xA1, /* дзеркалення сегментів */
    0xC8, /* напрям COM */
    0xDA, 0x12, /* COM pins для 128×64 */
    0x81, 0x7F, /* контраст */
    0xA6, /* нормальна полярність */
    0xAF, /* Display ON */
    0xA5, /* усі пікселі; RAM не чіпаємо */
};

static const char *TAG = "oled";
static i2c_master_dev_handle_t dev;

void oled_init(i2c_master_bus_handle_t bus_handle)
{
  i2c_device_config_t cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = ADDR_OLED,
      .scl_speed_hz = I2C_HZ,
  };
  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &cfg, &dev));
}

void oled_test(void)
{
  if (i2c_master_transmit(dev, cmd, sizeof(cmd), I2C_TIMEOUT_MS) != ESP_OK)
  {
    ESP_LOGE(TAG, "OLED 0x%02X: failed to transmit", ADDR_OLED);
    return;
  }
  ESP_LOGI(TAG, "OLED 0x%02X: Display ON, all pixels (0xA5)", ADDR_OLED);
}