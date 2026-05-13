// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"

#include <stdlib.h>

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

bool cg_should_lower_variadic_sret(CodegenContext* ctx,
                                   LLVMTypeRef returnType,
                                   bool isVariadicFunction) {
    if (!isVariadicFunction) {
        return false;
    }
    return cg_should_lower_indirect_aggregate_return(ctx, returnType);
}

bool cg_should_lower_indirect_aggregate_return(CodegenContext* ctx, LLVMTypeRef returnType) {
    if (!ctx || !returnType) {
        return false;
    }
    LLVMTypeKind kind = LLVMGetTypeKind(returnType);
    if (kind != LLVMStructTypeKind && kind != LLVMArrayTypeKind) {
        return false;
    }
    LLVMTargetDataRef td = ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
    if (!td || !LLVMTypeIsSized(returnType)) {
        return false;
    }
    uint64_t size = LLVMABISizeOfType(td, returnType);
    return size > 16u;
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
