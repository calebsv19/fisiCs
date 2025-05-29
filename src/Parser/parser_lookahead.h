#ifndef _PARSER_LOOKAHEAD_H
#define _PARSER_LOOKAHEAD_H

#include "parser.h"
#include "parsed_type.h"

#include "AST/ast_node.h"

#include "Lexer/tokens.h"
#include "Lexer/lexer.h"

void printParserState(const char* label, Parser* parser);


// Predictive lookahead checks
bool looksLikeTypeDeclaration(Parser* parser);
bool looksLikeCastType(Parser* parser);
bool looksLikeFunctionPointerDeclaration(Parser* parser);

#endif // _PARSER_LOOKAHEAD_H

