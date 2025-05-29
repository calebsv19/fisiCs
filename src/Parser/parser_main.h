#ifndef _PARSER_MAIN_H
#define _PARSER_MAIN_H

#include "parser.h"
#include "parsed_type.h"

#include "AST/ast_node.h"
#include "Lexer/tokens.h"
#include "Lexer/lexer.h"



// Root entrypoint for parsing the entire program
ASTNode* parse(Parser* parser);

// Top-level blocks
ASTNode* parseProgram(Parser* parser);
ASTNode* parseBlock(Parser* parser);
ASTNode* parseStatement(Parser* parser);

#endif // _PARSER_MAIN_H

