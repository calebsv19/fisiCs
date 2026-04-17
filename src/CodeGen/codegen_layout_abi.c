// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"

#include "codegen_types.h"
#include "Compiler/compiler_context.h"
#include "Syntax/layout.h"

#include <llvm-c/Core.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool cg_parsed_type_has_pointer_layer(const ParsedType* type) {
    if (!type) return false;
    if (type->isFunctionPointer) return true;
    if (type->pointerDepth > 0) return true;
    for (size_t i = 0; i < type->derivationCount; ++i) {
        const TypeDerivation* deriv = parsedTypeGetDerivation(type, i);
        if (deriv && deriv->kind == TYPE_DERIVATION_POINTER) {
            return true;
        }
    }
    return false;
}

static bool cg_named_type_has_surface_derivations(const ParsedType* type) {
    if (!type || type->kind != TYPE_NAMED) {
        return false;
    }
    return type->derivationCount > 0 || type->pointerDepth > 0 || type->isFunctionPointer;
}

static const ParsedType* cg_resolve_typedef_parsed(CodegenContext* ctx, const ParsedType* type) {
    if (!ctx || !type || type->kind != TYPE_NAMED || !type->userTypeName) {
        return type;
    }
    if (cg_named_type_has_surface_derivations(type)) {
        return type;
    }
    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    if (!cache) return type;
    CGNamedLLVMType* info = cg_type_cache_get_typedef_info(cache, type->userTypeName);
    if (info) {
        return &info->parsedType;
    }
    {
        const SemanticModel* model = cg_context_get_semantic_model(ctx);
        if (model) {
            const Symbol* sym = semanticModelLookupGlobal(model, type->userTypeName);
            if (sym && sym->kind == SYMBOL_TYPEDEF) {
                return &sym->type;
            }
        }
    }
    return type;
}

static const ParsedType* cg_resolve_typedef_chain(CodegenContext* ctx, const ParsedType* type) {
    const ParsedType* current = type;
    int guard = 0;
    while (current &&
           current->kind == TYPE_NAMED &&
           !cg_named_type_has_surface_derivations(current) &&
           guard < 64) {
        const ParsedType* next = cg_resolve_typedef_parsed(ctx, current);
        if (!next || next == current) {
            break;
        }
        current = next;
        guard++;
    }
    return current ? current : type;
}

static const ParsedType* cg_resolve_call_surface_type(CodegenContext* ctx, const ParsedType* type) {
    if (cg_named_type_has_surface_derivations(type)) {
        return type;
    }
    return cg_resolve_typedef_chain(ctx, type);
}

static bool cg_named_type_is_builtin_int128(const ParsedType* type, bool* outIsUnsigned) {
    if (!type || type->kind != TYPE_NAMED || !type->userTypeName) {
        return false;
    }
    if (strcmp(type->userTypeName, "__int128_t") == 0 ||
        strcmp(type->userTypeName, "__int128") == 0) {
        if (outIsUnsigned) *outIsUnsigned = false;
        return true;
    }
    if (strcmp(type->userTypeName, "__uint128_t") == 0 ||
        strcmp(type->userTypeName, "__uint128") == 0) {
        if (outIsUnsigned) *outIsUnsigned = true;
        return true;
    }
    return false;
}

static bool cg_parsed_type_is_unsigned_for_index(CodegenContext* ctx, const ParsedType* type) {
    if (!type) return false;
    const ParsedType* resolved = cg_resolve_typedef_chain(ctx, type);
    if (!resolved) {
        resolved = type;
    }
    switch (resolved->kind) {
        case TYPE_PRIMITIVE:
            if (resolved->primitiveType == TOKEN_BOOL || resolved->primitiveType == TOKEN_UNSIGNED) {
                return true;
            }
            return resolved->isUnsigned;
        case TYPE_NAMED: {
            bool int128Unsigned = false;
            if (cg_named_type_is_builtin_int128(resolved, &int128Unsigned)) {
                return int128Unsigned;
            }
            return resolved->isUnsigned;
        }
        default:
            return resolved->isUnsigned;
    }
}

static bool cg_parsed_type_is_direct_function_for_param(const ParsedType* type) {
    return type &&
           type->derivationCount > 0 &&
           type->derivations &&
           type->derivations[0].kind == TYPE_DERIVATION_FUNCTION;
}

static void cg_adjust_parameter_type_for_lowering(ParsedType* type) {
    if (!type) return;
    parsedTypeAdjustArrayParameter(type);
    if (!cg_parsed_type_is_direct_function_for_param(type)) {
        return;
    }
    TypeDerivation* grown = realloc(type->derivations, (type->derivationCount + 1) * sizeof(TypeDerivation));
    if (!grown) {
        return;
    }
    type->derivations = grown;
    memmove(type->derivations + 1, type->derivations, type->derivationCount * sizeof(TypeDerivation));
    memset(&type->derivations[0], 0, sizeof(TypeDerivation));
    type->derivations[0].kind = TYPE_DERIVATION_POINTER;
    type->derivations[0].as.pointer.isConst = false;
    type->derivations[0].as.pointer.isVolatile = false;
    type->derivations[0].as.pointer.isRestrict = false;
    type->derivationCount++;
    type->pointerDepth += 1;
    type->directlyDeclaresFunction = false;
}

