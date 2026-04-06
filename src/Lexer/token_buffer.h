// SPDX-License-Identifier: Apache-2.0

#ifndef TOKEN_BUFFER_H
#define TOKEN_BUFFER_H

#include <stddef.h>
#include <stdbool.h>

#include "lexer.h"

typedef struct {
    Token* tokens;
    size_t count;
    size_t capacity;
} TokenBuffer;

void token_buffer_init(TokenBuffer* buffer);
void token_buffer_destroy(TokenBuffer* buffer);
bool token_buffer_append(TokenBuffer* buffer, Token token);
const Token* token_buffer_peek(const TokenBuffer* buffer, size_t index);
bool token_buffer_fill_from_lexer(TokenBuffer* buffer, Lexer* lexer);

#endif
