// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"

#include <llvm-c/Core.h>

static bool cg_kind_is_float(LLVMTypeKind kind) {
    switch (kind) {
        case LLVMHalfTypeKind:
        case LLVMFloatTypeKind:
        case LLVMDoubleTypeKind:
        case LLVMFP128TypeKind:
        case LLVMX86_FP80TypeKind:
            return true;
        default:
            return false;
    }
}

static unsigned cg_float_rank(LLVMTypeKind kind) {
    switch (kind) {
        case LLVMHalfTypeKind: return 1;
        case LLVMFloatTypeKind: return 2;
        case LLVMDoubleTypeKind: return 3;
        case LLVMX86_FP80TypeKind: return 4;
        case LLVMFP128TypeKind: return 5;
        default: return 0;
    }
}

static LLVMValueRef cg_build_complex(CodegenContext* ctx,
                                     LLVMValueRef real,
                                     LLVMValueRef imag,
                                     LLVMTypeRef complexType,
                                     const char* nameHint) {
    if (!ctx || !complexType) return NULL;
    LLVMValueRef tmp = LLVMGetUndef(complexType);
    tmp = LLVMBuildInsertValue(ctx->builder, tmp, real, 0, nameHint ? nameHint : "complex.r");
    tmp = LLVMBuildInsertValue(ctx->builder, tmp, imag, 1, nameHint ? nameHint : "complex.i");
    return tmp;
}

static bool cg_is_unsigned_category(CGValueCategory category) {
    return category == CG_VALUE_UNSIGNED_INT || category == CG_VALUE_BOOL;
}

static CGValueCategory cg_classify_parsed_type(const ParsedType* type) {
    if (!type) {
        return CG_VALUE_UNKNOWN;
    }

    if (type->isFunctionPointer || type->pointerDepth > 0) {
        return CG_VALUE_POINTER;
    }

    switch (type->kind) {
        case TYPE_STRUCT:
        case TYPE_UNION:
            return CG_VALUE_AGGREGATE;
        case TYPE_ENUM:
            return CG_VALUE_SIGNED_INT;
        case TYPE_PRIMITIVE:
            switch (type->primitiveType) {
                case TOKEN_BOOL:
                    return CG_VALUE_BOOL;
                case TOKEN_FLOAT:
                case TOKEN_DOUBLE:
                    return CG_VALUE_FLOAT;
                case TOKEN_UNSIGNED:
                    return CG_VALUE_UNSIGNED_INT;
                case TOKEN_SIGNED:
                case TOKEN_INT:
                case TOKEN_LONG:
                case TOKEN_SHORT:
                case TOKEN_CHAR:
                    return type->isUnsigned ? CG_VALUE_UNSIGNED_INT : CG_VALUE_SIGNED_INT;
                default:
                    break;
            }
            if (type->isUnsigned) {
                return CG_VALUE_UNSIGNED_INT;
            }
            return CG_VALUE_SIGNED_INT;
        case TYPE_NAMED:
            switch (type->tag) {
                case TAG_STRUCT:
                case TAG_UNION:
                    return CG_VALUE_AGGREGATE;
                case TAG_ENUM:
                    return CG_VALUE_SIGNED_INT;
                default:
                    break;
            }
            return type->isUnsigned ? CG_VALUE_UNSIGNED_INT : CG_VALUE_SIGNED_INT;
        case TYPE_INVALID:
        default:
            break;
    }

    if (type->isUnsigned) {
        return CG_VALUE_UNSIGNED_INT;
    }
    return CG_VALUE_SIGNED_INT;
}

static bool cg_should_treat_as_unsigned(CodegenContext* ctx,
                                        const ParsedType* parsedType,
                                        LLVMTypeRef llvmType) {
    if (parsedType) {
        const ParsedType* resolved = parsedType;
        const SemanticModel* model = cg_context_get_semantic_model(ctx);
        if (model && parsedType->kind == TYPE_NAMED && parsedType->userTypeName) {
            const Symbol* sym = semanticModelLookupGlobal(model, parsedType->userTypeName);
            if (sym && sym->kind == SYMBOL_TYPEDEF) {
                resolved = &sym->type;
            }
        }
        CGValueCategory category = cg_classify_parsed_type(resolved);
        return cg_is_unsigned_category(category);
    }

    LLVMTypeKind kind = LLVMGetTypeKind(llvmType);
    if (kind == LLVMIntegerTypeKind && LLVMGetIntTypeWidth(llvmType) == 1) {
        return true;
    }
    return false;
}

static bool cg_parsed_type_is_complex(const ParsedType* type) {
    return type && (type->isComplex || type->isImaginary);
}

static bool cg_kind_is_aggregate_like(LLVMTypeKind kind) {
    return kind == LLVMStructTypeKind || kind == LLVMArrayTypeKind;
}

