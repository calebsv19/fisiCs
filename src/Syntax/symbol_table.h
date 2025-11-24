#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "Parser/Helpers/parsed_type.h"
#include "AST/ast_node.h"
#include <stdbool.h>

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_TYPEDEF,
    SYMBOL_STRUCT,
    SYMBOL_ENUM
} SymbolKind;

typedef struct FunctionSignature {
    ParsedType* params;
    size_t paramCount;
    bool isVariadic;
} FunctionSignature;

typedef struct Symbol {
    char* name;
    SymbolKind kind;
    ParsedType type;
    ASTNode* definition;
    FunctionSignature signature;
    struct Symbol* next; // For linked list in hash buckets
} Symbol;

#define SYMBOL_TABLE_SIZE 128

typedef struct {
    Symbol* buckets[SYMBOL_TABLE_SIZE];
} SymbolTable;

unsigned int hashSymbolName(const char* name);
void initSymbolTable(SymbolTable* table);
bool insertSymbol(SymbolTable* table, Symbol* sym);
Symbol* lookupSymbol(SymbolTable* table, const char* name);
void freeSymbolTable(SymbolTable* table);

#endif // SYMBOL_TABLE_H
