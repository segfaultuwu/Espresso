#include "espresso/vm.h"
#include "espresso/vm/bytecode.h"

#include "espresso/hal/gpio.h"
#include "espresso/hal/sleep.h"

#include "esp_log.h"
#include "espresso/vm/types.h"

static const char *TAG = "ESPRESSO_VM";

void vm_run(Bytecode *bc) {
  if (!bc || !bc->code) {
    ESP_LOGE(TAG, "Invalid bytecode");
    return;
  }

  ESP_LOGI(TAG, "VM start: instructions=%d", bc->len);

  for (int i = 0; i < bc->len; i++) {
    Instr ins = bc->code[i];

    switch ((OpCode)ins.op) {

    case OP_GPIO_WRITE:
      ESP_LOGI(TAG, "GPIO_WRITE pin=%d value=%d", ins.a, ins.b);
      hal_gpio_write(ins.a, ins.b);
      break;

    case OP_SLEEP:
      ESP_LOGI(TAG, "SLEEP %d ms", ins.a);
      hal_sleep(ins.a);
      break;

    case OP_PUSH:
      ESP_LOGW(TAG, "OP_PUSH not implemented yet");
      break;

    case OP_POP:
      ESP_LOGW(TAG, "OP_POP not implemented yet");
      break;

    case OP_HALT:
      ESP_LOGI(TAG, "HALT");
      return;

    default:
      ESP_LOGE(TAG, "Unknown opcode: %d", ins.op);
      return;
    }
  }

  ESP_LOGI(TAG, "VM finished");
}
