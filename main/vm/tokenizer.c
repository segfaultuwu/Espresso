#include "espresso/tokenizer.h"
#include <ctype.h>
#include <string.h>

Token next_token(const char **src) {
  while (**src == ' ')
    (*src)++;

  if (strncmp(*src, "gpio", 4) == 0) {
    *src += 4;
    return (Token){TOK_GPIO, 0};
  }

  if (strncmp(*src, "sleep", 5) == 0) {
    *src += 5;
    return (Token){TOK_SLEEP, 0};
  }

  if (isdigit(**src)) {
    int val = 0;
    while (isdigit(**src)) {
      val = val * 10 + (**src - '0');
      (*src)++;
    }
    return (Token){TOK_NUMBER, val};
  }

  return (Token){TOK_EOF, 0};
}