static bool cg_abi_return_contains_fp_or_vector(LLVMTypeRef type, unsigned depth) {
    if (!type || depth > 8u) return false;
    switch (LLVMGetTypeKind(type)) {
        case LLVMHalfTypeKind:
        case LLVMFloatTypeKind:
        case LLVMDoubleTypeKind:
        case LLVMX86_FP80TypeKind:
        case LLVMFP128TypeKind:
        case LLVMPPC_FP128TypeKind:
        case LLVMVectorTypeKind:
        case LLVMScalableVectorTypeKind:
            return true;
        case LLVMArrayTypeKind:
            return cg_abi_return_contains_fp_or_vector(LLVMGetElementType(type), depth + 1u);
        case LLVMStructTypeKind: {
            unsigned count = LLVMCountStructElementTypes(type);
            if (count == 0) {
                return false;
            }
            LLVMTypeRef* elems = (LLVMTypeRef*)calloc(count, sizeof(LLVMTypeRef));
            if (!elems) {
                return true;
            }
            LLVMGetStructElementTypes(type, elems);
            bool hasFloat = false;
            for (unsigned i = 0; i < count; ++i) {
                if (cg_abi_return_contains_fp_or_vector(elems[i], depth + 1u)) {
                    hasFloat = true;
                    break;
                }
            }
            free(elems);
            return hasFloat;
        }
        default:
            return false;
    }
}

static bool cg_pointer_elem_size(CodegenContext* ctx,
                                 const ParsedType* parsed,
                                 LLVMTypeRef llvmHint,
                                 uint64_t* outSize,
                                 uint32_t* outAlign) {
    uint64_t semanticSize = 0;
    uint32_t semanticAlign = 0;
    bool haveSemantic = cg_size_align_of_parsed(ctx, parsed, &semanticSize, &semanticAlign);

    uint64_t llvmSize = 0;
    uint32_t llvmAlign = 0;
    bool haveLLVM = false;
    LLVMTargetDataRef tdata = ctx && ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
    if (tdata && llvmHint && LLVMTypeIsSized(llvmHint)) {
        llvmSize = LLVMABISizeOfType(tdata, llvmHint);
        llvmAlign = (uint32_t)LLVMABIAlignmentOfType(tdata, llvmHint);
        haveLLVM = true;
    }

    if (haveLLVM && haveSemantic &&
        (semanticSize != llvmSize || (semanticAlign && llvmAlign && semanticAlign != llvmAlign))) {
        if (getenv("FISICS_DEBUG_LAYOUT")) {
            fprintf(stderr,
                    "[layout] pointer elem size prefers LLVM: sz=%llu(%llu) align=%u(%u)\n",
                    (unsigned long long)semanticSize,
                    (unsigned long long)llvmSize,
                    semanticAlign,
                    llvmAlign);
        }
        if (outSize) *outSize = llvmSize;
        if (outAlign) *outAlign = llvmAlign ? llvmAlign : 1;
        return true;
    }

    if (haveSemantic) {
        if (outSize) *outSize = semanticSize;
        if (outAlign) *outAlign = semanticAlign ? semanticAlign : (haveLLVM ? llvmAlign : 1);
        return true;
    }

    if (haveLLVM) {
        if (outSize) *outSize = llvmSize;
        if (outAlign) *outAlign = llvmAlign ? llvmAlign : 1;
        return true;
    }

    return false;
}

static LLVMValueRef cg_build_vla_array_size_bytes(CodegenContext* ctx,
                                                  const ParsedType* arrayParsed) {
    if (!ctx || !arrayParsed) return NULL;
    if (!parsedTypeIsDirectArray(arrayParsed) || !parsedTypeHasVLA(arrayParsed)) return NULL;

    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
    LLVMValueRef elemCount = LLVMConstInt(intptrTy, 1, 0);
    ParsedType cursor = parsedTypeClone(arrayParsed);

    while (parsedTypeIsDirectArray(&cursor)) {
        const TypeDerivation* deriv = parsedTypeGetDerivation(&cursor, 0);
        LLVMValueRef dim = NULL;
        if (deriv && deriv->kind == TYPE_DERIVATION_ARRAY) {
            if (!deriv->as.array.isVLA &&
                deriv->as.array.hasConstantSize &&
                deriv->as.array.constantSize > 0) {
                dim = LLVMConstInt(intptrTy, (unsigned long long)deriv->as.array.constantSize, 0);
            } else if (deriv->as.array.sizeExpr) {
                LLVMValueRef evaluated = codegenNode(ctx, deriv->as.array.sizeExpr);
                if (evaluated && LLVMGetTypeKind(LLVMTypeOf(evaluated)) == LLVMIntegerTypeKind) {
                    if (LLVMTypeOf(evaluated) != intptrTy) {
                        evaluated = LLVMBuildIntCast2(ctx->builder, evaluated, intptrTy, 0, "vla.dim.cast");
                    }
                    dim = evaluated;
                }
            }
        }
        if (!dim) {
            dim = LLVMConstInt(intptrTy, 1, 0);
        }
        elemCount = LLVMBuildMul(ctx->builder, elemCount, dim, "vla.count");

        ParsedType next = parsedTypeArrayElementType(&cursor);
        parsedTypeFree(&cursor);
        cursor = next;
    }

    LLVMTypeRef elemTy = cg_type_from_parsed(ctx, &cursor);
    uint64_t elemBytes = 0;
    uint32_t align = 0;
    if (!cg_size_align_for_type(ctx, &cursor, elemTy, &elemBytes, &align) || elemBytes == 0) {
        LLVMTargetDataRef td = ctx && ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
        if (td && elemTy && LLVMTypeIsSized(elemTy)) {
            elemBytes = LLVMABISizeOfType(td, elemTy);
        }
    }
    parsedTypeFree(&cursor);
    if (elemBytes == 0) {
        elemBytes = 1;
    }

    LLVMValueRef elemSize = LLVMConstInt(intptrTy, elemBytes, 0);
    return LLVMBuildMul(ctx->builder, elemCount, elemSize, "vla.bytes");
}

