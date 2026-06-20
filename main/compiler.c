#include <ctype.h>
#include <string.h>

#include "esp_log.h"
#include "espresso/compiler.h"
#include "espresso/vm/types.h"

#define MAX_INSTR 512
#define MAX_VARS 64

static char var_names[MAX_VARS][32];
static int var_count = 0;

static int get_or_create_var(const char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(var_names[i], name) == 0) return i;
    }
    if (var_count < MAX_VARS) {
        strncpy(var_names[var_count], name, 31);
        var_names[var_count][31] = '\0';
        return var_count++;
    }
    return -1;
}

static OpCode parse_opcode(const char *op) {
  if (strcmp(op, "gpio") == 0) return OP_GPIO_WRITE;
  if (strcmp(op, "sleep") == 0) return OP_SLEEP;
  if (strcmp(op, "halt") == 0) return OP_HALT;
  if (strcmp(op, "puts") == 0 || strcmp(op, "print") == 0) return OP_PRINT;
  return OP_INVALID;
}

static CondOp parse_cond(const char **s) {
    while (isspace((unsigned char)**s)) (*s)++;
    if (strncmp(*s, "==", 2) == 0) { *s += 2; return COND_EQ; }
    if (strncmp(*s, "!=", 2) == 0) { *s += 2; return COND_NEQ; }
    if (strncmp(*s, "<=", 2) == 0) { *s += 2; return COND_LE; }
    if (strncmp(*s, ">=", 2) == 0) { *s += 2; return COND_GE; }
    if (**s == '<') { (*s)++; return COND_LT; }
    if (**s == '>') { (*s)++; return COND_GT; }
    return COND_EQ;
}

static int parse_math(const char **s) {
    while (isspace((unsigned char)**s)) (*s)++;
    if (**s == '+') { (*s)++; return MATH_ADD; }
    if (**s == '-') { (*s)++; return MATH_SUB; }
    if (**s == '*') { (*s)++; return MATH_MUL; }
    if (**s == '/') { (*s)++; return MATH_DIV; }
    return -1;
}

static Value parse_val(const char **s) {
    while (isspace((unsigned char)**s)) (*s)++;
    if (isalpha((unsigned char)**s)) {
        char name[32];
        int i = 0;
        while ((isalnum((unsigned char)**s) || **s == '_') && i < 31) {
            name[i++] = **s;
            (*s)++;
        }
        name[i] = '\0';
        int var_idx = get_or_create_var(name);
        return (Value){VAL_VAR, var_idx};
    }
    if (isdigit((unsigned char)**s) || **s == '-') {
        int val = 0;
        int sign = 1;
        if (**s == '-') { sign = -1; (*s)++; }
        while (isdigit((unsigned char)**s)) {
            val = val * 10 + (**s - '0');
            (*s)++;
        }
        return (Value){VAL_CONST, val * sign};
    }
    return (Value){VAL_NONE, 0};
}

typedef struct {
    int type; // 1=if, 2=while, 3=for
    int start_ip;
    int jmp_ip;
    int for_var;
} Block;

