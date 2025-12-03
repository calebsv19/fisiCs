#ifndef LEXER_H
#define LEXER_H


#include "tokens.h"

typedef struct {
    const char* source;
    const char* filePath;
    int position;
    int line;
    int lineStart;
} Lexer;

// Function declarations
void initLexer(Lexer* lexer, const char* source, const char* filePath);
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
