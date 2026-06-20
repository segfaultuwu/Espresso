#include <stdint.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "espresso/vm/types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "esp_log.h"

#include "espresso/compiler.h"
#include "espresso/repl.h"
#include "espresso/vm.h"
#include "espresso/vm/bytecode.h"
#include "portmacro.h"

#define UART_PORT UART_NUM_0
#define BUF_SIZE 1024

static const char *TAG = "REPL";

static void run_line(const char *line) {
  Bytecode bc = compile_source(line);

  if (!bc.code || bc.len == 0) {
    ESP_LOGW(TAG, "compile failed");
    return;
  }

  ESP_LOGI(TAG, "input: '%s'", line);
  ESP_LOGI(TAG, "bc.len = %d", bc.len);

  vm_run(&bc);
}

static void repl_task(void *arg) {
    char line[128];
    int pos = 0;
    uint8_t c;

    uart_write_bytes(UART_PORT, "\r\n> ", 4);

    while (1) {
        int len = uart_read_bytes(UART_PORT, &c, 1, portMAX_DELAY);

        if (len <= 0) continue;

        uart_write_bytes(UART_PORT, (char*)&c, 1);

        if (c == '\r' || c == '\n') {
            if (pos > 0) {
                line[pos] = 0;
                uart_write_bytes(UART_PORT, "\r\n", 2);
                run_line(line);
                pos = 0;
                uart_write_bytes(UART_PORT, "> ", 2);
            } else if (c == '\r') {
                uart_write_bytes(UART_PORT, "\r\n> ", 4);
            } else if (c == '\n') {
                // Ignore '\n' if we just processed '\r' or if it's an empty line
            }
            continue;
        }

        if (pos < sizeof(line) - 1)
            line[pos++] = c;
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
