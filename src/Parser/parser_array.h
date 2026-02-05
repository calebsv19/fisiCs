#ifndef _PARSER_ARRAY_H
#define _PARSER_ARRAY_H

#include "parser.h"
#include "Parser/Helpers/parsed_type.h"

#include "AST/ast_node.h"

#include "Lexer/tokens.h"
#include "Lexer/lexer.h"


// Initializer list (used for both arrays and structs)
DesignatedInit** parseInitializerList(Parser* parser, ParsedType type, size_t* outCount);

// Array declaration helpers
ASTNode* parseArraySize(Parser* parser, bool* outIsVLA, ParsedArrayInfo* outInfo);
DesignatedInit** parseArrayInitializer(Parser* parser, ParsedType parentType, size_t* valueCount);
DesignatedInit** parseStructInitializer(Parser* parser, ParsedType parentType, size_t* outCount);

// Runtime access
ASTNode* parseArrayAccess(Parser* parser, ASTNode* array);

#endif // _PARSER_ARRAY_H
