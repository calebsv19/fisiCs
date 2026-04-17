// SPDX-License-Identifier: Apache-2.0

#ifndef LEXER_INTERNAL_H
#define LEXER_INTERNAL_H

#include <stdio.h>

#include "lexer.h"

typedef struct {
    int position;
    int line;
    int lineStart;
} LexerMark;

int lexer_debug_enabled(void);

#define LEXER_DEBUG_PRINTF(...) do { if (lexer_debug_enabled()) fprintf(stderr, __VA_ARGS__); } while (0)

const char* lexer_file_path(const Lexer* lexer);
bool lexer_is_system_header_path(const char* file);
LexerMark lexer_mark(const Lexer* lexer);
Token make_token(Lexer* lexer, TokenType type, char* value, LexerMark start);
void report_lexer_error(Lexer* lexer, LexerMark start, const char* message, const char* got);

#endif
