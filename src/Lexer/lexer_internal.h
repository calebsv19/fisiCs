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
void lexer_mark_source_location(const Lexer* lexer, LexerMark mark, int* outLine, int* outColumn);
Token make_token(Lexer* lexer, TokenType type, char* value, LexerMark start);
void report_lexer_error(Lexer* lexer, LexerMark start, const char* message, const char* got);
int lexer_identifier_ucn_length(const Lexer* lexer, int position, bool initial);
int lexer_identifier_utf8_length(const Lexer* lexer, int position, bool initial);
char* lexer_normalize_identifier_spelling(const char* text, size_t length);

#endif
