#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "espresso/compiler.h"

#define BUFFER_SIZE 65536

void send_message(const char *json) {
    printf("Content-Length: %zu\r\n\r\n%s", strlen(json), json);
    fflush(stdout);
}

void handle_initialize(const char *req, char *id_buf) {
    char response[2048];
    snprintf(response, sizeof(response),
             "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":{"
             "\"capabilities\":{"
             "\"textDocumentSync\":1" // 1 = Full sync
             "}}}", id_buf);
    send_message(response);
}

void extract_id(const char *json, char *id_buf) {
    const char *id_key = "\"id\":";
    const char *p = strstr(json, id_key);
    if (p) {
        p += strlen(id_key);
        while (isspace((unsigned char)*p)) p++;
        int i = 0;
        while (isdigit((unsigned char)*p) && i < 31) {
            id_buf[i++] = *p++;
        }
        id_buf[i] = '\0';
    } else {
        strcpy(id_buf, "null");
    }
}

int main() {
    char header[256];
    char *json_buf = malloc(BUFFER_SIZE);

    if (!json_buf) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }

    while (1) {
        int content_length = 0;

        // Read headers
        while (fgets(header, sizeof(header), stdin)) {
            if (strcmp(header, "\r\n") == 0) break;
            if (strncmp(header, "Content-Length: ", 16) == 0) {
                content_length = atoi(header + 16);
            }
        }

        if (content_length == 0 || content_length >= BUFFER_SIZE) {
            if (feof(stdin)) break;
            continue; // Skip or error
        }

        // Read body
        size_t read_bytes = fread(json_buf, 1, content_length, stdin);
        if (read_bytes < content_length) {
            break;
        }
        json_buf[read_bytes] = '\0';

        // Dispatch
        char id_buf[32] = "null";
        extract_id(json_buf, id_buf);

        if (strstr(json_buf, "\"method\":\"initialize\"") || strstr(json_buf, "\"method\": \"initialize\"")) {
            handle_initialize(json_buf, id_buf);
        } else if (strstr(json_buf, "\"method\":\"shutdown\"") || strstr(json_buf, "\"method\": \"shutdown\"")) {
            char response[256];
            snprintf(response, sizeof(response), "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":null}", id_buf);
            send_message(response);
        } else if (strstr(json_buf, "\"method\":\"exit\"") || strstr(json_buf, "\"method\": \"exit\"")) {
            break;
        } else {
            // For other requests with ID, return empty result to not hang the client
            if (strcmp(id_buf, "null") != 0) {
                char response[256];
                snprintf(response, sizeof(response), "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":null}", id_buf);
                send_message(response);
            }
        }
    }

    free(json_buf);
    return 0;
}
