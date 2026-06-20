#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "espresso/compiler.h"
#include "espresso/vm/types.h"

#define MAX_INSTR 128

static OpCode parse_opcode(const char *op) {
  if (strcmp(op, "gpio") == 0)
    return OP_GPIO_WRITE;
  if (strcmp(op, "sleep") == 0)
    return OP_SLEEP;
  if (strcmp(op, "halt") == 0)
    return OP_HALT;

  return OP_INVALID;
}

Bytecode compile_source(const char *src) {
  static Instr code[MAX_INSTR];

  Bytecode bc = {.code = code, .len = 0};

  char buffer[512];
  strncpy(buffer, src, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';

  char *line = strtok(buffer, "\n");

  while (line && bc.len < MAX_INSTR) {

    while (isspace((unsigned char)*line))
      line++;

    if (*line == '\0') {
      line = strtok(NULL, "\n");
      continue;
    }

    char op[32] = {0};
    int a = 0, b = 0;

    int args = sscanf(line, "%31[^ (](%d,%d)", op, &a, &b);

    if (args < 1) {
      args = sscanf(line, "%31s %d %d", op, &a, &b);
    }

    OpCode opcode = parse_opcode(op);

    ESP_LOGI("COMPILER", "OP='%s' A=%d B=%d", op, a, b);

    if (opcode == OP_INVALID) {
      ESP_LOGE("COMPILER", "Unknown command: %s", op);
      line = strtok(NULL, "\n");
      continue;
    }

    Instr ins = {
        .op = opcode,
        .a = a,
        .b = b,
    };

    bc.code[bc.len++] = ins;

    ESP_LOGI("COMPILER", "compiled: op=%d a=%d b=%d", opcode, a, b);

    line = strtok(NULL, "\n");
  }

  return bc;
}
