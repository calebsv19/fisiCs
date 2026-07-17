// SPDX-License-Identifier: Apache-2.0

#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stddef.h>

#include "tokens.h"

struct CompilerContext;

typedef struct {
    const char* source;
    const char* filePath;
    int position;
    int length;
    int line;
    int lineStart;
    int fatalErrorCount;
    const char* rawSource;  // original source before phase-1/phase-2 translation
    int* rawLineMap;        // translated position -> original physical line
    int* rawColumnMap;      // translated position -> original physical column
    char* ownedSource;   // translated buffer (trigraph/digraph), owned if non-NULL
    bool enableTrigraphs;
} Lexer;

// Function declarations
void initLexer(Lexer* lexer, const char* source, const char* filePath, bool enableTrigraphs);
void destroyLexer(Lexer* lexer);
void lexer_set_diag_context(struct CompilerContext* ctx);
Token getNextToken(Lexer* lexer);
void skipWhitespace(Lexer* lexer);
int isEOF(Lexer* lexer);

const char *lookupKeyword(const char *str, size_t len);
Token handleIdentifierOrKeyword(Lexer* lexer);
Token handleNumber(Lexer* lexer);
Token handleStringLiteral(Lexer* lexer);
Token handleCharLiteral(Lexer* lexer);
Token handlePreprocessorDirective(Lexer* lexer);
Token handleComment(Lexer* lexer);
Token handleOperator(Lexer* lexer);
Token handleSingleCharOperator(Lexer* lexer);
Token handlePunctuation(Lexer* lexer);
Token handleUnknownToken(Lexer* lexer);

TokenType keywordToTokenType(const char* word);

#endif
