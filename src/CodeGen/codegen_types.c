#include "codegen_types.h"

#include "codegen_internal.h"
#include "codegen_type_cache.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static LLVMTypeRef makeIntegerType(CodegenContext* ctx, unsigned bits) {
    LLVMContextRef llvmCtx = cg_context_get_llvm_context(ctx);
    switch (bits) {
        case 1:  return LLVMInt1TypeInContext(llvmCtx);
        case 8:  return LLVMInt8TypeInContext(llvmCtx);
        case 16: return LLVMInt16TypeInContext(llvmCtx);
        case 32: return LLVMInt32TypeInContext(llvmCtx);
        case 64: return LLVMInt64TypeInContext(llvmCtx);
        default: return LLVMInt32TypeInContext(llvmCtx);
    }
}

static LLVMTypeRef primitiveType(CodegenContext* ctx, const ParsedType* type) {
    LLVMContextRef llvmCtx = cg_context_get_llvm_context(ctx);
    TokenType tok = type->primitiveType;

    switch (tok) {
        case TOKEN_VOID:
            return LLVMVoidTypeInContext(llvmCtx);
        case TOKEN_CHAR:
            return makeIntegerType(ctx, 8);
        case TOKEN_BOOL:
            return makeIntegerType(ctx, 1);
        case TOKEN_SHORT:
            return makeIntegerType(ctx, 16);
        case TOKEN_LONG:
            return makeIntegerType(ctx, 64);
        case TOKEN_FLOAT:
            return LLVMFloatTypeInContext(llvmCtx);
        case TOKEN_DOUBLE:
            return LLVMDoubleTypeInContext(llvmCtx);
        case TOKEN_INT:
        case TOKEN_SIGNED:
        case TOKEN_UNSIGNED:
        default: {
            if (type->isShort) return makeIntegerType(ctx, 16);
            if (type->isLong)  return makeIntegerType(ctx, 64);
            return makeIntegerType(ctx, 32);
        }
    }
}

static LLVMTypeRef ensureStructFromInfo(CodegenContext* ctx, CGStructLLVMInfo* info);
static LLVMTypeRef structOrUnionType(CodegenContext* ctx, const ParsedType* type);
static LLVMTypeRef namedAliasType(CodegenContext* ctx, const ParsedType* type);

static LLVMTypeRef baseType(CodegenContext* ctx, const ParsedType* type) {
    if (!type) {
        return LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
    }

    switch (type->kind) {
        case TYPE_PRIMITIVE:
            return primitiveType(ctx, type);
        case TYPE_ENUM:
            return makeIntegerType(ctx, 32);
        case TYPE_STRUCT:
        case TYPE_UNION:
            return structOrUnionType(ctx, type);
        case TYPE_NAMED:
            return namedAliasType(ctx, type);
        case TYPE_INVALID:
        default:
            return LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
    }
}

static LLVMTypeRef applyPointerDepth(CodegenContext* ctx, LLVMTypeRef base, int depth) {
    LLVMContextRef llvmCtx = cg_context_get_llvm_context(ctx);
    LLVMTypeRef pointer = base;

    if (LLVMGetTypeKind(base) == LLVMVoidTypeKind && depth > 0) {
        pointer = LLVMInt8TypeInContext(llvmCtx);
    }

    for (int i = 0; i < depth; ++i) {
        pointer = LLVMPointerType(pointer, 0);
    }
    return pointer;
}

static LLVMTypeRef functionPointerType(CodegenContext* ctx, const ParsedType* type, LLVMTypeRef returnType) {
    size_t count = type->fpParamCount;
    LLVMTypeRef* params = NULL;
    if (count > 0) {
        params = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * count);
        if (!params) return LLVMPointerType(returnType, 0);
        for (size_t i = 0; i < count; ++i) {
            params[i] = cg_type_from_parsed(ctx, &type->fpParams[i]);
            if (!params[i]) {
                params[i] = LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
            }
        }
    }

    LLVMTypeRef fnType = LLVMFunctionType(returnType, params, (unsigned)count, 0);
    free(params);
    return LLVMPointerType(fnType, 0);
}