LLVMTypeRef cg_dereference_ptr_type(CodegenContext* ctx, LLVMTypeRef ptrType, const char* ctxMsg) {
    (void)ctx;
    if (!ptrType || LLVMGetTypeKind(ptrType) != LLVMPointerTypeKind) {
        if (ctxMsg) {
            CG_ERROR("Cannot dereference non-pointer in %s", ctxMsg);
        } else {
            CG_ERROR("Cannot dereference non-pointer");
        }
        return NULL;
    }
    if (LLVMPointerTypeIsOpaque(ptrType)) {
        return LLVMInt8TypeInContext(cg_context_get_llvm_context(ctx));
    }
    LLVMTypeRef elem = LLVMGetElementType(ptrType);
    if (!elem) {
        return LLVMInt8TypeInContext(cg_context_get_llvm_context(ctx));
    }
    return elem;
}

LLVMValueRef cg_build_entry_alloca(CodegenContext* ctx, LLVMTypeRef type, const char* nameHint) {
    if (!ctx || !type) {
        return NULL;
    }
    if (!ctx->builder) {
        return NULL;
    }

    const char* name = nameHint ? nameHint : "";
    LLVMBasicBlockRef currentBlock = LLVMGetInsertBlock(ctx->builder);
    if (!currentBlock) {
        return LLVMBuildAlloca(ctx->builder, type, name);
    }

    LLVMValueRef currentFunction = LLVMGetBasicBlockParent(currentBlock);
    if (!currentFunction) {
        return LLVMBuildAlloca(ctx->builder, type, name);
    }

    LLVMBasicBlockRef entryBlock = LLVMGetEntryBasicBlock(currentFunction);
    if (!entryBlock) {
        return LLVMBuildAlloca(ctx->builder, type, name);
    }

    LLVMBuilderRef entryBuilder = LLVMCreateBuilderInContext(ctx->llvmContext);
    if (!entryBuilder) {
        return LLVMBuildAlloca(ctx->builder, type, name);
    }

    LLVMValueRef firstInst = LLVMGetFirstInstruction(entryBlock);
    if (firstInst) {
        LLVMPositionBuilderBefore(entryBuilder, firstInst);
    } else {
        LLVMPositionBuilderAtEnd(entryBuilder, entryBlock);
    }

    LLVMValueRef slot = LLVMBuildAlloca(entryBuilder, type, name);
    LLVMDisposeBuilder(entryBuilder);
    return slot;
}

LLVMTypeRef cg_element_type_from_pointer_parsed(CodegenContext* ctx, const ParsedType* parsed) {
    if (!parsed || parsed->pointerDepth <= 0) {
        return NULL;
    }
    ParsedType base = parsedTypeClone(parsed);
    if (base.pointerDepth > 0) {
        base.pointerDepth -= 1;
    }
    for (ssize_t i = (ssize_t)base.derivationCount - 1; i >= 0; --i) {
        if (base.derivations[i].kind == TYPE_DERIVATION_POINTER) {
            for (size_t j = (size_t)i; j + 1 < base.derivationCount; ++j) {
                base.derivations[j] = base.derivations[j + 1];
            }
            base.derivationCount -= 1;
            break;
        }
    }
    LLVMTypeRef elem = cg_type_from_parsed(ctx, &base);
    parsedTypeFree(&base);
    return elem;
}

bool cg_is_volatile_object(const ParsedType* parsed) {
    if (!parsed) {
        return false;
    }
    if (parsed->pointerDepth > 0) {
        for (size_t i = 0; i < parsed->derivationCount; ++i) {
            const TypeDerivation* deriv = parsedTypeGetDerivation(parsed, i);
            if (!deriv) {
                break;
            }
            if (deriv->kind == TYPE_DERIVATION_POINTER) {
                return deriv->as.pointer.isVolatile;
            }
            if (deriv->kind == TYPE_DERIVATION_ARRAY) {
                return deriv->as.array.qualVolatile;
            }
        }
        return false;
    }
    return parsed->isVolatile;
}

LLVMValueRef cg_build_load(CodegenContext* ctx,
                           LLVMTypeRef type,
                           LLVMValueRef ptr,
                           const char* name,
                           const ParsedType* parsed) {
    LLVMValueRef load = LLVMBuildLoad2(ctx->builder, type, ptr, name ? name : "");
    if (cg_is_volatile_object(parsed)) {
        LLVMSetVolatile(load, 1);
    }
    return load;
}

LLVMValueRef cg_build_store(CodegenContext* ctx,
                            LLVMValueRef value,
                            LLVMValueRef ptr,
                            const ParsedType* parsed) {
    LLVMValueRef store = LLVMBuildStore(ctx->builder, value, ptr);
    if (cg_is_volatile_object(parsed)) {
        LLVMSetVolatile(store, 1);
    }
    return store;
}

