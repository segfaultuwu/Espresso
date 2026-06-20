#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"

void hal_sleep(int ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
