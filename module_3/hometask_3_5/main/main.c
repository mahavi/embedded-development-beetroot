#include "configuration.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void loop(void)
{
  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(PROCESS_INTERVAL_MS));
  }
}

static void setup(void)
{
}

void app_main(void)
{
  setup();
  loop();
}