LLVMTypeRef cg_type_from_parsed(CodegenContext* ctx, const ParsedType* type) {
    LLVMTypeRef base = baseType(ctx, type);

    if (type && type->isFunctionPointer) {
        LLVMTypeRef fnPtr = functionPointerType(ctx, type, base);
        int extraDepth = type ? type->pointerDepth : 0;
        if (extraDepth > 0) {
            return applyPointerDepth(ctx, fnPtr, extraDepth);
        }
        return fnPtr;
    }

    int depth = type ? type->pointerDepth : 0;
    return applyPointerDepth(ctx, base, depth);
}

static LLVMTypeRef ensureStructFromInfo(CodegenContext* ctx, CGStructLLVMInfo* info) {
    if (!ctx || !info || !info->name) {
        return LLVMPointerType(LLVMInt8TypeInContext(cg_context_get_llvm_context(ctx)), 0);
    }

    if (info->llvmType) {
        return info->llvmType;
    }

    LLVMTypeRef existing = cg_context_lookup_named_type(ctx, info->name);
    if (existing) {
        info->llvmType = existing;
    } else {
        info->llvmType = LLVMStructCreateNamed(cg_context_get_llvm_context(ctx), info->name);
        cg_context_cache_named_type(ctx, info->name, info->llvmType);
    }

    if (info->fieldCount == 0) {
        LLVMStructSetBody(info->llvmType, NULL, 0, 0);
        return info->llvmType;
    }

    if (info->isUnion) {
        LLVMTypeRef fieldType = cg_type_from_parsed(ctx, &info->fields[0].parsedType);
        LLVMStructSetBody(info->llvmType, &fieldType, 1, 0);
        return info->llvmType;
    }

    LLVMTypeRef* fieldTypes = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * info->fieldCount);
    if (!fieldTypes) {
        return info->llvmType;
    }
    for (size_t i = 0; i < info->fieldCount; ++i) {
        fieldTypes[i] = cg_type_from_parsed(ctx, &info->fields[i].parsedType);
        if (!fieldTypes[i]) {
            fieldTypes[i] = LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
        }
    }
    LLVMStructSetBody(info->llvmType, fieldTypes, (unsigned)info->fieldCount, 0);
    free(fieldTypes);
    return info->llvmType;
}

static LLVMTypeRef structOrUnionType(CodegenContext* ctx, const ParsedType* type) {
    static int depth = 0;
    ++depth;
    const char* typeNameForLog = (type && type->userTypeName) ? type->userTypeName : "<anon>";
    fprintf(stderr, "[TRACE] structOrUnionType depth=%d name=%s kind=%d\n",
            depth, typeNameForLog, type ? type->kind : -1);

    if (!ctx || !type || !type->userTypeName) {
        LLVMTypeRef fallback = LLVMPointerType(LLVMInt8TypeInContext(cg_context_get_llvm_context(ctx)), 0);
        fprintf(stderr, "[TRACE] structOrUnionType depth=%d name=%s returning fallback=%p\n",
                depth, typeNameForLog, (void*)fallback);
        --depth;
        return fallback;
    }

    const char* typeName = type->userTypeName;
    CGTypeCache* cache = cg_context_get_type_cache(ctx);

    if (cache) {
        CGStructLLVMInfo* info = cg_type_cache_get_struct_info(cache, typeName);
        if (info) {
            if (info->resolving) {
                fprintf(stderr, "[TRACE] structOrUnionType depth=%d name=%s hit resolving true\n",
                        depth, typeNameForLog);
                if (!info->llvmType) {
                    LLVMTypeRef existing = cg_context_lookup_named_type(ctx, typeName);
                    if (existing) {
                        info->llvmType = existing;
                    } else {
                        info->llvmType = LLVMStructCreateNamed(cg_context_get_llvm_context(ctx), typeName);
                        cg_context_cache_named_type(ctx, typeName, info->llvmType);
                    }
                }
                return info->llvmType;
            }

            if (!info->llvmType) {
                fprintf(stderr, "[TRACE] structOrUnionType depth=%d name=%s populating llvmType\n",
                        depth, typeNameForLog);
                LLVMTypeRef existing = cg_context_lookup_named_type(ctx, typeName);
                if (existing) {
                    info->llvmType = existing;
                }
            }

            info->resolving = true;
            LLVMTypeRef resolved = ensureStructFromInfo(ctx, info);
            info->resolving = false;
            if (resolved) {
                fprintf(stderr, "[TRACE] structOrUnionType depth=%d name=%s resolved=%p\n",
                        depth, typeNameForLog, (void*)resolved);
                --depth;
                return resolved;
            }
        }
    }

    LLVMTypeRef cached = cg_context_lookup_named_type(ctx, typeName);
    if (cached) {
        fprintf(stderr, "[TRACE] structOrUnionType depth=%d name=%s hit cached=%p\n",
                depth, typeNameForLog, (void*)cached);
        --depth;
        return cached;
    }

    LLVMTypeRef created = LLVMStructCreateNamed(cg_context_get_llvm_context(ctx), typeName);
    cg_context_cache_named_type(ctx, typeName, created);
    fprintf(stderr, "[TRACE] structOrUnionType depth=%d name=%s created=%p\n",
            depth, typeNameForLog, (void*)created);
    --depth;
    return created;
}

