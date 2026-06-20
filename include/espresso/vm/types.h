#pragma once
#include <stddef.h>
#include <stdint.h>

#define MAX_CALL_DEPTH 16
#define MAX_LOCALS 16
#define MAX_CALL_ARGS 8

typedef enum {
  OP_GPIO_WRITE,
  OP_SLEEP,
  OP_HALT,
  OP_SET_VAR,
  OP_PRINT,
  OP_MATH,
  OP_JMP,
  OP_JMP_FALSE,
  OP_INVALID,
  OP_POP,
  OP_PUSH,
  OP_CALL,
  OP_RET,
  OP_DEF
} OpCode;

typedef enum {
    MATH_ADD,
    MATH_SUB,
    MATH_MUL,
    MATH_DIV
} MathOp;

typedef enum {
    COND_EQ,
    COND_NEQ,
    COND_LT,
    COND_GT,
    COND_LE,
    COND_GE
} CondOp;

typedef enum {
    VAL_CONST,
    VAL_VAR,    // global variable, indexed into vm_vars[]
    VAL_LOCAL,  // local variable/parameter, indexed into the current call frame
    VAL_NONE
} ValType;

typedef struct {
    ValType type;
    int value;
} Value;

typedef struct {
    OpCode op;
    int target;
    int subop;
    Value dest;
    Value a;
    Value b;
    Value args[MAX_CALL_ARGS];
    int arg_count;
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