Bytecode compile_source(const char *src) {
  static Instr code[MAX_INSTR];
  Bytecode bc = {.code = code, .len = 0};

  char buffer[1024];
  strncpy(buffer, src, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';

  Block block_stack[16];
  int block_depth = 0;

  char *line = strtok(buffer, "\n");

  while (line && bc.len < MAX_INSTR) {
    char *p = line;
    while (isspace((unsigned char)*p)) p++;

    if (*p == '\0' || *p == '#') {
        line = strtok(NULL, "\n");
        continue;
    }

    if (strncmp(p, "end", 3) == 0 && (isspace((unsigned char)p[3]) || p[3] == '\0')) {
        if (block_depth > 0) {
            Block b = block_stack[--block_depth];
            if (b.type == 2) { // while
                bc.code[bc.len++] = (Instr){.op = OP_JMP, .target = b.start_ip};
                bc.code[b.jmp_ip].target = bc.len;
            } else if (b.type == 3) { // for
                bc.code[bc.len++] = (Instr){.op = OP_MATH, .subop = MATH_ADD, .dest = {VAL_VAR, b.for_var}, .a = {VAL_VAR, b.for_var}, .b = {VAL_CONST, 1}};
                bc.code[bc.len++] = (Instr){.op = OP_JMP, .target = b.start_ip};
                bc.code[b.jmp_ip].target = bc.len;
            } else if (b.type == 1) { // if
                bc.code[b.jmp_ip].target = bc.len;
            }
        } else {
            ESP_LOGE("COMPILER", "Unexpected 'end'");
        }
        line = strtok(NULL, "\n");
        continue;
    }

    if (strncmp(p, "while ", 6) == 0) {
        p += 6;
        Value a = parse_val((const char**)&p);
        CondOp cond = parse_cond((const char**)&p);
        Value b = parse_val((const char**)&p);
        block_stack[block_depth++] = (Block){.type = 2, .start_ip = bc.len, .jmp_ip = bc.len};
        bc.code[bc.len++] = (Instr){.op = OP_JMP_FALSE, .subop = cond, .a = a, .b = b};
        line = strtok(NULL, "\n");
        continue;
    }

    if (strncmp(p, "if ", 3) == 0) {
        p += 3;
        Value a = parse_val((const char**)&p);
        CondOp cond = parse_cond((const char**)&p);
        Value b = parse_val((const char**)&p);
        block_stack[block_depth++] = (Block){.type = 1, .start_ip = bc.len, .jmp_ip = bc.len};
        bc.code[bc.len++] = (Instr){.op = OP_JMP_FALSE, .subop = cond, .a = a, .b = b};
        line = strtok(NULL, "\n");
        continue;
    }

    if (strncmp(p, "for ", 4) == 0) {
        p += 4;
        Value var = parse_val((const char**)&p);
        while(isspace((unsigned char)*p)) p++;
        if (*p == '=') p++;
        Value start_val = parse_val((const char**)&p);
        while(isspace((unsigned char)*p)) p++;
        if (strncmp(p, "to ", 3) == 0) p += 3;
        Value end_val = parse_val((const char**)&p);

        bc.code[bc.len++] = (Instr){.op = OP_SET_VAR, .dest = var, .a = start_val};
        int start_ip = bc.len;
        int jmp_ip = bc.len;
        bc.code[bc.len++] = (Instr){.op = OP_JMP_FALSE, .subop = COND_LE, .a = var, .b = end_val};
        block_stack[block_depth++] = (Block){.type = 3, .start_ip = start_ip, .jmp_ip = jmp_ip, .for_var = var.value};
        line = strtok(NULL, "\n");
        continue;
    }

    char op[32] = {0};
    int i = 0;
    char *cmd_start = p;
    while (*p && !isspace((unsigned char)*p) && *p != '=' && *p != '(' && i < 31) {
        op[i++] = *p++;
    }
    op[i] = '\0';
    while (isspace((unsigned char)*p)) p++;

    if (*p == '=') {
        p++;
        Value val1 = parse_val((const char**)&p);
        int m_op = parse_math((const char**)&p);

        if (isalpha((unsigned char)op[0])) {
            int var_idx = get_or_create_var(op);
            if (var_idx >= 0) {
                if (m_op != -1) {
                    Value val2 = parse_val((const char**)&p);
                    bc.code[bc.len++] = (Instr){.op = OP_MATH, .subop = m_op, .dest = {VAL_VAR, var_idx}, .a = val1, .b = val2};
                } else {
                    bc.code[bc.len++] = (Instr){.op = OP_SET_VAR, .dest = {VAL_VAR, var_idx}, .a = val1};
                }
            }
        }
    } else {
        OpCode opcode = parse_opcode(op);
        if (opcode != OP_INVALID) {
            if (*p == '(') p++;
            Value a = parse_val((const char**)&p);
            while (isspace((unsigned char)*p)) p++;
            if (*p == ',') p++;
            Value b = parse_val((const char**)&p);
            bc.code[bc.len++] = (Instr){.op = opcode, .a = a, .b = b};
        } else {
            ESP_LOGE("COMPILER", "Unknown command: %s", op);
        }
    }

    line = strtok(NULL, "\n");
  }

  return bc;
}
