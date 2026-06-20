#pragma once

#include "vm/types.h"

Bytecode compile_source(const char *src);
void free_bytecode(Bytecode bc);
