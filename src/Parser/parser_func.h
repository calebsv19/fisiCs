#ifndef _PARSER_FUNC_H
#define _PARSER_FUNC_H

#include "parser.h"
#include "Parser/Helpers/parsed_type.h"

#include "AST/ast_node.h"

#include "Lexer/tokens.h"
#include "Lexer/lexer.h"

// Function definitions and declarations
ASTNode* parseFunctionDefinition(Parser* parser, ParsedType returnType);
ASTNode** parseParameterList(Parser* parser, size_t* paramCount);

// Function calls and pointers
ASTNode* parseFunctionCall(Parser* parser, ASTNode* callee);
ASTNode* parseFunctionPointerDeclaration(Parser* parser);

#endif // _PARSER_FUNC_H