static LLVMTypeRef namedAliasType(CodegenContext* ctx, const ParsedType* type) {
    static int depth = 0;
    ++depth;
    const char* aliasLog = (type && type->userTypeName) ? type->userTypeName : "<anon>";
    fprintf(stderr, "[TRACE] namedAliasType depth=%d name=%s kind=%d\n",
            depth, aliasLog, type ? type->kind : -1);

    if (!ctx || !type || !type->userTypeName) {
        LLVMTypeRef fallback = LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
        fprintf(stderr, "[TRACE] namedAliasType depth=%d name=%s returning fallback=%p\n",
                depth, aliasLog, (void*)fallback);
        --depth;
        return fallback;
    }

    const char* aliasName = type->userTypeName;
    CGTypeCache* cache = cg_context_get_type_cache(ctx);

    if (cache) {
        CGNamedLLVMType* info = cg_type_cache_get_typedef_info(cache, aliasName);
        if (info) {
            if (info->type && !info->resolving) {
                fprintf(stderr, "[TRACE] namedAliasType depth=%d name=%s returning cached=%p\n",
                        depth, aliasLog, (void*)info->type);
                --depth;
                return info->type;
            }

            if (info->resolving) {
                fprintf(stderr, "[TRACE] namedAliasType depth=%d name=%s hit resolving true\n",
                        depth, aliasLog);
                if (info->type) {
                    fprintf(stderr, "[TRACE] namedAliasType depth=%d name=%s returning in-flight type=%p\n",
                            depth, aliasLog, (void*)info->type);
                    --depth;
                    return info->type;
                }
                LLVMTypeRef forward = cg_context_lookup_named_type(ctx, aliasName);
                if (forward) {
                    fprintf(stderr, "[TRACE] namedAliasType depth=%d name=%s returning forward=%p\n",
                            depth, aliasLog, (void*)forward);
                    --depth;
                    return forward;
                }
                LLVMTypeRef fallback = LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
                fprintf(stderr, "[TRACE] namedAliasType depth=%d name=%s returning fallback=%p\n",
                        depth, aliasLog, (void*)fallback);
                --depth;
                return fallback;
            }

            info->resolving = true;

            LLVMTypeRef resolved = NULL;
            const ParsedType* aliased = &info->parsedType;
            bool selfTagAlias =
                aliased &&
                aliased->kind == TYPE_NAMED &&
                aliased->userTypeName &&
                strcmp(aliased->userTypeName, aliasName) == 0 &&
                aliased->tag != TAG_NONE;

            if (selfTagAlias) {
                resolved = structOrUnionType(ctx, aliased);
            } else if (aliased) {
                resolved = cg_type_from_parsed(ctx, aliased);
                if (!resolved && aliased->tag != TAG_NONE) {
                    resolved = structOrUnionType(ctx, aliased);
                }
            }

            if (!resolved) {
                resolved = LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
            }

            info->type = resolved;
            info->resolving = false;
            fprintf(stderr, "[TRACE] namedAliasType depth=%d name=%s resolved=%p\n",
                    depth, aliasLog, (void*)resolved);
            --depth;
            return resolved;
        }
    }

    LLVMTypeRef fallback = structOrUnionType(ctx, type);
    fprintf(stderr, "[TRACE] namedAliasType depth=%d name=%s fallback struct=%p\n",
            depth, aliasLog, (void*)fallback);
    --depth;
    return fallback;
}
