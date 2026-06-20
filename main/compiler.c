#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "espresso/compiler.h"
#include "espresso/vm/types.h"

#define MAX_INSTR 128
#define MAX_LINE 128

static int parse_int(const char *s) { return atoi(s); }

static OpCode parse_opcode(const char *op) {
  if (strcmp(op, "gpio") == 0)
    return OP_GPIO_WRITE;
  if (strcmp(op, "sleep") == 0)
    return OP_SLEEP;
  if (strcmp(op, "halt") == 0)
    return OP_HALT;

  return OP_HALT; // fallback
}

Bytecode compile_source(const char *src) {
  static Instr code[MAX_INSTR];

  Bytecode bc = {.code = code, .len = 0};

  char buffer[512];
  strncpy(buffer, src, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';

  char *line = strtok(buffer, "\n");

  while (line && bc.len < MAX_INSTR) {

    // skip empty lines
    while (isspace(*line))
      line++;
    if (*line == '\0') {
      line = strtok(NULL, "\n");
      continue;
    }

    char op[32];
    int a = 0, b = 0;

    int args = sscanf(line, "%31s %d %d", op, &a, &b);

    OpCode opcode = parse_opcode(op);

    Instr ins = {.op = opcode, .a = a, .b = b};

    bc.code[bc.len++] = ins;

    line = strtok(NULL, "\n");
  }

  return bc;
}
