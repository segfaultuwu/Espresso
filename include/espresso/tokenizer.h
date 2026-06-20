#pragma once

typedef enum {
    TOK_GPIO,
    TOK_SLEEP,
    TOK_NUMBER,
    TOK_HIGH,
    TOK_LOW,
    TOK_EOF
} TokenType;

typedef struct {
    TokenType type;
    int value;
} Token;

Token next_token(const char **src);
