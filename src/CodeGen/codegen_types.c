#include "codegen_types.h"

#include "codegen_internal.h"
#include "codegen_type_cache.h"

/* Forward declaration to materialize inline struct/union definitions. */
LLVMTypeRef codegenStructDefinition(CodegenContext* ctx, ASTNode* node);

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Utils/logging.h"

#ifndef CG_DEBUG
#ifdef CODEGEN_DEBUG
#define CG_DEBUG(...) LOG_DEBUG("codegen", __VA_ARGS__)
#else
#define CG_DEBUG(...) ((void)0)
#endif
#endif

#ifndef CG_TRACE
#ifdef CODEGEN_TRACE
#define CG_TRACE(...) LOG_TRACE("codegen", __VA_ARGS__)
#else
#define CG_TRACE(...) ((void)0)
#endif
#endif


static LLVMTypeRef makeIntegerType(CodegenContext* ctx, unsigned bits) {
    LLVMContextRef llvmCtx = cg_context_get_llvm_context(ctx);
    switch (bits) {
        case 1:  return LLVMInt1TypeInContext(llvmCtx);
        case 8:  return LLVMInt8TypeInContext(llvmCtx);
        case 16: return LLVMInt16TypeInContext(llvmCtx);
        case 32: return LLVMInt32TypeInContext(llvmCtx);
        case 64: return LLVMInt64TypeInContext(llvmCtx);
        default: return LLVMIntTypeInContext(llvmCtx, bits ? bits : 32);
    }
}

