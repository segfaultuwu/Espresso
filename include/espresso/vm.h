#pragma once

#include "espresso/vm/types.h"

#define MAX_VARS 64

VmResult vm_run(Bytecode *bc);