static bool cg_kind_is_abi_pack_scalar(LLVMTypeKind kind) {
    return kind == LLVMIntegerTypeKind || kind == LLVMArrayTypeKind;
}

static LLVMValueRef cg_reinterpret_via_stack(CodegenContext* ctx,
                                             LLVMValueRef value,
                                             LLVMTypeRef targetType,
                                             const char* tag) {
    if (!ctx || !value || !targetType || !ctx->module) {
        return value;
    }
    LLVMTypeRef sourceType = LLVMTypeOf(value);
    if (!sourceType) {
        return value;
    }

    LLVMTargetDataRef td = LLVMGetModuleDataLayout(ctx->module);
    if (!td) {
        return value;
    }

    uint64_t srcSize = LLVMABISizeOfType(td, sourceType);
    uint64_t dstSize = LLVMABISizeOfType(td, targetType);
    if (srcSize == 0 || dstSize == 0 || srcSize != dstSize) {
        return value;
    }

    LLVMValueRef slot = cg_build_entry_alloca(ctx, targetType, "cast.repack.slot");
    if (!slot) {
        return value;
    }

    if (!cg_kind_is_aggregate_like(LLVMGetTypeKind(targetType))) {
        LLVMBuildStore(ctx->builder, LLVMConstNull(targetType), slot);
    }

    LLVMTypeRef sourcePtrType = LLVMPointerType(sourceType, 0);
    LLVMValueRef sourceAddr = LLVMBuildBitCast(ctx->builder, slot, sourcePtrType, "cast.repack.addr");
    LLVMBuildStore(ctx->builder, value, sourceAddr);
    return LLVMBuildLoad2(ctx->builder, targetType, slot, tag);
}

static LLVMTypeRef cg_complex_element_type(CodegenContext* ctx, const ParsedType* type) {
    LLVMContextRef llvmCtx = cg_context_get_llvm_context(ctx);
    if (!type) {
        return LLVMDoubleTypeInContext(llvmCtx);
    }
    switch (type->primitiveType) {
        case TOKEN_FLOAT:
            return LLVMFloatTypeInContext(llvmCtx);
        case TOKEN_DOUBLE:
        default:
            if (type->isLong) {
                const TargetLayout* tl = cg_context_get_target_layout(ctx);
                if (tl && tl->longDoubleBits == 128) {
                    return LLVMFP128TypeInContext(llvmCtx);
                }
                if (tl && tl->longDoubleBits == 64) {
                    return LLVMDoubleTypeInContext(llvmCtx);
                }
                LLVMTypeRef fp80 = LLVMX86FP80TypeInContext(llvmCtx);
                if (fp80) {
                    return fp80;
                }
            }
            return LLVMDoubleTypeInContext(llvmCtx);
    }
}

