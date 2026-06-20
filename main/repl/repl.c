#include <stdint.h>

#include <stdint.h>

#include "espresso/vm/types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "esp_log.h"

#include "espresso/repl.h"
#include "espresso/vm.h"
#include "espresso/compiler.h"
#include "espresso/vm/bytecode.h"

#define UART_PORT UART_NUM_0
#define BUF_SIZE 1024

static const char *TAG = "REPL";

static void repl_task(void *arg)
{
    uint8_t c;
    uint8_t data[1];
    char line[256];
    int idx = 0;

    ESP_LOGI(TAG, "REPL started");

    uart_write_bytes(UART_PORT, "\n> ", 3);

    while (1) {
        uart_write_bytes(UART_PORT, "> ", 2);

        int len = uart_read_bytes(UART_PORT, data, 1, portMAX_DELAY);

        if (len <= 0) continue;

        char c = (char)data[0];
        uart_write_bytes(UART_PORT, &c, 1);

        if (c == '\r') continue;

        if (c == '\n') {
            line[idx] = '\0';

            if (idx > 0) {
                ESP_LOGI(TAG, "RUN: %s", line);

                Bytecode bc = compile_source(line);
                vm_run(&bc);
            }

            idx = 0;
        }
        else if (idx < sizeof(line) - 1) {
            line[idx++] = c;
        }
    }
}

void repl_init(void) {
  uart_config_t config = {.baud_rate = 115200,
                          .data_bits = UART_DATA_8_BITS,
                          .parity = UART_PARITY_DISABLE,
                          .stop_bits = UART_STOP_BITS_1,
                          .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

  uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
  uart_param_config(UART_PORT, &config);
  uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

  xTaskCreate(repl_task, "repl_task", 8192, NULL, 5, NULL);
  ESP_LOGI("REPL", "Espresso REPL initalized");
}
