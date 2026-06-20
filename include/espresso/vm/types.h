#pragma once
#include <stddef.h>
#include <stdint.h>

typedef enum {
  OP_GPIO_WRITE,
  OP_SLEEP,
  OP_HALT,
  OP_INVALID,
  OP_POP,
  OP_PUSH
} OpCode;

typedef struct {
    OpCode op;
    int a;
    int b;
} Instr;

typedef struct {
    Instr *code;
    size_t len;
} Bytecode;

typedef enum {
    VmResult_Ok,
    VmResult_Halted,
    VmResult_Error
} VmResult;