LLVMValueRef cg_cast_value(CodegenContext* ctx,
                           LLVMValueRef value,
                           LLVMTypeRef targetType,
                           const ParsedType* fromParsed,
                           const ParsedType* toParsed,
                           const char* nameHint) {
    if (!ctx || !value || !targetType) {
        return value;
    }

    LLVMTypeRef sourceType = LLVMTypeOf(value);
    if (sourceType == targetType) {
        return value;
    }

    const char* tag = nameHint ? nameHint : "casttmp";
    LLVMTypeKind srcKind = LLVMGetTypeKind(sourceType);
    LLVMTypeKind dstKind = LLVMGetTypeKind(targetType);

    bool srcUnsigned = cg_should_treat_as_unsigned(ctx, fromParsed, sourceType);
    bool dstUnsigned = cg_should_treat_as_unsigned(ctx, toParsed, targetType);

    if (dstKind == LLVMIntegerTypeKind && LLVMGetIntTypeWidth(targetType) == 1) {
        if (srcKind == LLVMIntegerTypeKind) {
            if (LLVMGetIntTypeWidth(sourceType) == 1) {
                return value;
            }
            LLVMValueRef zero = LLVMConstInt(sourceType, 0, 0);
            return LLVMBuildICmp(ctx->builder, LLVMIntNE, value, zero, tag);
        }
        if (srcKind == LLVMPointerTypeKind) {
            LLVMValueRef nullPtr = LLVMConstPointerNull(sourceType);
            return LLVMBuildICmp(ctx->builder, LLVMIntNE, value, nullPtr, tag);
        }
        if (cg_kind_is_float(srcKind)) {
            LLVMValueRef zero = LLVMConstNull(sourceType);
            return LLVMBuildFCmp(ctx->builder, LLVMRealUNE, value, zero, tag);
        }
    }

    if (cg_parsed_type_is_complex(toParsed)) {
        LLVMTypeRef elemTy = cg_complex_element_type(ctx, toParsed);
        LLVMValueRef realVal = NULL;
        LLVMValueRef imagVal = NULL;
        if (cg_parsed_type_is_complex(fromParsed) && srcKind == LLVMStructTypeKind) {
            LLVMValueRef realPart = LLVMBuildExtractValue(ctx->builder, value, 0, "complex.real");
            LLVMValueRef imagPart = LLVMBuildExtractValue(ctx->builder, value, 1, "complex.imag");
            realVal = cg_cast_value(ctx, realPart, elemTy, fromParsed, NULL, "complex.r.cast");
            imagVal = cg_cast_value(ctx, imagPart, elemTy, fromParsed, NULL, "complex.i.cast");
        } else {
            LLVMValueRef casted = value;
            if (LLVMGetTypeKind(LLVMTypeOf(value)) != LLVMGetTypeKind(elemTy) ||
                LLVMTypeOf(value) != elemTy) {
                casted = cg_cast_value(ctx, value, elemTy, fromParsed, NULL, "complex.cast");
            }
            if (fromParsed && fromParsed->isImaginary) {
                realVal = LLVMConstNull(elemTy);
                imagVal = casted;
            } else {
                realVal = casted;
                imagVal = LLVMConstNull(elemTy);
            }
        }
        return cg_build_complex(ctx, realVal, imagVal, targetType, "complex.pack");
    }

    if (cg_parsed_type_is_complex(fromParsed) && srcKind == LLVMStructTypeKind) {
        LLVMValueRef realPart = LLVMBuildExtractValue(ctx->builder, value, 0, "complex.real");
        if (fromParsed && fromParsed->isImaginary) {
            realPart = LLVMBuildExtractValue(ctx->builder, value, 1, "complex.imag");
        }
        return cg_cast_value(ctx, realPart, targetType, NULL, toParsed, "complex.to.scalar");
    }

    if ((cg_kind_is_aggregate_like(srcKind) && cg_kind_is_abi_pack_scalar(dstKind)) ||
        (cg_kind_is_abi_pack_scalar(srcKind) && cg_kind_is_aggregate_like(dstKind))) {
        LLVMValueRef repacked = cg_reinterpret_via_stack(ctx, value, targetType, tag);
        if (repacked && LLVMTypeOf(repacked) == targetType) {
            return repacked;
        }
    }

    if (srcKind == LLVMIntegerTypeKind && dstKind == LLVMIntegerTypeKind) {
        unsigned srcBits = LLVMGetIntTypeWidth(sourceType);
        unsigned dstBits = LLVMGetIntTypeWidth(targetType);
        if (srcBits == dstBits) {
            return LLVMBuildBitCast(ctx->builder, value, targetType, tag);
        } else if (srcBits < dstBits) {
            return srcUnsigned
                ? LLVMBuildZExt(ctx->builder, value, targetType, tag)
                : LLVMBuildSExt(ctx->builder, value, targetType, tag);
        } else {
            return LLVMBuildTrunc(ctx->builder, value, targetType, tag);
        }
    }

    if (cg_kind_is_float(srcKind) && cg_kind_is_float(dstKind)) {
        unsigned srcRank = cg_float_rank(srcKind);
        unsigned dstRank = cg_float_rank(dstKind);
        if (srcRank < dstRank) {
            return LLVMBuildFPExt(ctx->builder, value, targetType, tag);
        }
        return LLVMBuildFPTrunc(ctx->builder, value, targetType, tag);
    }

    if (cg_kind_is_float(srcKind) && dstKind == LLVMIntegerTypeKind) {
        return dstUnsigned
            ? LLVMBuildFPToUI(ctx->builder, value, targetType, tag)
            : LLVMBuildFPToSI(ctx->builder, value, targetType, tag);
    }

    if (srcKind == LLVMIntegerTypeKind && cg_kind_is_float(dstKind)) {
        return srcUnsigned
            ? LLVMBuildUIToFP(ctx->builder, value, targetType, tag)
            : LLVMBuildSIToFP(ctx->builder, value, targetType, tag);
    }

    if (srcKind == LLVMPointerTypeKind && dstKind == LLVMPointerTypeKind) {
        return LLVMBuildBitCast(ctx->builder, value, targetType, tag);
    }

    if (srcKind == LLVMPointerTypeKind && dstKind == LLVMIntegerTypeKind) {
        return LLVMBuildPtrToInt(ctx->builder, value, targetType, tag);
    }

    if (srcKind == LLVMIntegerTypeKind && dstKind == LLVMPointerTypeKind) {
        return LLVMBuildIntToPtr(ctx->builder, value, targetType, tag);
    }

    return LLVMBuildBitCast(ctx->builder, value, targetType, tag);
}
