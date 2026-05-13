// SPDX-License-Identifier: Apache-2.0

#ifndef TOKENS_H
#define TOKENS_H

#include "Lexer/source_range.h"
#include "Lexer/token_type.h"

typedef struct Token {
    TokenType type;
    char* value;
    int line;
    SourceRange location;
    SourceRange macroCallSite;
    SourceRange macroDefinition;
} Token;

#endif
