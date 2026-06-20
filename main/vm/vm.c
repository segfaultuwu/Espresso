#include "espresso/vm.h"
#include "esp_log.h"
#include "espresso/hal/gpio.h"
#include "espresso/hal/sleep.h"
#include "espresso/vm/bytecode.h"
#include "espresso/vm/types.h"
#include <stdio.h>

#define MAX_VARS 64

/* Global variables: shared across all functions and the top-level script. */
static int vm_vars[MAX_VARS] = {0};

typedef struct {
    int return_ip;
    int locals[MAX_LOCALS];
} Frame;

static Frame call_stack[MAX_CALL_DEPTH];
static int frame_ptr = -1; /* -1 means "no active frame" (top-level/global scope) */

static int get_val(Value v) {
    switch (v.type) {
    case VAL_VAR:
        if (v.value >= 0 && v.value < MAX_VARS)
            return vm_vars[v.value];
        return 0;
    case VAL_LOCAL:
        if (frame_ptr >= 0 && v.value >= 0 && v.value < MAX_LOCALS)
            return call_stack[frame_ptr].locals[v.value];
        return 0;
    case VAL_CONST:
        return v.value;
    case VAL_NONE:
    default:
        return 0;
    }
}

static void set_val(Value dest, int result) {
    if (dest.type == VAL_VAR) {
        if (dest.value >= 0 && dest.value < MAX_VARS)
            vm_vars[dest.value] = result;
    } else if (dest.type == VAL_LOCAL) {
        if (frame_ptr >= 0 && dest.value >= 0 && dest.value < MAX_LOCALS)
            call_stack[frame_ptr].locals[dest.value] = result;
    }
}

VmResult vm_run(Bytecode *bc) {
    frame_ptr = -1;

    for (int ip = 0; ip < (int)bc->len; ip++) {
        Instr ins = bc->code[ip];
        /* ESP_LOGI("VM", "EXEC ip=%d OP=%d dest=%d a=(%d,%d) b=(%d,%d) tgt=%d "
                       "frame=%d",
                 ip, ins.op, ins.dest.value, ins.a.type, ins.a.value,
                 ins.b.type, ins.b.value, ins.target, frame_ptr);

        The above log is for debug only.
        */

        switch (ins.op) {

        case OP_CALL: {
            if (frame_ptr + 1 >= MAX_CALL_DEPTH) {
                ESP_LOGE("VM", "Stack overflow (max call depth %d)",
                         MAX_CALL_DEPTH);
                return VmResult_Error;
            }

            int evaluated_args[MAX_CALL_ARGS];
            for (int i = 0; i < ins.arg_count && i < MAX_CALL_ARGS; i++) {
                evaluated_args[i] = get_val(ins.args[i]);
            }

            int new_frame = frame_ptr + 1;
            call_stack[new_frame].return_ip = ip;
            for (int i = 0; i < MAX_LOCALS; i++) {
                call_stack[new_frame].locals[i] = 0;
            }
            for (int i = 0; i < ins.arg_count && i < MAX_LOCALS; i++) {
                call_stack[new_frame].locals[i] = evaluated_args[i];
            }

            frame_ptr = new_frame;
            ip = ins.target - 1; /* -1 because of ip++ in the for loop */
            break;
        }

        case OP_RET:
            if (frame_ptr < 0) {
                ESP_LOGE("VM", "Stack underflow");
                return VmResult_Error;
            }
            ip = call_stack[frame_ptr].return_ip;
            frame_ptr--;
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
            set_val(ins.dest, get_val(ins.a));
            break;

        case OP_MATH: {
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
            set_val(ins.dest, res);
            break;
        }

        case OP_JMP:
            ip = ins.target - 1;
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