LLVMTypeRef cg_get_intptr_type(CodegenContext* ctx) {
    if (ctx && ctx->module) {
        LLVMTargetDataRef layout = LLVMGetModuleDataLayout(ctx->module);
        if (layout) {
            return LLVMIntPtrType(layout);
        }
    }
    LLVMContextRef llvmCtx = cg_context_get_llvm_context(ctx);
    return llvmCtx ? LLVMInt64TypeInContext(llvmCtx) : LLVMInt64Type();
}

LLVMValueRef cg_widen_bool_to_int(CodegenContext* ctx, LLVMValueRef value, const char* nameHint) {
    LLVMTypeRef type = LLVMTypeOf(value);
    if (type && LLVMGetTypeKind(type) == LLVMIntegerTypeKind && LLVMGetIntTypeWidth(type) == 1) {
        LLVMTypeRef target = LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
        return LLVMBuildZExt(ctx->builder, value, target, nameHint ? nameHint : "bool.to.int");
    }
    return value;
}

LLVMTypeRef cg_coerce_function_return_type(CodegenContext* ctx, LLVMTypeRef returnType) {
    if (!ctx || !returnType) {
        return returnType;
    }
    LLVMTypeKind kind = LLVMGetTypeKind(returnType);
    if (kind != LLVMStructTypeKind && kind != LLVMArrayTypeKind) {
        return returnType;
    }
    LLVMTargetDataRef td = ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
    if (!td || !LLVMTypeIsSized(returnType)) {
        return returnType;
    }
    uint64_t size = LLVMABISizeOfType(td, returnType);
    if (size == 0 || size > 8) {
        return returnType;
    }
    if (cg_abi_return_contains_fp_or_vector(returnType, 0u)) {
        return returnType;
    }
    return LLVMIntTypeInContext(ctx->llvmContext, (unsigned)(size * 8u));
}

LLVMValueRef cg_pack_aggregate_for_abi_return(CodegenContext* ctx,
                                              LLVMValueRef value,
                                              LLVMTypeRef packedType,
                                              const char* nameHint) {
    if (!ctx || !value || !packedType) return value;
    LLVMTypeRef sourceType = LLVMTypeOf(value);
    if (!sourceType) return value;
    LLVMTypeKind sourceKind = LLVMGetTypeKind(sourceType);
    if (sourceKind != LLVMStructTypeKind && sourceKind != LLVMArrayTypeKind) {
        return value;
    }
    LLVMTypeKind packedKind = LLVMGetTypeKind(packedType);
    if (packedKind != LLVMIntegerTypeKind && packedKind != LLVMArrayTypeKind) {
        return value;
    }

    LLVMValueRef slot = cg_build_entry_alloca(ctx, packedType, "ret.abi.pack.slot");
    if (!slot) return value;
    LLVMBuildStore(ctx->builder, LLVMConstNull(packedType), slot);

    LLVMTypeRef sourcePtrType = LLVMPointerType(sourceType, 0);
    LLVMValueRef sourceAddr = LLVMBuildBitCast(ctx->builder, slot, sourcePtrType, "ret.abi.pack.addr");
    LLVMBuildStore(ctx->builder, value, sourceAddr);

    return LLVMBuildLoad2(ctx->builder,
                          packedType,
                          slot,
                          nameHint ? nameHint : "ret.abi.pack");
}

LLVMValueRef cg_unpack_aggregate_from_abi_return(CodegenContext* ctx,
                                                 LLVMValueRef packedValue,
                                                 LLVMTypeRef aggregateType,
                                                 const char* nameHint) {
    if (!ctx || !packedValue || !aggregateType) return packedValue;
    LLVMTypeKind aggKind = LLVMGetTypeKind(aggregateType);
    if (aggKind != LLVMStructTypeKind && aggKind != LLVMArrayTypeKind) {
        return packedValue;
    }
    LLVMTypeRef packedType = LLVMTypeOf(packedValue);
    if (!packedType) return packedValue;
    LLVMTypeKind packedKind = LLVMGetTypeKind(packedType);
    if (packedKind != LLVMIntegerTypeKind && packedKind != LLVMArrayTypeKind) {
        return packedValue;
    }

    LLVMValueRef slot = cg_build_entry_alloca(ctx, aggregateType, "ret.abi.unpack.slot");
    if (!slot) return packedValue;
    LLVMBuildStore(ctx->builder, LLVMConstNull(aggregateType), slot);

    LLVMTypeRef packedPtrType = LLVMPointerType(packedType, 0);
    LLVMValueRef packedAddr = LLVMBuildBitCast(ctx->builder, slot, packedPtrType, "ret.abi.unpack.addr");
    LLVMBuildStore(ctx->builder, packedValue, packedAddr);

    return LLVMBuildLoad2(ctx->builder,
                          aggregateType,
                          slot,
                          nameHint ? nameHint : "ret.abi.unpack");
}

