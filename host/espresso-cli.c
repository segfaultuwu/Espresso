#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "espresso/compiler.h"
#include "espresso/vm/types.h"

void print_usage(const char *prog_name) {
    printf("Espresso SPIFFS Uploader\n");
    printf("Usage: %s upload <file.es> [-p port] [-b baud]\n", prog_name);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "upload") != 0) {
        print_usage(argv[0]);
        return 1;
    }

    const char *filename = argv[2];
    const char *port = "/dev/ttyUSB0";
    const char *baud = "460800";

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = argv[++i];
        } else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            baud = argv[++i];
        }
    }

    const char *idf_path = getenv("IDF_PATH");
    if (!idf_path) {
        fprintf(stderr, "Error: IDF_PATH is not set.\n");
        fprintf(stderr, "Please activate ESP-IDF environment first (. ./esp-idf/export.sh)\n");
        return 1;
    }

    FILE *f_in = fopen(filename, "rb");
    if (!f_in) {
        fprintf(stderr, "Error: File not found: %s\n", filename);
        return 1;
    }

    fseek(f_in, 0, SEEK_END);
    long size = ftell(f_in);
    fseek(f_in, 0, SEEK_SET);

    char *src = malloc(size + 1);
    if (!src) return 1;
    fread(src, 1, size, f_in);
    src[size] = '\0';
    fclose(f_in);

    printf("Uploading '%s' to SPIFFS...\n", filename);

    printf("1. Compiling script to bytecode...\n");
    Bytecode bc = compile_source(src);
    free(src);

    if (bc.len == 0) {
        fprintf(stderr, "Compilation failed or script is empty.\n");
        return 1;
    }

    // Create a temporary directory structure for SPIFFS
    char temp_dir[] = "/tmp/espresso_XXXXXX";
    if (!mkdtemp(temp_dir)) {
        perror("mkdtemp failed");
        return 1;
    }

    char code_bin_path[512];
    snprintf(code_bin_path, sizeof(code_bin_path), "%s/code.bin", temp_dir);

    FILE *f_out = fopen(code_bin_path, "wb");
    if (!f_out) {
        perror("Failed to create code.bin");
        return 1;
    }
    fwrite(bc.code, sizeof(Instr), bc.len, f_out);
    fclose(f_out);

    char spiffs_bin_path[512];
    snprintf(spiffs_bin_path, sizeof(spiffs_bin_path), "%s_spiffs.bin", temp_dir);

    char cmd[2048];
    int ret;

    printf("2. Generating SPIFFS image...\n");
    snprintf(cmd, sizeof(cmd), "python3 %s/components/spiffs/spiffsgen.py 0x40000 %s %s", idf_path, temp_dir, spiffs_bin_path);
    ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Failed to generate SPIFFS image.\n");
        return 1;
    }

    printf("3. Flashing SPIFFS image to port %s at %s baud...\n", port, baud);
    snprintf(cmd, sizeof(cmd), "python3 %s/components/partition_table/parttool.py -p %s -b %s write_partition --partition-name storage --input %s", idf_path, port, baud, spiffs_bin_path);
    ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Failed to write partition.\n");
        return 1;
    }

    printf("4. Resetting the device...\n");
    snprintf(cmd, sizeof(cmd), "python3 %s/components/esptool_py/esptool/esptool.py -p %s hard_reset > /dev/null 2>&1", idf_path, port);
    system(cmd);

    // Cleanup
    snprintf(cmd, sizeof(cmd), "rm -rf %s %s", temp_dir, spiffs_bin_path);
    system(cmd);

    printf("Done! The script should now auto-load on the ESP32.\n");
    return 0;
}
