#include "espresso/vm/types.h"

void free_bytecode(Bytecode bc) {
  (void)bc; // no-op: code points to a static buffer owned by compile_source
}