LLVMTypeRef cg_lower_parameter_type(CodegenContext* ctx,
                                    const ParsedType* parsed,
                                    bool* outIndirect,
                                    LLVMTypeRef* outValueType) {
    if (outIndirect) {
        *outIndirect = false;
    }
    if (outValueType) {
        *outValueType = NULL;
    }
    if (!ctx || !parsed) {
        return NULL;
    }

    ParsedType adjusted = parsedTypeClone(parsed);
    cg_adjust_parameter_type_for_lowering(&adjusted);

    LLVMTypeRef valueType = cg_type_from_parsed(ctx, &adjusted);
    bool passIndirect = false;
    if (valueType) {
        LLVMTypeKind kind = LLVMGetTypeKind(valueType);
        if (kind == LLVMStructTypeKind || kind == LLVMArrayTypeKind) {
            uint64_t size = 0;
            uint32_t align = 0;
            bool haveSize = cg_size_align_for_type(ctx, &adjusted, valueType, &size, &align);
            if (!haveSize) {
                LLVMTargetDataRef td = ctx && ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
                if (td && LLVMTypeIsSized(valueType)) {
                    size = LLVMABISizeOfType(td, valueType);
                    align = (uint32_t)LLVMABIAlignmentOfType(td, valueType);
                    haveSize = true;
                }
            }
            if (haveSize && size > 16) {
                passIndirect = true;
            }
        }
    }

    parsedTypeFree(&adjusted);

    if (outValueType) {
        *outValueType = valueType;
    }
    if (outIndirect) {
        *outIndirect = passIndirect;
    }
    if (passIndirect && valueType) {
        return LLVMPointerType(valueType, 0);
    }
    return valueType;
}

bool cg_size_align_of_parsed(CodegenContext* ctx,
                             const ParsedType* parsed,
                             uint64_t* outSize,
                             uint32_t* outAlign) {
    if (!ctx || !parsed) return false;
    const ParsedType* surfaceParsed = cg_resolve_call_surface_type(ctx, parsed);
    if (!surfaceParsed) {
        surfaceParsed = parsed;
    }
    const SemanticModel* model = cg_context_get_semantic_model(ctx);
    if (!model) return false;
    const ParsedType* modelResolved = surfaceParsed;
    if (surfaceParsed->kind == TYPE_NAMED &&
        surfaceParsed->userTypeName &&
        !cg_named_type_has_surface_derivations(surfaceParsed)) {
        const Symbol* aliasSym = semanticModelLookupGlobal(model, surfaceParsed->userTypeName);
        if (aliasSym && aliasSym->kind == SYMBOL_TYPEDEF) {
            modelResolved = &aliasSym->type;
        }
    }
    CompilerContext* cctx = semanticModelGetContext(model);
    struct Scope* gscope = semanticModelGetGlobalScope(model);
    if (!cctx || !gscope) return false;
    ParsedType copy = *modelResolved;
    size_t sz = 0, al = 0;
    if (!size_align_of_parsed_type(&copy, gscope, &sz, &al)) {
        return false;
    }
    if (outSize) *outSize = (uint64_t)sz;
    if (outAlign) *outAlign = (uint32_t)al;
    return true;
}

bool cg_size_align_for_type(CodegenContext* ctx,
                            const ParsedType* parsed,
                            LLVMTypeRef llvmHint,
                            uint64_t* outSize,
                            uint32_t* outAlign) {
    uint64_t sz = 0;
    uint32_t al = 0;
    bool haveSemantic = cg_size_align_of_parsed(ctx, parsed, &sz, &al);

    LLVMTargetDataRef tdata = ctx && ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
    bool haveLLVM = false;
    uint64_t llvmSize = 0;
    uint32_t llvmAlign = 0;
    if (tdata && llvmHint && LLVMTypeIsSized(llvmHint)) {
        llvmSize = LLVMABISizeOfType(tdata, llvmHint);
        llvmAlign = (uint32_t)LLVMABIAlignmentOfType(tdata, llvmHint);
        haveLLVM = true;
    }

    if (haveSemantic) {
        if (haveLLVM && getenv("FISICS_DEBUG_LAYOUT")) {
            if (sz != llvmSize || (al && llvmAlign && al != llvmAlign)) {
                fprintf(stderr,
                        "[layout] semantic vs LLVM mismatch: sz=%llu(%llu) align=%u(%u)\n",
                        (unsigned long long)sz,
                        (unsigned long long)llvmSize,
                        al,
                        llvmAlign);
            }
        }
        if (outSize) *outSize = sz;
        if (outAlign) *outAlign = al ? al : (haveLLVM ? llvmAlign : 1);
        return true;
    }

    if (haveLLVM) {
        if (outSize) *outSize = llvmSize;
        if (outAlign) *outAlign = llvmAlign ? llvmAlign : 1;
        return true;
    }
    return false;
}

LLVMTypeRef cg_element_type_from_pointer(CodegenContext* ctx,
                                         const ParsedType* pointerParsed,
                                         LLVMTypeRef pointerLLVM) {
    if (pointerParsed) {
        ParsedType element = parsedTypePointerTargetType(pointerParsed);
        if (element.kind != TYPE_INVALID) {
            LLVMTypeRef elemType = cg_type_from_parsed(ctx, &element);
            parsedTypeFree(&element);
            if (elemType) {
                return elemType;
            }
        } else {
            parsedTypeFree(&element);
        }
    }
    LLVMTypeRef elem = cg_dereference_ptr_type(ctx, pointerLLVM, "element type lookup");
    if (elem && LLVMGetTypeKind(elem) == LLVMArrayTypeKind) {
        unsigned len = LLVMGetArrayLength(elem);
        LLVMTypeRef inner = LLVMGetElementType(elem);
        if (len == 0 && inner) {
            elem = inner;
        } else {
            elem = inner;
        }
    }
    if (elem && LLVMGetTypeKind(elem) != LLVMVoidTypeKind) {
        return elem;
    }
    return LLVMInt8TypeInContext(cg_context_get_llvm_context(ctx));
}

