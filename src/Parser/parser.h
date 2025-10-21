
#ifndef PARSER_H
#define PARSER_H

#include "Compiler/compiler_context.h"

#include <stdlib.h>
#include <stdbool.h>
#include "Parser/Helpers/parsed_type.h"
#include "Lexer/tokens.h"
#include "Lexer/lexer.h"
#include "parser_config.h"
#include "AST/ast_node.h"

typedef struct Parser {
    Token currentToken;
    Token nextToken;
    Token nextNextToken;
    Token nextNextNextToken;
    Lexer* lexer;

    ParserMode mode;

    CompilerContext* ctx;
} Parser;



static inline void parser_set_context(Parser* p, CompilerContext* ctx) {
    p->ctx = ctx;
}


// ---- Main parsing function
ASTNode* parse(Parser* parser);

#endif

