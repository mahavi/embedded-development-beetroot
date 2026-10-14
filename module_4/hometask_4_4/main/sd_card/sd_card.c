#include "sd_card.h"
#include "configuration.h"

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define SD_MOUNT_POINT "/sdcard"
#define SD_HISTORY_FILE SD_MOUNT_POINT "/history.csv"
#define SD_HISTORY_HEADER "uptime_s,temperature_c,humidity_pct,pressure_hpa\n"

static const char *TAG = "sd_card";
static sdmmc_card_t *s_card;

esp_err_t sd_card_init(void)
{
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SPI_HOST_USED;
  host.max_freq_khz = SPI_HZ / 1000;

  sdspi_device_config_t slot_config =
      SDSPI_DEVICE_CONFIG_DEFAULT();

  slot_config.host_id = SPI_HOST_USED;
  slot_config.gpio_cs = SD_CS_GPIO;

  esp_vfs_fat_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 4,
      .allocation_unit_size = 16 * 1024,
  };

  esp_err_t err = esp_vfs_fat_sdspi_mount(
      SD_MOUNT_POINT,
      &host,
      &slot_config,
      &mount_config,
      &s_card);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG,
             "Failed to mount SD card: %s",
             esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG,
           "SD card mounted at %s, CS=GPIO%d",
           SD_MOUNT_POINT,
           (int)SD_CS_GPIO);

  sdmmc_card_print_info(stdout, s_card);

  return ESP_OK;
}

esp_err_t sd_card_append_line(const char *line)
{
  if (line == NULL)
  {
    return ESP_ERR_INVALID_ARG;
  }

  if (s_card == NULL)
  {
    return ESP_ERR_INVALID_STATE;
  }

  FILE *file = fopen(SD_HISTORY_FILE, "a+");
  if (file == NULL)
  {
    ESP_LOGE(TAG,
             "Failed to open %s: %s",
             SD_HISTORY_FILE,
             strerror(errno));
    return ESP_FAIL;
  }

  if (fseek(file, 0, SEEK_END) != 0)
  {
    ESP_LOGE(TAG,
             "Failed to seek %s: %s",
             SD_HISTORY_FILE,
             strerror(errno));
    fclose(file);
    return ESP_FAIL;
  }

  long file_size = ftell(file);
  if (file_size < 0)
  {
    ESP_LOGE(TAG,
             "Failed to inspect %s: %s",
             SD_HISTORY_FILE,
             strerror(errno));
    fclose(file);
    return ESP_FAIL;
  }

  if (file_size == 0)
  {
    if (fputs(SD_HISTORY_HEADER, file) == EOF)
    {
      ESP_LOGE(TAG,
               "Failed to write CSV header: %s",
               strerror(errno));
      fclose(file);
      return ESP_FAIL;
    }
  }

  if (fputs(line, file) == EOF)
  {
    ESP_LOGE(TAG,
             "Failed to write %s: %s",
             SD_HISTORY_FILE,
             strerror(errno));
    fclose(file);
    return ESP_FAIL;
  }

  if (fclose(file) != 0)
  {
    ESP_LOGE(TAG,
             "Failed to close %s: %s",
             SD_HISTORY_FILE,
             strerror(errno));
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t sd_card_print_history(void)
{
  if (s_card == NULL)
  {
    return ESP_ERR_INVALID_STATE;
  }

  FILE *file = fopen(SD_HISTORY_FILE, "r");
  if (file == NULL)
  {
    ESP_LOGE(TAG, "Failed to open history: %s", strerror(errno));
    return ESP_FAIL;
  }

  char line[128];

  ESP_LOGI(TAG, "History contents:");
  while (fgets(line, sizeof(line), file) != NULL)
  {
    printf("%s", line);
  }

  bool read_failed = ferror(file);
  fclose(file);

  return read_failed ? ESP_FAIL : ESP_OK;
}
