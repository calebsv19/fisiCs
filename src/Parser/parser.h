
#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <stdbool.h>
#include "parsed_type.h"
#include "Lexer/tokens.h"
#include "Lexer/lexer.h"
#include "parser_config.h"
#include "AST/ast_node.h"

typedef struct {
    Token currentToken;
    Token nextToken;
    Token nextNextToken;
    Token nextNextNextToken;
    Lexer* lexer;

    ParserMode mode;
} Parser;




// ---- Main parsing function
ASTNode* parse(Parser* parser);

#endif

