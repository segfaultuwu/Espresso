#include <stdio.h>
#include <stdlib.h>
#include "espresso/compiler.h"
#include "espresso/vm/types.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.es> <output.bin>\n", argv[0]);
        return 1;
    }

    FILE *f_in = fopen(argv[1], "rb");
    if (!f_in) {
        perror("Failed to open input script");
        return 1;
    }

    fseek(f_in, 0, SEEK_END);
    long size = ftell(f_in);
    fseek(f_in, 0, SEEK_SET);

    char *src = malloc(size + 1);
    if (!src) {
        fclose(f_in);
        return 1;
    }

    fread(src, 1, size, f_in);
    src[size] = '\0';
    fclose(f_in);

    Bytecode bc = compile_source(src);
    free(src);

    if (bc.len == 0) {
        fprintf(stderr, "Failed to compile or empty script.\n");
        return 1;
    }

    FILE *f_out = fopen(argv[2], "wb");
    if (!f_out) {
        perror("Failed to open output binary");
        return 1;
    }

    fwrite(bc.code, sizeof(Instr), bc.len, f_out);
    fclose(f_out);

    printf("Compiled %zu instructions to %s\n", bc.len, argv[2]);
    return 0;
}
