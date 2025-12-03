
#ifndef PARSER_H
#define PARSER_H

#include "Compiler/compiler_context.h"

#include <stdlib.h>
#include <stdbool.h>
#include "Parser/Helpers/parsed_type.h"
#include "Lexer/tokens.h"
#include "Lexer/token_buffer.h"
#include "parser_config.h"
#include "AST/ast_node.h"
#include "Utils/logging.h"

#ifdef PARSER_DEBUG
#define PARSER_DEBUG_PRINTF(...) LOG_DEBUG("parser", __VA_ARGS__)
#else
#define PARSER_DEBUG_PRINTF(...) ((void)0)
#endif

typedef struct Parser {
    Token currentToken;
    Token nextToken;
    Token nextNextToken;
    Token nextNextNextToken;
    TokenBuffer* tokenBuffer;
    size_t cursor;

    ParserMode mode;
    bool enableStatementExpressions;
    bool preserveDirectives;
    CompilerContext* ctx;
} Parser;



static inline void parser_set_context(Parser* p, CompilerContext* ctx) {
    p->ctx = ctx;
}


// ---- Main parsing function
ASTNode* parse(Parser* parser);

#endif
