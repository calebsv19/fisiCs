#ifndef _PARSER_DECL_H
#define _PARSER_DECL_H

#include "parser.h"
#include "Parser/Helpers/parsed_type.h"

#include "AST/ast_node.h"

#include "Lexer/tokens.h"
#include "Lexer/lexer.h"

typedef struct PointerDeclaratorLayer {
    bool isConst;
    bool isVolatile;
    bool isRestrict;
} PointerDeclaratorLayer;

typedef struct PointerChain {
    PointerDeclaratorLayer* layers;
    size_t count;
} PointerChain;

// Type or function declaration dispatcher
ASTNode* handleTypeOrFunctionDeclaration(Parser* parser);

// Variable and array declarations
ASTNode* parseDeclaration(Parser* parser, ParsedType declaredType, size_t* varCount);
ASTNode* parseVariableDeclaration(Parser* parser, ParsedType declaredType, size_t* outVarCount);
ASTNode* parseArrayDeclaration(Parser* parser, ParsedType type);
ASTNode* parseDeclarationForLoop(Parser* parser);

// Struct, union, enum, typedef
ASTNode* parseStructDefinition(Parser* parser);
ASTNode* parseUnionDefinition(Parser* parser);
ASTNode** parseStructOrUnionFields(Parser* parser, size_t* outCount);
ASTNode* parseEnumDefinition(Parser* parser);
ASTNode* parseTypedef(Parser* parser);

// Struct helper routing
ASTNode* handleStructStatements(Parser* parser);

PointerChain parsePointerChain(Parser* parser);
void pointerChainFree(PointerChain* chain);
void applyPointerChainToType(ParsedType* type, const PointerChain* chain);
bool parserConsumeArraySuffixes(Parser* parser, ParsedType* type, ASTNode** outSizeChain);

#endif // _PARSER_DECL_H
