// SPDX-License-Identifier: Apache-2.0

#ifndef SYNTAX_ERRORS_H
#define SYNTAX_ERRORS_H

#include <stddef.h>
#include <stdbool.h>
#include "Lexer/tokens.h"
#include "Compiler/compiler_context.h"

typedef struct {
    int line;
    int column;
    char* message;
    char* hint;
    bool isWarning;
    SourceRange spelling;
    SourceRange macroCallSite;
    SourceRange macroDefinition;
} SyntaxError;

typedef struct {
    SyntaxError* errors;
    size_t count;
    size_t capacity;
    size_t errorCount;
} ErrorList;

void initErrorList(struct CompilerContext* ctx);
void addError(int line, int column, const char* message, const char* hint);
void addErrorFromToken(const Token* tok, const char* message, const char* hint);
void addErrorWithRanges(SourceRange spelling,
                        SourceRange macroCallSite,
                        SourceRange macroDefinition,
                        const char* message,
                        const char* hint);
void addWarning(int line, int column, const char* message, const char* hint);
void reportErrors(void);
size_t getErrorCount(void);
void freeErrorList(void);

#endif // SYNTAX_ERRORS_H
