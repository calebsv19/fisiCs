// SPDX-License-Identifier: Apache-2.0

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "Parser/Helpers/parsed_type.h"
#include "AST/ast_node.h"
#include <stdbool.h>

struct TypeInfo;

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_TYPEDEF,
    SYMBOL_STRUCT,
    SYMBOL_ENUM
} SymbolKind;

typedef enum {
    STORAGE_NONE,
    STORAGE_EXTERN,
    STORAGE_STATIC,
    STORAGE_REGISTER,
    STORAGE_AUTO
} StorageClass;

typedef enum {
    LINKAGE_NONE,
    LINKAGE_INTERNAL,
    LINKAGE_EXTERNAL
} SymbolLinkage;

typedef struct FunctionSignature {
    ParsedType* params;
    size_t paramCount;
    bool isVariadic;
    bool hasPrototype;
    /* Optional interop metadata */
    enum {
        CALLCONV_DEFAULT = 0,
        CALLCONV_STDCALL,
        CALLCONV_FASTCALL,
        CALLCONV_CDECL
    } callConv;
} FunctionSignature;

typedef enum {
    DLL_STORAGE_NONE = 0,
    DLL_STORAGE_IMPORT,
    DLL_STORAGE_EXPORT
} DllStorageClass;

typedef struct Symbol {
    char* name;
    SymbolKind kind;
    ParsedType type;
    ASTNode* definition;
    FunctionSignature signature;
    StorageClass storage;
    SymbolLinkage linkage;
    DllStorageClass dllStorage;
    bool hasDefinition;
    bool isTentative;
    bool hasConstValue;
    long long constValue;
    struct TypeInfo* cachedTypeInfo;
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
