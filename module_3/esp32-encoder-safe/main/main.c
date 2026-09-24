#include "buzzer/buzzer.h"
#include "configuration.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static void loop(void)
{
  while (1)
  {
    buzzer_update();
    vTaskDelay(pdMS_TO_TICKS(PROCESS_INTERVAL_MS));
  }
}

static void setup(void)
{
  buzzer_init();
  buzzer_play_alarm();
}

void app_main(void)
{
  setup();
  loop();
}