LLVMTypeRef cg_element_type_hint_from_parsed(CodegenContext* ctx, const ParsedType* parsedType) {
    if (!ctx || !parsedType) {
        return NULL;
    }
    const ParsedType* surfaceParsed = cg_resolve_call_surface_type(ctx, parsedType);
    if (!surfaceParsed) {
        surfaceParsed = parsedType;
    }
    const SemanticModel* model = cg_context_get_semantic_model(ctx);
    if (model &&
        surfaceParsed->kind == TYPE_NAMED &&
        surfaceParsed->userTypeName &&
        !cg_named_type_has_surface_derivations(surfaceParsed)) {
        const Symbol* aliasSym = semanticModelLookupGlobal(model, surfaceParsed->userTypeName);
        if (aliasSym && aliasSym->kind == SYMBOL_TYPEDEF) {
            surfaceParsed = &aliasSym->type;
        }
    }

    LLVMTypeRef hint = NULL;
    if (parsedTypeIsDirectArray(surfaceParsed)) {
        ParsedType element = parsedTypeArrayElementType(surfaceParsed);
        hint = cg_type_from_parsed(ctx, &element);
        parsedTypeFree(&element);
    } else {
        ParsedType pointed = parsedTypePointerTargetType(surfaceParsed);
        if (pointed.kind != TYPE_INVALID) {
            hint = cg_type_from_parsed(ctx, &pointed);
        }
        parsedTypeFree(&pointed);
    }

    if (hint && LLVMGetTypeKind(hint) == LLVMVoidTypeKind) {
        hint = NULL;
    }
    return hint;
}

LLVMValueRef cg_build_pointer_offset(CodegenContext* ctx,
                                     LLVMValueRef basePtr,
                                     LLVMValueRef offsetValue,
                                     const ParsedType* pointerParsed,
                                     const ParsedType* offsetParsed,
                                     bool isSubtract) {
    if (!ctx || !basePtr || !offsetValue) return NULL;
    (void)offsetParsed;

    LLVMTypeRef ptrType = LLVMTypeOf(basePtr);
    if (!ptrType || LLVMGetTypeKind(ptrType) != LLVMPointerTypeKind) {
        if (getenv("DEBUG_PTR_ARITH")) {
            fprintf(stderr, "Error: pointer arithmetic requires pointer base\n");
        }
        return NULL;
    }
    if (getenv("DEBUG_PTR_ARITH") && pointerParsed) {
        fprintf(stderr, "[ptr-arith] pointerDepth=%d derivations=", pointerParsed->pointerDepth);
        for (size_t i = 0; i < pointerParsed->derivationCount; ++i) {
            const TypeDerivation* deriv = &pointerParsed->derivations[i];
            const char* kind = "?";
            if (deriv->kind == TYPE_DERIVATION_ARRAY) kind = "array";
            else if (deriv->kind == TYPE_DERIVATION_POINTER) kind = "ptr";
            else if (deriv->kind == TYPE_DERIVATION_FUNCTION) kind = "func";
            fprintf(stderr, "%s%s", (i ? "," : ""), kind);
        }
        fprintf(stderr, "\n");
    }
    {
        const ParsedType* elemParsed = pointerParsed;
        if (pointerParsed &&
            !cg_parsed_type_has_pointer_layer(pointerParsed) &&
            parsedTypeIsDirectArray(pointerParsed)) {
            static ParsedType tmpElem;
            parsedTypeFree(&tmpElem);
            tmpElem = parsedTypeArrayElementType(pointerParsed);
            elemParsed = &tmpElem;

            LLVMTypeRef arrTy = cg_dereference_ptr_type(ctx, ptrType, "pointer arithmetic decay");
            if (arrTy && LLVMGetTypeKind(arrTy) == LLVMArrayTypeKind) {
                LLVMValueRef zero32 = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
                LLVMValueRef idxs[2] = { zero32, zero32 };
                basePtr = LLVMBuildGEP2(ctx->builder, arrTy, basePtr, idxs, 2, "ptr.arith.decay");
                ptrType = LLVMTypeOf(basePtr);
            }
        }

        LLVMValueRef index = offsetValue;
        LLVMTypeRef offsetType = LLVMTypeOf(offsetValue);
        if (offsetType && LLVMGetTypeKind(offsetType) == LLVMIntegerTypeKind) {
            LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
            unsigned offsetBits = LLVMGetIntTypeWidth(offsetType);
            unsigned intptrBits = LLVMGetIntTypeWidth(intptrTy);
            if (offsetBits != intptrBits) {
                bool offsetIsUnsigned = cg_parsed_type_is_unsigned_for_index(ctx, offsetParsed);
                index = LLVMBuildIntCast2(ctx->builder,
                                          offsetValue,
                                          intptrTy,
                                          offsetIsUnsigned ? 0 : 1,
                                          "ptr.idx.cast");
            }
        } else if (LLVMGetTypeKind(offsetType) == LLVMPointerTypeKind) {
            if (getenv("DEBUG_PTR_ARITH")) {
                fprintf(stderr, "Error: pointer arithmetic with pointer offset unsupported\n");
            }
            return NULL;
        } else {
            if (getenv("DEBUG_PTR_ARITH")) {
                fprintf(stderr, "Error: pointer arithmetic offset must be integer\n");
            }
            return NULL;
        }

        if (isSubtract) {
            index = LLVMBuildNeg(ctx->builder, index, "ptr.idx.neg");
        }

        LLVMTypeRef elementType = cg_element_type_from_pointer(ctx, elemParsed, ptrType);
        ParsedType targetParsed = parsedTypePointerTargetType(elemParsed);
        const ParsedType* sizeParsed = elemParsed;
        if (targetParsed.kind != TYPE_INVALID) {
            sizeParsed = &targetParsed;
        }
        uint64_t elemSize = 0;
        uint32_t elemAlign = 0;
        LLVMValueRef dynamicElemSize = NULL;
        if (sizeParsed && parsedTypeIsDirectArray(sizeParsed) && parsedTypeHasVLA(sizeParsed)) {
            dynamicElemSize = cg_build_vla_array_size_bytes(ctx, sizeParsed);
        }
        LLVMTypeRef sizeHintType = cg_type_from_parsed(ctx, sizeParsed);
        if (!sizeHintType || LLVMGetTypeKind(sizeHintType) == LLVMVoidTypeKind) {
            sizeHintType = elementType;
        }
        if (!dynamicElemSize) {
            if (!cg_pointer_elem_size(ctx, sizeParsed, sizeHintType, &elemSize, &elemAlign) || elemSize == 0) {
                LLVMTargetDataRef td = ctx && ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
                if (td && sizeHintType && LLVMTypeIsSized(sizeHintType)) {
                    elemSize = LLVMABISizeOfType(td, sizeHintType);
                }
            }
        }
        if (targetParsed.kind != TYPE_INVALID) {
            parsedTypeFree(&targetParsed);
        }
        if (!dynamicElemSize && elemSize == 0) elemSize = 1;

        LLVMTypeRef i8Type = LLVMInt8TypeInContext(ctx->llvmContext);
        LLVMTypeRef bytePtrTy = LLVMPointerType(i8Type, 0);
        LLVMValueRef baseI8 = LLVMBuildPointerCast(ctx->builder, basePtr, bytePtrTy, "ptr.byte.base");
        LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
        LLVMValueRef elemSizeVal = dynamicElemSize ? dynamicElemSize : LLVMConstInt(intptrTy, elemSize, 0);
        LLVMValueRef byteOffset = LLVMBuildMul(ctx->builder, index, elemSizeVal, "ptr.byte.offset");
        LLVMValueRef gep = LLVMBuildGEP2(ctx->builder, i8Type, baseI8, &byteOffset, 1, "ptr.arith");
        if (!gep) return NULL;
        return gep;
    }
}

