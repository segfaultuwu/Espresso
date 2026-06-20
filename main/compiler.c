#include <ctype.h>
#include <string.h>

#include "esp_log.h"
#include "espresso/compiler.h"
#include "espresso/vm.h"
#include "espresso/vm/types.h"

#define MAX_INSTR 512
#define MAX_VARS 64

/* Global variable table (top-level scope, shared by all functions). */
static char var_names[MAX_VARS][32];
static int var_count = 0;

static char local_names[MAX_LOCALS][32];
static int local_count = 0;
static int in_function = 0; /* 0 = compiling top-level code, 1 = inside a def body */

static int get_or_create_global(const char *name) {
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

static int find_local(const char *name) {
    if (!in_function) return -1;
    for (int i = 0; i < local_count; i++) {
        if (strcmp(local_names[i], name) == 0) return i;
    }
    return -1;
}

static Value resolve_read(const char *name) {
    int local_idx = find_local(name);
    if (local_idx >= 0) {
        return (Value){VAL_LOCAL, local_idx};
    }
    int global_idx = get_or_create_global(name);
    return (Value){VAL_VAR, global_idx};
}

static Value resolve_assign_target(const char *name) {
    if (in_function) {
        int local_idx = find_local(name);
        if (local_idx >= 0) {
            return (Value){VAL_LOCAL, local_idx};
        }
        if (local_count < MAX_LOCALS) {
            strncpy(local_names[local_count], name, 31);
            local_names[local_count][31] = '\0';
            return (Value){VAL_LOCAL, local_count++};
        }
        ESP_LOGE("COMPILER", "Too many locals in function (max %d)", MAX_LOCALS);
        /* Fall through to global as a last resort rather than losing the write. */
    }
    int global_idx = get_or_create_global(name);
    return (Value){VAL_VAR, global_idx};
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
        return resolve_read(name);
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
    int type; // 1=if, 2=while, 3=for, 4=def
    int start_ip;
    int jmp_ip;
    int for_var; /* for 'for' loops: the loop variable's Value (re-encoded below) */
    Value for_var_val;
} Block;

Bytecode compile_source(const char *src) {
  static Instr code[MAX_INSTR];
  Bytecode bc = {.code = code, .len = 0};

  /* Reset all compiler state so repeated calls (e.g. from a REPL) start clean. */
  var_count = 0;
  local_count = 0;
  in_function = 0;

  char buffer[1024];
  strncpy(buffer, src, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';

  Block block_stack[16];
  int block_depth = 0;
  int func_count = 0;
  char func_names[MAX_VARS][32];
  int func_ips[MAX_VARS];
  int func_arity[MAX_VARS];
  int unresolved_count = 0;
  struct {
      char name[32];
      int instr_idx;
  } unresolved_calls[MAX_VARS];

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
                bc.code[bc.len++] = (Instr){.op = OP_MATH, .subop = MATH_ADD, .dest = b.for_var_val, .a = b.for_var_val, .b = {VAL_CONST, 1}};
                bc.code[bc.len++] = (Instr){.op = OP_JMP, .target = b.start_ip};
                bc.code[b.jmp_ip].target = bc.len;
            } else if (b.type == 1) { // if
                bc.code[b.jmp_ip].target = bc.len;
            } else if (b.type == 4) { // def
                bc.code[bc.len++] = (Instr){.op = OP_RET, .target = b.jmp_ip};
                bc.code[b.jmp_ip].target = bc.len;
                /* Leaving the function body: locals go out of scope. */
                in_function = 0;
                local_count = 0;
            }
        } else {
            ESP_LOGE("COMPILER", "Unexpected 'end'");
        }
        line = strtok(NULL, "\n");
        continue;
    }

    if (strncmp(p, "def ", 4) == 0) {
        p += 4;
        while (isspace((unsigned char)*p)) p++;
        char fname[32] = {0};
        int i = 0;

        while ((isalnum((unsigned char)*p) || *p == '_') && i < 31) {
            fname[i++] = *p++;
        }
        fname[i] = '\0';

        /* Parse the optional parameter list: def name(arg1, arg2) */
        local_count = 0;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '(') {
            p++;
            while (1) {
                while (isspace((unsigned char)*p)) p++;
                if (*p == ')' || *p == '\0') break;
                char pname[32] = {0};
                int j = 0;
                while ((isalnum((unsigned char)*p) || *p == '_') && j < 31) {
                    pname[j++] = *p++;
                }
                pname[j] = '\0';
                if (j > 0 && local_count < MAX_LOCALS) {
                    strncpy(local_names[local_count], pname, 31);
                    local_names[local_count][31] = '\0';
                    local_count++;
                }
                while (isspace((unsigned char)*p)) p++;
                if (*p == ',') { p++; continue; }
                break;
            }
            if (*p == ')') p++;
        }
        in_function = 1;

        /* Emit the placeholder jump FIRST, so func_ips points past it, at
         * the function body's real first instruction (not at the jump). */
        int jmp_ip = bc.len;
        bc.code[bc.len++] = (Instr){.op = OP_JMP, .target = 0};

        if (func_count < MAX_VARS) {
            strncpy(func_names[func_count], fname, 31);
            func_ips[func_count] = bc.len;
            func_arity[func_count] = local_count;
            func_count++;
        }

        block_stack[block_depth++] = (Block){.type = 4, .start_ip = 0, .jmp_ip = jmp_ip};

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
        while (isspace((unsigned char)*p)) p++;
        char varname[32] = {0};
        int vi = 0;
        while ((isalnum((unsigned char)*p) || *p == '_') && vi < 31) {
            varname[vi++] = *p++;
        }
        varname[vi] = '\0';
        Value var = resolve_assign_target(varname);
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
        block_stack[block_depth++] = (Block){.type = 3, .start_ip = start_ip, .jmp_ip = jmp_ip, .for_var_val = var};
        line = strtok(NULL, "\n");
        continue;
    }

    char op[32] = {0};
    int i = 0;
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
            Value dest = resolve_assign_target(op);
            if (m_op != -1) {
                Value val2 = parse_val((const char**)&p);
                bc.code[bc.len++] = (Instr){.op = OP_MATH, .subop = m_op, .dest = dest, .a = val1, .b = val2};
            } else {
                bc.code[bc.len++] = (Instr){.op = OP_SET_VAR, .dest = dest, .a = val1};
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
            /* Possibly a user function call: name(arg1, arg2, ...) */
            Instr call_ins = {.op = OP_CALL, .target = 0, .arg_count = 0};

            while (isspace((unsigned char)*p)) p++;
            if (*p == '(') {
                p++;
                while (1) {
                    while (isspace((unsigned char)*p)) p++;
                    if (*p == ')' || *p == '\0') break;
                    Value arg = parse_val((const char**)&p);
                    if (call_ins.arg_count < MAX_CALL_ARGS) {
                        call_ins.args[call_ins.arg_count++] = arg;
                    }
                    while (isspace((unsigned char)*p)) p++;
                    if (*p == ',') { p++; continue; }
                    break;
                }
                if (*p == ')') p++;
            }

            int is_func = 0;
            for (int j = 0; j < func_count; j++) {
                if (strcmp(op, func_names[j]) == 0) {
                    call_ins.target = func_ips[j];
                    bc.code[bc.len++] = call_ins;
                    is_func = 1;
                    break;
                }
            }
            if (!is_func && op[0] != '\0') {
                if (unresolved_count < MAX_VARS) {
                    strncpy(unresolved_calls[unresolved_count].name, op, 31);
                    unresolved_calls[unresolved_count].instr_idx = bc.len;
                    unresolved_count++;
                    bc.code[bc.len++] = call_ins; /* target patched below once known */
                } else {
                    ESP_LOGE("COMPILER", "Unknown command: %s", op);
                }
            }
        }
    }

    line = strtok(NULL, "\n");
  }

  for (int i = 0; i < unresolved_count; i++) {
      int found = 0;
      for (int j = 0; j < func_count; j++) {
          if (strcmp(unresolved_calls[i].name, func_names[j]) == 0) {
              bc.code[unresolved_calls[i].instr_idx].target = func_ips[j];
              found = 1;
              break;
          }
      }
      if (!found) ESP_LOGE("COMPILER", "Unresolved function: %s", unresolved_calls[i].name);
  }

  return bc;
}
