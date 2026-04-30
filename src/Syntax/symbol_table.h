// SPDX-License-Identifier: Apache-2.0

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "Parser/Helpers/parsed_type.h"
#include "AST/ast_node.h"
#include <stdbool.h>

struct TypeInfo;
typedef struct FisicsUnitsAnnotation FisicsUnitsAnnotation;

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

typedef enum {
    CALLCONV_DEFAULT = 0,
    CALLCONV_STDCALL,
    CALLCONV_FASTCALL,
    CALLCONV_CDECL
} FunctionCallConv;

typedef struct FunctionSignature {
    ParsedType* params;
    size_t paramCount;
    bool isVariadic;
    bool hasPrototype;
    /* Optional interop metadata */
    FunctionCallConv callConv;
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
    struct DesignatedInit* initializer;
    FunctionSignature signature;
    StorageClass storage;
    SymbolLinkage linkage;
    DllStorageClass dllStorage;
    bool hasDefinition;
    bool isTentative;
    bool hasConstValue;
    long long constValue;
    struct TypeInfo* cachedTypeInfo;
    const FisicsUnitsAnnotation* unitsAnnotation;
    size_t unitsDeclaratorIndex;
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
void symbolAttachUnitsAnnotation(Symbol* sym,
                                 const FisicsUnitsAnnotation* annotation,
                                 size_t declaratorIndex);
const FisicsUnitsAnnotation* symbolGetUnitsAnnotation(const Symbol* sym);
size_t symbolGetUnitsDeclaratorIndex(const Symbol* sym);

#endif // SYMBOL_TABLE_H
