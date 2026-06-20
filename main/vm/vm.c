#include "espresso/vm.h"
#include "esp_log.h"
#include "espresso/hal/gpio.h"
#include "espresso/hal/sleep.h"
#include "espresso/vm/bytecode.h"
#include "espresso/vm/types.h"
#include <stdio.h>

static int vm_vars[MAX_VARS] = {0};

static int get_val(Value v) {
  if (v.type == VAL_VAR) {
    if (v.value >= 0 && v.value < MAX_VARS)
      return vm_vars[v.value];
    return 0;
  }
  return v.value;
}

VmResult vm_run(Bytecode *bc) {

  int stack[MAX_VARS] = {0};
  int depth = 0;

  for (int ip = 0; ip < bc->len; ip++) {
    Instr ins = bc->code[ip];
    ESP_LOGI("VM", "EXEC ip=%d OP=%d dest=%d a=(%d,%d) b=(%d,%d) tgt=%d", ip,
             ins.op, ins.dest.value, ins.a.type, ins.a.value, ins.b.type,
             ins.b.value, ins.target);
    switch (ins.op) {

    case OP_CALL:
      if (depth < MAX_VARS) {
        stack[depth++] = ip;
        ip = ins.target - 1;
      } else {
        ESP_LOGI("VM", "Stack overflow");
        return VmResult_Error;
      }
      break;

    case OP_RET:
      if (depth > 0) {
        ip = stack[--depth];
      } else {
        ESP_LOGI("VM", "Stack underflow");
        return VmResult_Error;
      }
      break;

    case OP_GPIO_WRITE:
      hal_gpio_write(get_val(ins.a), get_val(ins.b));
      break;

    case OP_SLEEP:
      hal_sleep(get_val(ins.a));
      break;

    case OP_PRINT:
      printf("%d\n", get_val(ins.a));
      break;

    case OP_SET_VAR:
      if (ins.dest.type == VAL_VAR && ins.dest.value >= 0 &&
          ins.dest.value < MAX_VARS) {
        vm_vars[ins.dest.value] = get_val(ins.a);
      }
      break;

    case OP_MATH:
      if (ins.dest.type == VAL_VAR && ins.dest.value >= 0 &&
          ins.dest.value < MAX_VARS) {
        int a = get_val(ins.a);
        int b = get_val(ins.b);
        int res = 0;
        if (ins.subop == MATH_ADD)
          res = a + b;
        else if (ins.subop == MATH_SUB)
          res = a - b;
        else if (ins.subop == MATH_MUL)
          res = a * b;
        else if (ins.subop == MATH_DIV)
          res = (b != 0) ? a / b : 0;
        vm_vars[ins.dest.value] = res;
      }
      break;

    case OP_JMP:
      ip = ins.target - 1; // -1 because ip++ in loop
      break;

    case OP_JMP_FALSE: {
      int a = get_val(ins.a);
      int b = get_val(ins.b);
      int is_true = 0;
      if (ins.subop == COND_EQ)
        is_true = (a == b);
      else if (ins.subop == COND_NEQ)
        is_true = (a != b);
      else if (ins.subop == COND_LT)
        is_true = (a < b);
      else if (ins.subop == COND_GT)
        is_true = (a > b);
      else if (ins.subop == COND_LE)
        is_true = (a <= b);
      else if (ins.subop == COND_GE)
        is_true = (a >= b);

      if (!is_true)
        ip = ins.target - 1;
      break;
    }

    case OP_HALT:
      return VmResult_Ok;

    default:
      break;
    }
  }

  return VmResult_Ok;
}
