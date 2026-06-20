#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "espresso/compiler.h"
#include "espresso/repl.h"
#include "espresso/vm.h"
#include "esp_spiffs.h"
#include "espresso/vm/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

void app_main(void) {
  ESP_LOGI("ESPRESSO", "boot ok");

  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
  };

  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
          ESP_LOGE("ESPRESSO", "Failed to mount or format filesystem");
      } else if (ret == ESP_ERR_NOT_FOUND) {
          ESP_LOGE("ESPRESSO", "Failed to find SPIFFS partition");
      } else {
          ESP_LOGE("ESPRESSO", "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
      }
  } else {
      ESP_LOGI("ESPRESSO", "SPIFFS mounted");

      FILE* f = fopen("/spiffs/code.bin", "rb");
      if (f == NULL) {
          ESP_LOGI("ESPRESSO", "No code.bin found on SPIFFS");
      } else {
          ESP_LOGI("ESPRESSO", "Auto-loading pre-compiled /spiffs/code.bin...");

          fseek(f, 0, SEEK_END);
          long size = ftell(f);
          fseek(f, 0, SEEK_SET);

          size_t num_instr = size / sizeof(Instr);
          if (num_instr > 0) {
              Instr* code = malloc(size);
              if (code) {
                  fread(code, sizeof(Instr), num_instr, f);

                  Bytecode bc = { .code = code, .len = num_instr };
                  ESP_LOGI("ESPRESSO", "Loaded %d instructions. Running...", (int)num_instr);

                  vm_run(&bc);
                  free(code);
              }
          } else {
              ESP_LOGI("ESPRESSO", "File code.bin is empty or invalid size.");
          }
          fclose(f);
      }
  }

  repl_init();
}
