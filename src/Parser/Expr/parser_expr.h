// SPDX-License-Identifier: Apache-2.0

#ifndef _PARSER_EXPR_H
#define _PARSER_EXPR_H

#include "Parser/parser.h"
#include "Parser/Helpers/parsed_type.h"

#include "AST/ast_node.h"

#include "Lexer/tokens.h"
#include "Lexer/lexer.h"

// Expressions and assignments
ASTNode* parseExpression(Parser* parser);
ASTNode* parseCommaExpression(Parser* parser);
ASTNode* parseCommaExpressionRecursive(Parser* parser);

ASTNode* parseAssignmentExpression(Parser* parser);
ASTNode* parseTernaryExpression(Parser* parser);
ASTNode* parseBooleanExpression(Parser* parser);
ASTNode* parseComparisonExpression(Parser* parser);
ASTNode* parseBitwiseOr(Parser* parser);
ASTNode* parseBitwiseXor(Parser* parser);
ASTNode* parseBitwiseAnd(Parser* parser);
ASTNode* parseBitwiseShift(Parser* parser);
ASTNode* parseAdditionSubtraction(Parser* parser);
ASTNode* parseMultiplication(Parser* parser);
ASTNode* parseUnaryOrCast(Parser* parser);
ASTNode* parseFactor(Parser* parser);
ASTNode* parseCastExpression(Parser* parser);
ASTNode* parsePostfixExpression(Parser* parser);
ASTNode* parsePrimary(Parser* parser);
ASTNode* parseSizeofExpression(Parser* parser, const Token* sizeofToken);
ASTNode* parseAlignofExpression(Parser* parser, const Token* alignofToken);

// Statement-level expression routing
ASTNode* handleExpressionOrAssignment(Parser* parser);

#endif // _PARSER_EXPR_H
