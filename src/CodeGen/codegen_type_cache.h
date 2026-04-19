// SPDX-License-Identifier: Apache-2.0

#ifndef CODEGEN_TYPE_CACHE_H
#define CODEGEN_TYPE_CACHE_H

#include "codegen_model_adapter.h"
#include "Syntax/semantic_model.h"
#include <llvm-c/Core.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "Parser/Helpers/parsed_type.h"

typedef struct CGNamedLLVMType {
    char* name;
    LLVMTypeRef type;
    ParsedType parsedType;
    bool resolving;
} CGNamedLLVMType;

typedef struct CGFieldNameIndex {
    char* name;
    unsigned index;
    ParsedType parsedType;
} CGFieldNameIndex;

typedef struct CGStructLLVMInfo {
    char* name;
    const ASTNode* definition;
    const Symbol* symbol;
    CGFieldNameIndex* fields;
    size_t fieldCount;
    bool isUnion;
    LLVMTypeRef llvmType;
    bool resolving;
} CGStructLLVMInfo;

typedef struct CGParsedTypeLLVM {
    char* key;
    LLVMTypeRef type;
} CGParsedTypeLLVM;

typedef struct CGTypeCache {
    const SemanticModel* model;
    CGNamedLLVMType* typedefCache;
    size_t typedefCount;
    CGStructLLVMInfo* structCache;
    size_t structCount;
    CGParsedTypeLLVM* parsedCache;
    size_t parsedCount;
} CGTypeCache;

CGTypeCache* cg_type_cache_create(const SemanticModel* model);
void cg_type_cache_destroy(CGTypeCache* cache);
LLVMTypeRef cg_type_cache_lookup_typedef(CGTypeCache* cache, const char* name);
LLVMTypeRef cg_type_cache_lookup_struct(CGTypeCache* cache, const char* name, bool* outIsUnion);
unsigned cg_type_cache_lookup_field_index(CGTypeCache* cache, const char* structName, const char* fieldName, bool* outFound);
CGStructLLVMInfo* cg_type_cache_get_struct_info(CGTypeCache* cache, const char* name);
CGNamedLLVMType* cg_type_cache_get_typedef_info(CGTypeCache* cache, const char* name);
CGStructLLVMInfo* cg_type_cache_get_struct_by_definition(CGTypeCache* cache, const ASTNode* definition);
CGStructLLVMInfo* cg_type_cache_find_struct_by_llvm(CGTypeCache* cache, LLVMTypeRef llvmType);
LLVMTypeRef cg_type_cache_lookup_parsed(CGTypeCache* cache, const char* key);
bool cg_type_cache_store_parsed(CGTypeCache* cache, const char* key, LLVMTypeRef type);

#ifdef __cplusplus
}
#endif

#endif // CODEGEN_TYPE_CACHE_H
