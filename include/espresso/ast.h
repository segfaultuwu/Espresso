#pragma once

typedef enum {
    NODE_GPIO,
    NODE_SLEEP
} NodeType;

typedef struct Node {
    NodeType type;
    int a;
    int b;
} Node;
