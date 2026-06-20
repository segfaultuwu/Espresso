#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "portmacro.h"

void hal_sleep(int ms) { vTaskDelay(ms / portTICK_PERIOD_MS); }
