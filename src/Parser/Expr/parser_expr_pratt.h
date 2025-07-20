#ifndef _PARSER_EXPR_PRATT_H
#define _PARSER_EXPR_PRATT_H

#include "parser.h"
#include "parsed_type.h"

#include "AST/ast_node.h"
#include "Lexer/tokens.h"
#include "Lexer/lexer.h"

// Main Pratt-based entry point
ASTNode* parseExpressionPratt(Parser* parser, int minPrecedence);

// Helpers for testing and debugging (optional)
int getTokenPrecedence(TokenType type);
int getTokenRightBindingPower(TokenType type);


// parse methods
ASTNode* parseFunctionCallPratt(Parser* parser, ASTNode* callee);
ASTNode* parseCastExpressionPratt(Parser* parser, bool alreadyConsumedLParen);

#endif // _PARSER_EXPR_PRATT_H

