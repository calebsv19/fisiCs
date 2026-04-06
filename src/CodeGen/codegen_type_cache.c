// SPDX-License-Identifier: Apache-2.0

#include "codegen_type_cache.h"

#include "codegen_model_adapter.h"

#include <stdlib.h>
#include <string.h>

static void freeTypedefCache(CGNamedLLVMType* cache, size_t count) {
    if (!cache) return;
    for (size_t i = 0; i < count; ++i) {
        free(cache[i].name);
    }
    free(cache);
}

static void freeStructCache(CGStructLLVMInfo* cache, size_t count) {
    if (!cache) return;
    for (size_t i = 0; i < count; ++i) {
        free(cache[i].name);
        if (cache[i].fields) {
            for (size_t j = 0; j < cache[i].fieldCount; ++j) {
                free(cache[i].fields[j].name);
            }
            free(cache[i].fields);
        }
    }
    free(cache);
}

CGTypeCache* cg_type_cache_create(const SemanticModel* model) {
    CGTypeCache* cache = (CGTypeCache*)calloc(1, sizeof(CGTypeCache));
    if (!cache) return NULL;
    cache->model = model;

    size_t typedefCount = codegenModelGetTypedefCount(model);
    if (typedefCount > 0) {
        cache->typedefCache = (CGNamedLLVMType*)calloc(typedefCount, sizeof(CGNamedLLVMType));
        if (!cache->typedefCache) {
            cg_type_cache_destroy(cache);
            return NULL;
        }
        cache->typedefCount = typedefCount;
        CGTypedefInfo* info = (CGTypedefInfo*)calloc(typedefCount, sizeof(CGTypedefInfo));
        if (!info) {
            cg_type_cache_destroy(cache);
            return NULL;
        }
        codegenModelFillTypedefInfo(model, info, typedefCount);
        for (size_t i = 0; i < typedefCount; ++i) {
            cache->typedefCache[i].name = info[i].name ? strdup(info[i].name) : NULL;
            cache->typedefCache[i].type = NULL;
            cache->typedefCache[i].parsedType = info[i].type;
            cache->typedefCache[i].resolving = false;
        }
        free(info);
    }

    size_t structCount = codegenModelGetStructCount(model);
    if (structCount > 0) {
        cache->structCache = (CGStructLLVMInfo*)calloc(structCount, sizeof(CGStructLLVMInfo));
        if (!cache->structCache) {
            cg_type_cache_destroy(cache);
            return NULL;
        }
        cache->structCount = structCount;
        CGStructInfo* infos = (CGStructInfo*)calloc(structCount, sizeof(CGStructInfo));
        if (!infos) {
            cg_type_cache_destroy(cache);
            return NULL;
        }
        codegenModelFillStructInfo(model, infos, structCount);
        for (size_t i = 0; i < structCount; ++i) {
            cache->structCache[i].name = infos[i].name ? strdup(infos[i].name) : NULL;
            cache->structCache[i].definition = infos[i].definition;
            cache->structCache[i].symbol = infos[i].symbol;
            cache->structCache[i].isUnion = infos[i].isUnion;
            cache->structCache[i].llvmType = NULL;
            cache->structCache[i].fieldCount = infos[i].fieldCount;
            cache->structCache[i].resolving = false;
            if (infos[i].fieldCount > 0) {
                cache->structCache[i].fields = (CGFieldNameIndex*)calloc(infos[i].fieldCount, sizeof(CGFieldNameIndex));
                if (!cache->structCache[i].fields) {
                    free(infos);
                    cg_type_cache_destroy(cache);
                    return NULL;
                }
                for (size_t j = 0; j < infos[i].fieldCount; ++j) {
                    cache->structCache[i].fields[j].name = infos[i].fields[j].name ? strdup(infos[i].fields[j].name) : NULL;
                    cache->structCache[i].fields[j].index = infos[i].isUnion ? 0 : (unsigned)j;
                    cache->structCache[i].fields[j].parsedType = infos[i].fields[j].type;
                }
            }
        }
        for (size_t i = 0; i < structCount; ++i) {
            if (infos[i].fields) {
                free((void*)infos[i].fields);
            }
        }
        free(infos);
    }

    return cache;
}

void cg_type_cache_destroy(CGTypeCache* cache) {
    if (!cache) return;
    freeTypedefCache(cache->typedefCache, cache->typedefCount);
    freeStructCache(cache->structCache, cache->structCount);
    free(cache);
}

static CGNamedLLVMType* findTypedef(CGTypeCache* cache, const char* name) {
    if (!cache || !name) return NULL;
    for (size_t i = 0; i < cache->typedefCount; ++i) {
        if (cache->typedefCache[i].name && strcmp(cache->typedefCache[i].name, name) == 0) {
            return &cache->typedefCache[i];
        }
    }
    return NULL;
}

LLVMTypeRef cg_type_cache_lookup_typedef(CGTypeCache* cache, const char* name) {
    CGNamedLLVMType* entry = findTypedef(cache, name);
    return entry ? entry->type : NULL;
}

static CGStructLLVMInfo* findStruct(CGTypeCache* cache, const char* name) {
    if (!cache || !name) return NULL;
    for (size_t i = 0; i < cache->structCount; ++i) {
        if (cache->structCache[i].name && strcmp(cache->structCache[i].name, name) == 0) {
            return &cache->structCache[i];
        }
    }
    return NULL;
}

LLVMTypeRef cg_type_cache_lookup_struct(CGTypeCache* cache, const char* name, bool* outIsUnion) {
    CGStructLLVMInfo* info = findStruct(cache, name);
    if (!info) return NULL;
    if (outIsUnion) *outIsUnion = info->isUnion;
    return info->llvmType;
}

unsigned cg_type_cache_lookup_field_index(CGTypeCache* cache, const char* structName, const char* fieldName, bool* outFound) {
    if (outFound) *outFound = false;
    CGStructLLVMInfo* info = findStruct(cache, structName);
    if (!info || !fieldName) return 0;
    for (size_t i = 0; i < info->fieldCount; ++i) {
        if (info->fields[i].name && strcmp(info->fields[i].name, fieldName) == 0) {
            if (outFound) *outFound = true;
            return info->fields[i].index;
        }
    }
    return 0;
}

CGStructLLVMInfo* cg_type_cache_get_struct_info(CGTypeCache* cache, const char* name) {
    return findStruct(cache, name);
}

CGNamedLLVMType* cg_type_cache_get_typedef_info(CGTypeCache* cache, const char* name) {
    return findTypedef(cache, name);
}

CGStructLLVMInfo* cg_type_cache_get_struct_by_definition(CGTypeCache* cache, const ASTNode* definition) {
    if (!cache || !definition) {
        return NULL;
    }
    for (size_t i = 0; i < cache->structCount; ++i) {
        if (cache->structCache[i].definition == definition) {
            return &cache->structCache[i];
        }
    }
    return NULL;
}

CGStructLLVMInfo* cg_type_cache_find_struct_by_llvm(CGTypeCache* cache, LLVMTypeRef llvmType) {
    if (!cache || !llvmType) {
        return NULL;
    }
    for (size_t i = 0; i < cache->structCount; ++i) {
        if (cache->structCache[i].llvmType == llvmType) {
            return &cache->structCache[i];
        }
    }
    return NULL;
}
