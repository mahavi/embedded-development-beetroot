#include "configuration.h"
#include "bme280/bme280.h"
#include "display/display.h"
#include "wifi_sta/wifi_sta.h"

#include "cJSON.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "mqtt_client.h"

#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "wifi_mqtt";
static i2c_master_bus_handle_t s_i2c_bus;
static esp_mqtt_client_handle_t s_mqtt_client;
static atomic_bool s_mqtt_connected;
static QueueHandle_t s_display_queue;

static void receive_mqtt_data(esp_mqtt_event_handle_t event)
{
  char payload[128];
  const size_t topic_len = sizeof(MQTT_SUBSCRIBE_TOPIC) - 1;
  if (event->current_data_offset != 0 || event->data_len != event->total_data_len ||
      event->data_len <= 0 || (size_t)event->data_len >= sizeof(payload) ||
      event->topic_len != (int)topic_len ||
      memcmp(event->topic, MQTT_SUBSCRIBE_TOPIC, topic_len) != 0) {
    return;
  }

  memcpy(payload, event->data, event->data_len);
  payload[event->data_len] = '\0';
  const char *parse_end;
  cJSON *json = cJSON_ParseWithLengthOpts(payload, event->data_len + 1, &parse_end, true);
  if (json == NULL || parse_end != payload + event->data_len) {
    cJSON_Delete(json);
    return;
  }

  const cJSON *temperature = cJSON_GetObjectItemCaseSensitive(json, "temperature_c");
  const cJSON *pressure = cJSON_GetObjectItemCaseSensitive(json, "pressure_hpa");
  const cJSON *humidity = cJSON_GetObjectItemCaseSensitive(json, "humidity_pct");
  if (cJSON_IsNumber(temperature) && cJSON_IsNumber(pressure) && cJSON_IsNumber(humidity) &&
      isfinite(temperature->valuedouble) && isfinite(pressure->valuedouble) &&
      isfinite(humidity->valuedouble)) {
    char line[DISPLAY_LINE_SIZE] = {0};
    int len = snprintf(line, sizeof(line), "T%.1f  P%.0f  H%.0f%%",
                       temperature->valuedouble, pressure->valuedouble, humidity->valuedouble);
    if (len > 0 && (size_t)len < sizeof(line)) {
      xQueueOverwrite(s_display_queue, line);
    }
  }
  cJSON_Delete(json);
}

static void process_sensor_data(void)
{
  bme280_data_t data;
  esp_err_t err = bme280_read(&data);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read sensor data: %s", esp_err_to_name(err));
    return;
  }

  ESP_LOGI(TAG,
           "T: %.1f C  RH: %.1f%%  P: %.1f hPa",
           (double)data.temperature,
           (double)data.humidity,
           (double)data.pressure);

  if (!atomic_load(&s_mqtt_connected)) {
    return;
  }

  char payload[128];
  int len = snprintf(payload,
                     sizeof(payload),
                     "{\"temperature_c\":%.1f,\"humidity_pct\":%.1f,\"pressure_hpa\":%.1f}",
                     (double)data.temperature,
                     (double)data.humidity,
                     (double)data.pressure);
  if (len < 0 || (size_t)len >= sizeof(payload)) {
    ESP_LOGE(TAG, "Failed to format sensor data for MQTT");
    return;
  }

  if (esp_mqtt_client_enqueue(s_mqtt_client, MQTT_PUBLISH_TOPIC, payload, len, 0, 0, true) < 0) {
    ESP_LOGW(TAG, "Failed to queue MQTT sensor data");
  }
}

static void loop(void)
{
  TickType_t last_wake = xTaskGetTickCount();
  while (1) {
    char readings[DISPLAY_LINE_SIZE];
    if (xQueueReceive(s_display_queue, readings, 0) == pdTRUE) {
      ESP_ERROR_CHECK_WITHOUT_ABORT(display_show_mqtt_data(MQTT_SUBSCRIBE_TOPIC, readings));
    }
    process_sensor_data();
    xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PROCESS_INTERVAL_MS));
  }
}

static void i2c_bus_init(void)
{
  i2c_master_bus_config_t cfg = {
      .i2c_port = I2C_PORT,
      .sda_io_num = I2C_SDA_GPIO,
      .scl_io_num = I2C_SCL_GPIO,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &s_i2c_bus));
  ESP_LOGI(TAG, "bus SDA=GPIO%d SCL=GPIO%d %d Hz", (int)I2C_SDA_GPIO, (int)I2C_SCL_GPIO, I2C_HZ);
}

static void spi_bus_init(void)
{
  spi_bus_config_t cfg = {
      .mosi_io_num = SPI_MOSI_GPIO,
      .miso_io_num = SPI_MISO_GPIO,
      .sclk_io_num = SPI_SCLK_GPIO,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 128 * 64 / 8,
  };

  ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST_USED, &cfg, SPI_DMA_CH_AUTO));

  ESP_LOGI(TAG,
           "SPI bus MOSI=GPIO%d MISO=GPIO%d SCLK=GPIO%d %d Hz",
           (int)SPI_MOSI_GPIO,
           (int)SPI_MISO_GPIO,
           (int)SPI_SCLK_GPIO,
           SPI_HZ);
}

static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
  switch (event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "Connected to MQTT broker");
    ESP_LOGI(TAG, "Publishing sensor data to %s", MQTT_PUBLISH_TOPIC);
    if (esp_mqtt_client_subscribe(s_mqtt_client, MQTT_SUBSCRIBE_TOPIC, 0) < 0) {
      ESP_LOGW(TAG, "Failed to subscribe to %s", MQTT_SUBSCRIBE_TOPIC);
    } else {
      ESP_LOGI(TAG, "Requested subscription to %s", MQTT_SUBSCRIBE_TOPIC);
    }
    atomic_store(&s_mqtt_connected, true);
    break;
  case MQTT_EVENT_DATA:
    receive_mqtt_data(event_data);
    break;
  case MQTT_EVENT_DISCONNECTED:
    atomic_store(&s_mqtt_connected, false);
    ESP_LOGW(TAG, "Disconnected from MQTT broker");
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGE(TAG, "MQTT client error");
    break;
  default:
    break;
  }
}

static void setup_mqtt(void)
{
  const esp_mqtt_client_config_t cfg = {
      .broker.address.uri = "mqtt://broker.hivemq.com:1883",
  };

  s_mqtt_client = esp_mqtt_client_init(&cfg);
  ESP_ERROR_CHECK(s_mqtt_client != NULL ? ESP_OK : ESP_FAIL);
  ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID,
                                                mqtt_event_handler, NULL));
  ESP_ERROR_CHECK(esp_mqtt_client_start(s_mqtt_client));
}

static void setup(void)
{
  i2c_bus_init();
  ESP_ERROR_CHECK(bme280_init(s_i2c_bus));

  spi_bus_init();
  ESP_ERROR_CHECK(display_init(SPI_HOST_USED));

  ESP_ERROR_CHECK(wifi_sta_connect());

  s_display_queue = xQueueCreate(1, DISPLAY_LINE_SIZE);
  ESP_ERROR_CHECK(s_display_queue != NULL ? ESP_OK : ESP_ERR_NO_MEM);
  setup_mqtt();
}

void app_main(void)
{
  setup();
  loop();
}
