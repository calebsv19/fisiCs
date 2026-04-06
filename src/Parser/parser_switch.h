// SPDX-License-Identifier: Apache-2.0

#ifndef _PARSER_SWITCH_H
#define _PARSER_SWITCH_H

#include "parser.h"
#include "Parser/Helpers/parsed_type.h"

#include "AST/ast_node.h"

#include "Lexer/tokens.h"
#include "Lexer/lexer.h"


// switch-case parser
ASTNode* parseSwitchStatement(Parser* parser);

// helper for linking fallthrough statements
void linkStatements(ASTNode* caseBody, ASTNode* stmt);

#endif // _PARSER_SWITCH_H