static LLVMTypeRef primitiveType(CodegenContext* ctx, const ParsedType* type) {
    LLVMContextRef llvmCtx = cg_context_get_llvm_context(ctx);
    TokenType tok = type->primitiveType;

    if (type->isComplex || type->isImaginary) {
        LLVMTypeRef base = NULL;
        switch (tok) {
            case TOKEN_FLOAT:
                base = LLVMFloatTypeInContext(llvmCtx);
                break;
            case TOKEN_DOUBLE:
            default:
                if (type->isLong) {
                    const TargetLayout* tl = cg_context_get_target_layout(ctx);
                    if (tl && tl->longDoubleBits == 128) {
                        base = LLVMFP128TypeInContext(llvmCtx);
                    } else if (tl && tl->longDoubleBits == 64) {
                        base = LLVMDoubleTypeInContext(llvmCtx);
                    } else {
                        base = LLVMX86FP80TypeInContext(llvmCtx);
                    }
                } else {
                    base = LLVMDoubleTypeInContext(llvmCtx);
                }
                break;
        }
        if (!base) {
            base = LLVMDoubleTypeInContext(llvmCtx);
        }
        LLVMTypeRef parts[2] = { base, base };
        return LLVMStructTypeInContext(llvmCtx, parts, 2, false);
    }

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
            if (type->isLong) {
                const TargetLayout* tl = cg_context_get_target_layout(ctx);
                if (tl && tl->longDoubleBits == 128) {
                    return LLVMFP128TypeInContext(llvmCtx);
                }
                if (tl && tl->longDoubleBits == 64) {
                    return LLVMDoubleTypeInContext(llvmCtx);
                }
                LLVMTypeRef fp80 = LLVMX86FP80TypeInContext(llvmCtx);
                if (fp80 && (!tl || tl->longDoubleBits >= 80)) {
                    return fp80;
                }
            }
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
static LLVMTypeRef buildDerivedType(CodegenContext* ctx, const ParsedType* type, size_t derivationIndex);

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

static LLVMTypeRef buildDerivedType(CodegenContext* ctx, const ParsedType* type, size_t derivationIndex) {
    if (!type) {
        return LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
    }
    if (derivationIndex >= type->derivationCount) {
        return baseType(ctx, type);
    }

    const TypeDerivation* deriv = parsedTypeGetDerivation(type, derivationIndex);
    if (!deriv) {
        return baseType(ctx, type);
    }

    switch (deriv->kind) {
        case TYPE_DERIVATION_POINTER: {
            LLVMTypeRef target = buildDerivedType(ctx, type, derivationIndex + 1);
            if (!target || LLVMGetTypeKind(target) == LLVMVoidTypeKind) {
                target = LLVMInt8TypeInContext(cg_context_get_llvm_context(ctx));
            }
            return LLVMPointerType(target, 0);
        }
        case TYPE_DERIVATION_ARRAY: {
            LLVMTypeRef elementType = buildDerivedType(ctx, type, derivationIndex + 1);
            if (!elementType || LLVMGetTypeKind(elementType) == LLVMVoidTypeKind) {
                elementType = LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
            }
            if (deriv->as.array.isFlexible) {
                return LLVMArrayType(elementType, 0);
            }
            if (!deriv->as.array.isVLA && deriv->as.array.hasConstantSize && deriv->as.array.constantSize > 0) {
                unsigned len = (unsigned)deriv->as.array.constantSize;
                return LLVMArrayType(elementType, len);
            }
            return LLVMPointerType(elementType, 0);
        }
        case TYPE_DERIVATION_FUNCTION: {
            LLVMTypeRef returnType = buildDerivedType(ctx, type, derivationIndex + 1);
            size_t paramCount = deriv->as.function.paramCount;
            LLVMTypeRef* params = NULL;
            if (paramCount > 0) {
                params = (LLVMTypeRef*)calloc(paramCount, sizeof(LLVMTypeRef));
                if (!params) {
                    return LLVMFunctionType(returnType ? returnType : LLVMVoidTypeInContext(cg_context_get_llvm_context(ctx)),
                                            NULL,
                                            0,
                                            deriv->as.function.isVariadic ? 1 : 0);
                }
                for (size_t i = 0; i < paramCount; ++i) {
                    params[i] = cg_type_from_parsed(ctx, &deriv->as.function.params[i]);
                    if (!params[i]) {
                        params[i] = LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
                    }
                }
            }
            LLVMTypeRef fnType = LLVMFunctionType(returnType ? returnType : LLVMVoidTypeInContext(cg_context_get_llvm_context(ctx)),
                                                  params,
                                                  (unsigned)paramCount,
                                                  deriv->as.function.isVariadic ? 1 : 0);
            free(params);
            return fnType;
        }
    }

    return baseType(ctx, type);
}

LLVMTypeRef cg_type_from_parsed(CodegenContext* ctx, const ParsedType* type) {
    LLVMTypeRef derived = buildDerivedType(ctx, type, 0);
    if (!derived) {
        derived = baseType(ctx, type);
    }

    if (type && type->isFunctionPointer && type->derivationCount == 0) {
        LLVMTypeRef fnPtr = functionPointerType(ctx, type, derived);
        int extraDepthLegacy = type ? type->pointerDepth : 0;
        if (extraDepthLegacy > 0) {
            return applyPointerDepth(ctx, fnPtr, extraDepthLegacy);
        }
        return fnPtr;
    }

    int pointerDerivations = 0;
    if (type) {
        pointerDerivations = (int)parsedTypeCountDerivationsOfKind(type, TYPE_DERIVATION_POINTER);
    }
    int depth = type ? type->pointerDepth - pointerDerivations : 0;
    if (depth < 0) depth = 0;
    return applyPointerDepth(ctx, derived, depth);
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
        LLVMModuleRef module = cg_context_get_module(ctx);
        LLVMTargetDataRef td = module ? LLVMGetModuleDataLayout(module) : NULL;
        LLVMTypeRef maxAlignTy = NULL;
        uint64_t maxSize = 0;
        uint32_t maxAlign = 1;
        uint64_t maxAlignTySize = 0;

        for (size_t i = 0; i < info->fieldCount; ++i) {
            LLVMTypeRef fieldTy = cg_type_from_parsed(ctx, &info->fields[i].parsedType);
            if (!fieldTy || LLVMGetTypeKind(fieldTy) == LLVMVoidTypeKind) {
                fieldTy = LLVMInt8TypeInContext(cg_context_get_llvm_context(ctx));
            }
            uint64_t sz = td ? LLVMABISizeOfType(td, fieldTy) : 0;
            uint32_t al = td ? (uint32_t)LLVMABIAlignmentOfType(td, fieldTy) : 1;
            if (al == 0) al = 1;
            if (sz > maxSize) maxSize = sz;
            if (!maxAlignTy || al > maxAlign || (al == maxAlign && sz > maxAlignTySize)) {
                maxAlignTy = fieldTy;
                maxAlign = al;
                maxAlignTySize = sz;
            }
        }

        if (!maxAlignTy) {
            maxAlignTy = LLVMInt8TypeInContext(cg_context_get_llvm_context(ctx));
            maxAlign = 1;
            maxSize = 1;
            maxAlignTySize = 1;
        }

        uint64_t finalSize = maxSize;
        if (maxAlign > 1) {
            uint64_t rem = finalSize % (uint64_t)maxAlign;
            if (rem != 0) finalSize += ((uint64_t)maxAlign - rem);
        }
        if (finalSize == 0) finalSize = 1;

        if (maxAlignTySize >= finalSize) {
            LLVMStructSetBody(info->llvmType, &maxAlignTy, 1, 0);
            return info->llvmType;
        }

        uint64_t tailBytes = finalSize - maxAlignTySize;
        LLVMTypeRef members[2];
        members[0] = maxAlignTy;
        members[1] = LLVMArrayType(LLVMInt8TypeInContext(cg_context_get_llvm_context(ctx)),
                                   (unsigned)tailBytes);
        LLVMStructSetBody(info->llvmType, members, 2, 0);
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
    const char* typeName = (type && type->userTypeName) ? type->userTypeName : "<anon>";
    CG_TRACE("[TRACE] structOrUnionType name=%s kind=%d\n", typeName, type ? type->kind : -1);

    /* If we have the defining AST handy (inline def), force materialization before fallback. */
    if (ctx && type && type->inlineStructOrUnionDef) {
        LLVMTypeRef forced = codegenStructDefinition(ctx, type->inlineStructOrUnionDef);
        if (forced) {
            return forced;
        }
    }

    if (!ctx || !type || !type->userTypeName) {
        LLVMTypeRef fallback = LLVMPointerType(LLVMInt8TypeInContext(cg_context_get_llvm_context(ctx)), 0);
        CG_TRACE("[TRACE] structOrUnionType returning fallback=%p\n", (void*)fallback);
        return fallback;
    }

    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    if (cache) {
        CGStructLLVMInfo* info = cg_type_cache_get_struct_info(cache, typeName);
        if (info) {
            if (info->resolving) {
                CG_TRACE("[TRACE] structOrUnionType hit resolving true\n");
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
                CG_TRACE("[TRACE] structOrUnionType populating llvmType\n");
                LLVMTypeRef existing = cg_context_lookup_named_type(ctx, typeName);
                if (existing) {
                    info->llvmType = existing;
                }
            }

            info->resolving = true;
            LLVMTypeRef resolved = ensureStructFromInfo(ctx, info);
            info->resolving = false;
            if (resolved) {
                CG_TRACE("[TRACE] structOrUnionType resolved=%p\n", (void*)resolved);
                return resolved;
            }
        }
    }

    LLVMTypeRef cached = cg_context_lookup_named_type(ctx, typeName);
    if (cached) {
        CG_TRACE("[TRACE] structOrUnionType hit cached=%p\n", (void*)cached);
        return cached;
    }

    LLVMTypeRef created = LLVMStructCreateNamed(cg_context_get_llvm_context(ctx), typeName);
    cg_context_cache_named_type(ctx, typeName, created);
    CG_TRACE("[TRACE] structOrUnionType created=%p\n", (void*)created);
    return created;
}



static LLVMTypeRef namedAliasType(CodegenContext* ctx, const ParsedType* type) {
    const char* aliasName = (type && type->userTypeName) ? type->userTypeName : "<anon>";
    CG_TRACE("[TRACE] namedAliasType name=%s kind=%d\n", aliasName, type ? type->kind : -1);

    if (!ctx || !type || !type->userTypeName) {
        LLVMTypeRef fallback = LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
        CG_TRACE("[TRACE] namedAliasType returning fallback=%p\n", (void*)fallback);
        return fallback;
    }

    if (strcmp(aliasName, "_Float16") == 0 || strcmp(aliasName, "__fp16") == 0) {
        LLVMTypeRef halfTy = LLVMHalfTypeInContext(cg_context_get_llvm_context(ctx));
        CG_TRACE("[TRACE] namedAliasType builtin half=%p\n", (void*)halfTy);
        return halfTy;
    }
    if (strcmp(aliasName, "__builtin_va_list") == 0) {
        LLVMTypeRef vaListTy = LLVMPointerType(LLVMInt8TypeInContext(cg_context_get_llvm_context(ctx)), 0);
        CG_TRACE("[TRACE] namedAliasType builtin va_list=%p\n", (void*)vaListTy);
        return vaListTy;
    }

    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    if (cache) {
        CGNamedLLVMType* info = cg_type_cache_get_typedef_info(cache, aliasName);
        if (info) {
            if (info->type && !info->resolving) {
                CG_TRACE("[TRACE] namedAliasType returning cached=%p\n", (void*)info->type);
                return info->type;
            }

            if (info->resolving) {
                CG_TRACE("[TRACE] namedAliasType hit resolving true\n");
                if (info->type) {
                    CG_TRACE("[TRACE] namedAliasType returning in-flight type=%p\n", (void*)info->type);
                    return info->type;
                }
                LLVMTypeRef forward = cg_context_lookup_named_type(ctx, aliasName);
                if (forward) {
                    CG_TRACE("[TRACE] namedAliasType returning forward=%p\n", (void*)forward);
                    return forward;
                }
                LLVMTypeRef fallback = LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
                CG_TRACE("[TRACE] namedAliasType returning fallback=%p\n", (void*)fallback);
                return fallback;
            }

            info->resolving = true;
            const ParsedType* aliased = &info->parsedType;
            LLVMTypeRef resolved = NULL;
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
            CG_TRACE("[TRACE] namedAliasType resolved=%p\n", (void*)resolved);
            return resolved;
        }
    }

    LLVMTypeRef fallback = structOrUnionType(ctx, type);
    CG_TRACE("[TRACE] namedAliasType fallback struct=%p\n", (void*)fallback);
    return fallback;
}
