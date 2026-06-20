#pragma once

#include "espresso/vm/types.h"

typedef enum {
    VM_OK,
    VM_HALTED,
    VM_ERROR
} VmResult;

VmResult vm_run(Bytecode *bc);
