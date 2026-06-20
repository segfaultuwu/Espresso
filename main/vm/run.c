#include "espresso/hal/gpio.h"
#include "espresso/hal/sleep.h"
#include "espresso/vm.h"
#include "espresso/vm/bytecode.h"
#include "espresso/vm/opcode.h"

void vm_run(Bytecode *bc) {
  for (int i = 0; i < bc->len; i++) {
    Instr ins = bc->code[i];

    switch (ins.op) {
    case OP_GPIO_WRITE:
      hal_gpio_write(ins.a, ins.b);
      break;

    case OP_SLEEP:
      hal_sleep(ins.a);
      break;
    }
  }
}
