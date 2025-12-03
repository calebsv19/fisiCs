#include "token_buffer.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static const Token* token_buffer_sentinel(void) {
    static const Token token = {
        TOKEN_EOF,
        (char*)"EOF",
        0,
        { { "<unknown>", 0, 0 }, { "<unknown>", 0, 0 } },
        { { NULL, 0, 0 }, { NULL, 0, 0 } },
        { { NULL, 0, 0 }, { NULL, 0, 0 } }
    };
    return &token;
}

void token_buffer_init(TokenBuffer* buffer) {
    if (!buffer) return;
    buffer->tokens = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
}

void token_buffer_destroy(TokenBuffer* buffer) {
    if (!buffer) return;
    free(buffer->tokens);
    buffer->tokens = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
}

bool token_buffer_append(TokenBuffer* buffer, Token token) {
    if (!buffer) return false;
    if (buffer->count == buffer->capacity) {
        size_t newCapacity = (buffer->capacity == 0) ? 64 : buffer->capacity * 2;
        Token* newData = (Token*)realloc(buffer->tokens, newCapacity * sizeof(Token));
        if (!newData) {
            return false;
        }
        buffer->tokens = newData;
        buffer->capacity = newCapacity;
    }
    buffer->tokens[buffer->count++] = token;
    return true;
}

const Token* token_buffer_peek(const TokenBuffer* buffer, size_t index) {
    if (!buffer || buffer->count == 0) {
        return token_buffer_sentinel();
    }
    if (index >= buffer->count) {
        return &buffer->tokens[buffer->count - 1];
    }
    return &buffer->tokens[index];
}

bool token_buffer_fill_from_lexer(TokenBuffer* buffer, Lexer* lexer) {
    if (!buffer || !lexer) return false;
    while (1) {
        Token token = getNextToken(lexer);
        if (!token_buffer_append(buffer, token)) {
            return false;
        }
        if (token.type == TOKEN_EOF) {
            break;
        }
    }
    return true;
}