LLVMValueRef cg_build_pointer_difference(CodegenContext* ctx,
                                         LLVMValueRef lhsPtr,
                                         LLVMValueRef rhsPtr,
                                         const ParsedType* lhsParsed,
                                         const ParsedType* rhsParsed) {
    if (!ctx || !lhsPtr || !rhsPtr) return NULL;
    LLVMTypeRef lhsType = LLVMTypeOf(lhsPtr);
    LLVMTypeRef rhsType = LLVMTypeOf(rhsPtr);

    if (!lhsType || !rhsType || LLVMGetTypeKind(lhsType) != LLVMPointerTypeKind ||
        LLVMGetTypeKind(rhsType) != LLVMPointerTypeKind) {
        fprintf(stderr, "Error: pointer difference requires pointer operands\n");
        return NULL;
    }

    LLVMTypeRef i8Type = LLVMInt8TypeInContext(ctx->llvmContext);
    LLVMTypeRef bytePtrTy = LLVMPointerType(i8Type, 0);
    LLVMValueRef lhsByte = LLVMBuildPointerCast(ctx->builder, lhsPtr, bytePtrTy, "ptr.diff.lhs.byte");
    LLVMValueRef rhsByte = LLVMBuildPointerCast(ctx->builder, rhsPtr, bytePtrTy, "ptr.diff.rhs.byte");

    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
    LLVMValueRef byteDiff = LLVMBuildPtrDiff2(ctx->builder, i8Type, lhsByte, rhsByte, "ptr.diff.bytes");

    uint64_t elemBytes = 0;
    uint64_t lhsElemBytes = 0;
    uint64_t rhsElemBytes = 0;
    bool haveLhsElem = false;
    bool haveRhsElem = false;
    bool lhsCharPtr = false;
    bool rhsCharPtr = false;
    LLVMValueRef dynamicElemSize = NULL;

    ParsedType lhsTarget = {0};
    lhsTarget.kind = TYPE_INVALID;
    if (lhsParsed) {
        lhsTarget = (parsedTypeIsDirectArray(lhsParsed) && !cg_parsed_type_has_pointer_layer(lhsParsed))
                        ? parsedTypeArrayElementType(lhsParsed)
                        : parsedTypePointerTargetType(lhsParsed);
        if (lhsTarget.kind != TYPE_INVALID) {
            lhsCharPtr = (lhsTarget.kind == TYPE_PRIMITIVE &&
                          lhsTarget.primitiveType == TOKEN_CHAR &&
                          lhsTarget.pointerDepth == 0 &&
                          !parsedTypeIsDirectArray(&lhsTarget));
            if (parsedTypeIsDirectArray(&lhsTarget) && parsedTypeHasVLA(&lhsTarget)) {
                dynamicElemSize = cg_build_vla_array_size_bytes(ctx, &lhsTarget);
                haveLhsElem = dynamicElemSize != NULL;
            } else if (parsedTypeIsDirectArray(&lhsTarget)) {
                uint32_t al = 0;
                haveLhsElem = cg_size_align_of_parsed(ctx, &lhsTarget, &lhsElemBytes, &al) && lhsElemBytes > 0;
            }
            if (!haveLhsElem) {
                LLVMTypeRef hint = cg_type_from_parsed(ctx, &lhsTarget);
                uint32_t al = 0;
                haveLhsElem = cg_pointer_elem_size(ctx, &lhsTarget, hint, &lhsElemBytes, &al) && lhsElemBytes > 0;
            }
        }
        parsedTypeFree(&lhsTarget);
    }

    ParsedType rhsTarget = {0};
    rhsTarget.kind = TYPE_INVALID;
    if (rhsParsed) {
        rhsTarget = (parsedTypeIsDirectArray(rhsParsed) && !cg_parsed_type_has_pointer_layer(rhsParsed))
                        ? parsedTypeArrayElementType(rhsParsed)
                        : parsedTypePointerTargetType(rhsParsed);
        if (rhsTarget.kind != TYPE_INVALID) {
            rhsCharPtr = (rhsTarget.kind == TYPE_PRIMITIVE &&
                          rhsTarget.primitiveType == TOKEN_CHAR &&
                          rhsTarget.pointerDepth == 0 &&
                          !parsedTypeIsDirectArray(&rhsTarget));
            if (!dynamicElemSize && parsedTypeIsDirectArray(&rhsTarget) && parsedTypeHasVLA(&rhsTarget)) {
                dynamicElemSize = cg_build_vla_array_size_bytes(ctx, &rhsTarget);
                haveRhsElem = dynamicElemSize != NULL;
            } else if (parsedTypeIsDirectArray(&rhsTarget)) {
                uint32_t al = 0;
                haveRhsElem = cg_size_align_of_parsed(ctx, &rhsTarget, &rhsElemBytes, &al) && rhsElemBytes > 0;
            }
            if (!haveRhsElem) {
                LLVMTypeRef hint = cg_type_from_parsed(ctx, &rhsTarget);
                uint32_t al = 0;
                haveRhsElem = cg_pointer_elem_size(ctx, &rhsTarget, hint, &rhsElemBytes, &al) && rhsElemBytes > 0;
            }
        }
        parsedTypeFree(&rhsTarget);
    }

    if (lhsCharPtr || rhsCharPtr) {
        elemBytes = 1;
    } else if (haveLhsElem && haveRhsElem) {
        if (lhsElemBytes == rhsElemBytes) {
            elemBytes = lhsElemBytes;
        } else {
            elemBytes = lhsElemBytes;
            if (getenv("DEBUG_PTR_ARITH")) {
                fprintf(stderr,
                        "[ptr-arith] pointer diff size mismatch lhs=%llu rhs=%llu (using lhs)\n",
                        (unsigned long long)lhsElemBytes,
                        (unsigned long long)rhsElemBytes);
            }
        }
    } else if (haveLhsElem) {
        elemBytes = lhsElemBytes;
    } else if (haveRhsElem) {
        elemBytes = rhsElemBytes;
    }
    if (getenv("DEBUG_PTR_ARITH")) {
        if (lhsParsed) {
            fprintf(stderr,
                    "[ptr-arith] ptrdiff lhsParsed kind=%d prim=%d ptrDepth=%d derivs=%zu\n",
                    lhsParsed->kind,
                    lhsParsed->primitiveType,
                    lhsParsed->pointerDepth,
                    lhsParsed->derivationCount);
        }
        if (rhsParsed) {
            fprintf(stderr,
                    "[ptr-arith] ptrdiff rhsParsed kind=%d prim=%d ptrDepth=%d derivs=%zu\n",
                    rhsParsed->kind,
                    rhsParsed->primitiveType,
                    rhsParsed->pointerDepth,
                    rhsParsed->derivationCount);
        }
        fprintf(stderr,
                "[ptr-arith] ptrdiff lhsBytes=%llu rhsBytes=%llu lhsChar=%d rhsChar=%d chosen=%llu\n",
                (unsigned long long)lhsElemBytes,
                (unsigned long long)rhsElemBytes,
                lhsCharPtr ? 1 : 0,
                rhsCharPtr ? 1 : 0,
                (unsigned long long)elemBytes);
    }

    LLVMTypeRef elemHint = cg_element_type_hint_from_parsed(ctx, lhsParsed);
    if (!elemHint) {
        elemHint = cg_element_type_hint_from_parsed(ctx, rhsParsed);
    }
    if (elemBytes == 0 && elemHint) {
        LLVMTargetDataRef td = ctx && ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
        if (td && LLVMTypeIsSized(elemHint)) {
            elemBytes = LLVMABISizeOfType(td, elemHint);
        }
    }
    if (!dynamicElemSize && elemBytes == 0) {
        CG_ERROR("Cannot compute element size for pointer difference");
        return NULL;
    }
    LLVMValueRef elementSize =
        dynamicElemSize ? dynamicElemSize : LLVMConstInt(intptrTy, elemBytes, 0);
    return LLVMBuildSDiv(ctx->builder, byteDiff, elementSize, "ptr.diff");
}
