#ifndef _PARSER_EXPR_PRATT_H
#define _PARSER_EXPR_PRATT_H

#include "parser.h"
#include "Parser/Helpers/parsed_type.h"

#include "AST/ast_node.h"
#include "Lexer/tokens.h"
#include "Lexer/lexer.h"

// Main Pratt-based entry point
ASTNode* parseExpressionPratt(Parser* parser, int minPrecedence);

// Helpers for testing and debugging (optional)
int getTokenPrecedence(TokenType type);
int getTokenRightBindingPower(TokenType type);


// parse methods
bool consumeBalancedParens(Parser* p);
void consumeAbstractDeclarator(Parser* p);
void consumeAbstractDeclaratorIntoType(Parser* p, ParsedType* type);
bool looksLikeParenTypeName(Parser* parser);
ASTNode* ledFunctionCall(Parser* parser, ASTNode* callee);
ASTNode* parseFunctionCallPratt(Parser* parser, ASTNode* callee);
ASTNode* parseCastExpressionPratt(Parser* parser, bool alreadyConsumedLParen);
ASTNode* parseSizeofExpressionPratt(Parser* parser);
ASTNode* parseAlignofExpressionPratt(Parser* parser);
ASTNode* parseCompoundLiteralPratt(Parser* parser, bool alreadyConsumedLParen);


#endif // _PARSER_EXPR_PRATT_H
