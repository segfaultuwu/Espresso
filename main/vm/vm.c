#include "esp_log.h"
#include "espresso/hal/gpio.h"
#include "espresso/hal/sleep.h"
#include "espresso/vm/bytecode.h"
#include "espresso/vm/types.h"
#include <stdio.h>

VmResult vm_run(Bytecode *bc) {
  for (int ip = 0; ip < bc->len; ip++) {
    Instr ins = bc->code[ip];
    ESP_LOGI("VM", "EXEC OP=%d a=%d b=%d", ins.op, ins.a, ins.b);
    switch (ins.op) {

    case OP_GPIO_WRITE:
      hal_gpio_write(ins.a, ins.b);
      break;

    case OP_SLEEP:
      hal_sleep(ins.a);
      break;

    case OP_HALT:
      return VmResult_Ok;

    default:
      return VmResult_Error;
    }
  }

  return VmResult_Ok;
}
