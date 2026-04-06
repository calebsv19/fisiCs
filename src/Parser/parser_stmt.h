// SPDX-License-Identifier: Apache-2.0

#ifndef _PARSER_STMT_H
#define _PARSER_STMT_H

#include "parser.h"
#include "Parser/Helpers/parsed_type.h"

#include "AST/ast_node.h"

#include "Lexer/tokens.h"
#include "Lexer/lexer.h"

// Control flow and jump statements
ASTNode* handleControlStatements(Parser* parser);

ASTNode* parseAssignment(Parser* parser);

ASTNode* parseIfStatement(Parser* parser);
ASTNode* parseWhileLoop(Parser* parser);
ASTNode* parseDoWhileLoop(Parser* parser);
ASTNode* parseForLoop(Parser* parser);
ASTNode* parseForLoopInitializer(Parser* parser);
ASTNode* parseForLoopCondition(Parser* parser);
ASTNode* parseForLoopIncrement(Parser* parser);

ASTNode* parseReturnStatement(Parser* parser);
ASTNode* parseBreakStatement(Parser* parser);
ASTNode* parseContinueStatement(Parser* parser);
ASTNode* parseGotoStatement(Parser* parser);

ASTNode* parseAsmStatement(Parser* parser);
ASTNode* parseLabel(Parser* parser);

void parserSyncToStatementEnd(Parser* parser);
void parserSyncToDeclarationStart(Parser* parser);

#endif // _PARSER_STMT_H
