#include "esp_log.h"
#include "espresso/repl.h"

void app_main(void) {
  ESP_LOGI("ESPRESSO", "boot ok");
  repl_init();
}
