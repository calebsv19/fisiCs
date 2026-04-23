// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"
#include "codegen_expr_internal.h"

#include "codegen_types.h"
#include "Parser/Helpers/parsed_type.h"
#include "literal_utils.h"
#include "Syntax/type_checker.h"
#include "Syntax/layout.h"
#include "Syntax/Expr/analyze_expr.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

bool cg_is_builtin_bool_literal_name(const char* name) {
    if (!name) return false;
    return strcmp(name, "true") == 0 || strcmp(name, "false") == 0;
}

bool cg_parsed_type_is_pointerish(const ParsedType* t) {
    if (!t) return false;
    if (t->isFunctionPointer) return true;
    if (t->pointerDepth > 0) return true;
    for (size_t i = 0; i < t->derivationCount; ++i) {
        if (t->derivations[i].kind == TYPE_DERIVATION_POINTER) {
            return true;
        }
    }
    return false;
}

bool cg_eval_builtin_offsetof(CodegenContext* ctx,
                              const ParsedType* type,
                              const char* fieldName,
                              size_t* offsetOut) {
    if (!ctx || !type || !fieldName || !offsetOut) return false;
    const SemanticModel* model = cg_context_get_semantic_model(ctx);
    CompilerContext* cctx = model ? semanticModelGetContext(model) : NULL;
    Scope* scope = model ? semanticModelGetGlobalScope(model) : NULL;
    if (!cctx || !scope) return false;
    return evalOffsetofFieldPath(type, fieldName, scope, offsetOut, NULL);
}

bool cg_parsed_type_is_complex_value(const ParsedType* t) {
    return t && (t->isComplex || t->isImaginary);
}

bool cg_llvm_type_is_complex_value(LLVMTypeRef ty) {
    if (!ty || LLVMGetTypeKind(ty) != LLVMStructTypeKind) {
        return false;
    }
    if (LLVMCountStructElementTypes(ty) != 2) {
        return false;
    }
    LLVMTypeRef elems[2] = {0};
    LLVMGetStructElementTypes(ty, elems);
    if (!elems[0] || !elems[1]) {
        return false;
    }
    if (elems[0] != elems[1]) {
        return false;
    }
    LLVMTypeKind kind = LLVMGetTypeKind(elems[0]);
    return kind == LLVMHalfTypeKind || kind == LLVMFloatTypeKind ||
           kind == LLVMDoubleTypeKind || kind == LLVMX86_FP80TypeKind ||
           kind == LLVMFP128TypeKind || kind == LLVMPPC_FP128TypeKind;
}

LLVMTypeRef cg_complex_element_type_from_llvm(LLVMTypeRef complexType) {
    if (!cg_llvm_type_is_complex_value(complexType)) {
        return NULL;
    }
    LLVMTypeRef elems[2] = {0};
    LLVMGetStructElementTypes(complexType, elems);
    return elems[0];
}

LLVMTypeRef cg_complex_element_type(CodegenContext* ctx, const ParsedType* type) {
    if (!type) {
        return LLVMDoubleTypeInContext(ctx->llvmContext);
    }
    switch (type->primitiveType) {
        case TOKEN_FLOAT:
            return LLVMFloatTypeInContext(ctx->llvmContext);
        case TOKEN_DOUBLE:
        default:
            if (type->isLong) {
                const TargetLayout* tl = cg_context_get_target_layout(ctx);
                if (tl && tl->longDoubleBits == 128) {
                    return LLVMFP128TypeInContext(ctx->llvmContext);
                }
                if (tl && tl->longDoubleBits == 64) {
                    return LLVMDoubleTypeInContext(ctx->llvmContext);
                }
                LLVMTypeRef fp80 = LLVMX86FP80TypeInContext(ctx->llvmContext);
                if (fp80) {
                    return fp80;
                }
            }
            return LLVMDoubleTypeInContext(ctx->llvmContext);
    }
}

LLVMValueRef cg_build_complex_value(CodegenContext* ctx,
                                    LLVMValueRef real,
                                    LLVMValueRef imag,
                                    LLVMTypeRef complexType,
                                    const char* nameHint) {
    LLVMValueRef tmp = LLVMGetUndef(complexType);
    tmp = LLVMBuildInsertValue(ctx->builder, tmp, real, 0, nameHint ? nameHint : "complex.r");
    tmp = LLVMBuildInsertValue(ctx->builder, tmp, imag, 1, nameHint ? nameHint : "complex.i");
    return tmp;
}

LLVMValueRef cg_promote_to_complex(CodegenContext* ctx,
                                   LLVMValueRef value,
                                   const ParsedType* parsed,
                                   LLVMTypeRef complexType,
                                   LLVMTypeRef elemType) {
    if (LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMStructTypeKind) {
        return value;
    }
    LLVMValueRef casted = value;
    if (LLVMTypeOf(value) != elemType) {
        casted = cg_cast_value(ctx, value, elemType, parsed, NULL, "complex.scalar.cast");
    }
    if (parsed && parsed->isImaginary) {
        return cg_build_complex_value(ctx,
                                      LLVMConstNull(elemType),
                                      casted,
                                      complexType,
                                      "complex.imag");
    }
    return cg_build_complex_value(ctx,
                                  casted,
                                  LLVMConstNull(elemType),
                                  complexType,
                                  "complex.real");
}

void cg_promote_integer_operands(CodegenContext* ctx,
                                 LLVMValueRef* lhsValue,
                                 LLVMValueRef* rhsValue,
                                 LLVMTypeRef* lhsType,
                                 LLVMTypeRef* rhsType,
                                 bool lhsUnsigned,
                                 bool rhsUnsigned) {
    if (!ctx || !lhsValue || !rhsValue || !lhsType || !rhsType) return;
    if (!*lhsType || !*rhsType) return;
    if (LLVMGetTypeKind(*lhsType) != LLVMIntegerTypeKind ||
        LLVMGetTypeKind(*rhsType) != LLVMIntegerTypeKind) {
        return;
    }

    unsigned lhsBits = LLVMGetIntTypeWidth(*lhsType);
    unsigned rhsBits = LLVMGetIntTypeWidth(*rhsType);
    unsigned targetBits = lhsBits < 32 ? 32 : lhsBits;
    if (rhsBits > targetBits) {
        targetBits = rhsBits;
    }
    if (targetBits < 32) {
        targetBits = 32;
    }

    if (lhsBits == targetBits && rhsBits == targetBits) {
        return;
    }

    LLVMTypeRef targetType = LLVMIntTypeInContext(ctx->llvmContext, targetBits);
    if (lhsBits != targetBits) {
        *lhsValue = LLVMBuildIntCast2(ctx->builder, *lhsValue, targetType, !lhsUnsigned, "int.promote.l");
        *lhsType = targetType;
    }
    if (rhsBits != targetBits) {
        *rhsValue = LLVMBuildIntCast2(ctx->builder, *rhsValue, targetType, !rhsUnsigned, "int.promote.r");
        *rhsType = targetType;
    }
}

bool cg_is_unsigned_parsed(const ParsedType* type, LLVMTypeRef llvmType) {
    if (type) {
        if (type->kind == TYPE_ENUM) {
            return false;
        }
        if (type->kind == TYPE_PRIMITIVE) {
            if (type->primitiveType == TOKEN_BOOL) {
                return true;
            }
            return type->isUnsigned;
        }
        if (type->kind == TYPE_NAMED) {
            return type->isUnsigned;
        }
    }
    if (llvmType && LLVMGetTypeKind(llvmType) == LLVMIntegerTypeKind &&
        LLVMGetIntTypeWidth(llvmType) == 1) {
        return true;
    }
    return false;
}

LLVMAtomicOrdering cg_atomic_order_from_builtin_arg(LLVMValueRef orderArg,
                                                    LLVMAtomicOrdering fallback,
                                                    bool forLoad,
                                                    bool forStore) {
    LLVMAtomicOrdering ordering = fallback;
    if (ordering == LLVMAtomicOrderingNotAtomic || ordering == LLVMAtomicOrderingUnordered) {
        ordering = LLVMAtomicOrderingSequentiallyConsistent;
    }
    if (orderArg && LLVMIsAConstantInt(orderArg)) {
        long long rawOrder = LLVMConstIntGetSExtValue(orderArg);
        switch (rawOrder) {
            case 0: /* memory_order_relaxed */
                ordering = LLVMAtomicOrderingMonotonic;
                break;
            case 1: /* memory_order_consume */
            case 2: /* memory_order_acquire */
                ordering = LLVMAtomicOrderingAcquire;
                break;
            case 3: /* memory_order_release */
                ordering = LLVMAtomicOrderingRelease;
                break;
            case 4: /* memory_order_acq_rel */
                ordering = LLVMAtomicOrderingAcquireRelease;
                break;
            case 5: /* memory_order_seq_cst */
                ordering = LLVMAtomicOrderingSequentiallyConsistent;
                break;
            default:
                break;
        }
    }

    if (forLoad) {
        if (ordering == LLVMAtomicOrderingRelease || ordering == LLVMAtomicOrderingAcquireRelease) {
            ordering = LLVMAtomicOrderingAcquire;
        }
    } else if (forStore) {
        if (ordering == LLVMAtomicOrderingAcquire || ordering == LLVMAtomicOrderingAcquireRelease) {
            ordering = LLVMAtomicOrderingRelease;
        }
    }

    if (ordering == LLVMAtomicOrderingNotAtomic || ordering == LLVMAtomicOrderingUnordered) {
        ordering = LLVMAtomicOrderingSequentiallyConsistent;
    }
    return ordering;
}

LLVMValueRef cg_atomic_cast_value(CodegenContext* ctx,
                                  LLVMValueRef value,
                                  LLVMTypeRef targetType,
                                  ASTNode* valueNode,
                                  const char* nameHint) {
    if (!ctx || !value || !targetType) {
        return NULL;
    }
    if (LLVMTypeOf(value) == targetType) {
        return value;
    }

    LLVMTypeKind srcKind = LLVMGetTypeKind(LLVMTypeOf(value));
    LLVMTypeKind dstKind = LLVMGetTypeKind(targetType);
    if (srcKind == LLVMPointerTypeKind && dstKind == LLVMPointerTypeKind) {
        return LLVMBuildBitCast(ctx->builder,
                                value,
                                targetType,
                                nameHint ? nameHint : "atomic.ptr.cast");
    }
    if (srcKind == LLVMIntegerTypeKind && dstKind == LLVMIntegerTypeKind) {
        bool isUnsigned = valueNode ? cg_expression_is_unsigned(ctx, valueNode) : false;
        return LLVMBuildIntCast2(ctx->builder,
                                 value,
                                 targetType,
                                 !isUnsigned,
                                 nameHint ? nameHint : "atomic.int.cast");
    }

    const ParsedType* fromParsed = valueNode ? cg_resolve_expression_type(ctx, valueNode) : NULL;
    return cg_cast_value(ctx,
                         value,
                         targetType,
                         fromParsed,
                         NULL,
                         nameHint ? nameHint : "atomic.value.cast");
}

LLVMValueRef cg_atomic_cast_call_result(CodegenContext* ctx,
                                        ASTNode* callNode,
                                        LLVMValueRef value,
                                        const char* nameHint) {
    if (!ctx || !callNode || !value) {
        return value;
    }
    const ParsedType* callParsed = cg_resolve_expression_type(ctx, callNode);
    LLVMTypeRef targetType = callParsed ? cg_type_from_parsed(ctx, callParsed) : NULL;
    if (!targetType || targetType == LLVMTypeOf(value) ||
        LLVMGetTypeKind(targetType) == LLVMVoidTypeKind) {
        return value;
    }
    return cg_cast_value(ctx,
                         value,
                         targetType,
                         NULL,
                         callParsed,
                         nameHint ? nameHint : "atomic.result.cast");
}

LLVMValueRef cg_atomic_cast_pointer(CodegenContext* ctx,
                                    LLVMValueRef pointerValue,
                                    LLVMTypeRef elementType,
                                    const char* nameHint) {
    if (!ctx || !pointerValue || !elementType) {
        return NULL;
    }
    LLVMTypeRef ptrType = LLVMTypeOf(pointerValue);
    if (!ptrType || LLVMGetTypeKind(ptrType) != LLVMPointerTypeKind) {
        return NULL;
    }

    LLVMTypeRef expectedPtrType = LLVMPointerType(elementType, 0);
    if (ptrType == expectedPtrType) {
        return pointerValue;
    }
    if (LLVMPointerTypeIsOpaque(ptrType)) {
        return pointerValue;
    }
    return LLVMBuildBitCast(ctx->builder,
                            pointerValue,
                            expectedPtrType,
                            nameHint ? nameHint : "atomic.ptr.cast");
}

LLVMValueRef cg_build_complex_addsub(CodegenContext* ctx,
                                     LLVMValueRef lhs,
                                     LLVMValueRef rhs,
                                     LLVMTypeRef complexType,
                                     bool isSub) {
    LLVMValueRef lhsReal = LLVMBuildExtractValue(ctx->builder, lhs, 0, "complex.lhs.r");
    LLVMValueRef lhsImag = LLVMBuildExtractValue(ctx->builder, lhs, 1, "complex.lhs.i");
    LLVMValueRef rhsReal = LLVMBuildExtractValue(ctx->builder, rhs, 0, "complex.rhs.r");
    LLVMValueRef rhsImag = LLVMBuildExtractValue(ctx->builder, rhs, 1, "complex.rhs.i");
    LLVMValueRef real = isSub
        ? LLVMBuildFSub(ctx->builder, lhsReal, rhsReal, "complex.sub.r")
        : LLVMBuildFAdd(ctx->builder, lhsReal, rhsReal, "complex.add.r");
    LLVMValueRef imag = isSub
        ? LLVMBuildFSub(ctx->builder, lhsImag, rhsImag, "complex.sub.i")
        : LLVMBuildFAdd(ctx->builder, lhsImag, rhsImag, "complex.add.i");
    return cg_build_complex_value(ctx, real, imag, complexType, "complex.addsub");
}

LLVMValueRef cg_build_complex_muldiv(CodegenContext* ctx,
                                     LLVMValueRef lhs,
                                     LLVMValueRef rhs,
                                     LLVMTypeRef complexType,
                                     bool isDiv) {
    LLVMValueRef lhsReal = LLVMBuildExtractValue(ctx->builder, lhs, 0, "complex.lhs.r");
    LLVMValueRef lhsImag = LLVMBuildExtractValue(ctx->builder, lhs, 1, "complex.lhs.i");
    LLVMValueRef rhsReal = LLVMBuildExtractValue(ctx->builder, rhs, 0, "complex.rhs.r");
    LLVMValueRef rhsImag = LLVMBuildExtractValue(ctx->builder, rhs, 1, "complex.rhs.i");

    if (!isDiv) {
        LLVMValueRef ac = LLVMBuildFMul(ctx->builder, lhsReal, rhsReal, "complex.mul.ac");
        LLVMValueRef bd = LLVMBuildFMul(ctx->builder, lhsImag, rhsImag, "complex.mul.bd");
        LLVMValueRef ad = LLVMBuildFMul(ctx->builder, lhsReal, rhsImag, "complex.mul.ad");
        LLVMValueRef bc = LLVMBuildFMul(ctx->builder, lhsImag, rhsReal, "complex.mul.bc");
        LLVMValueRef real = LLVMBuildFSub(ctx->builder, ac, bd, "complex.mul.r");
        LLVMValueRef imag = LLVMBuildFAdd(ctx->builder, ad, bc, "complex.mul.i");
        return cg_build_complex_value(ctx, real, imag, complexType, "complex.mul");
    }

    LLVMValueRef rr = LLVMBuildFMul(ctx->builder, rhsReal, rhsReal, "complex.div.rr");
    LLVMValueRef ii = LLVMBuildFMul(ctx->builder, rhsImag, rhsImag, "complex.div.ii");
    LLVMValueRef denom = LLVMBuildFAdd(ctx->builder, rr, ii, "complex.div.den");
    LLVMValueRef ac = LLVMBuildFMul(ctx->builder, lhsReal, rhsReal, "complex.div.ac");
    LLVMValueRef bd = LLVMBuildFMul(ctx->builder, lhsImag, rhsImag, "complex.div.bd");
    LLVMValueRef bc = LLVMBuildFMul(ctx->builder, lhsImag, rhsReal, "complex.div.bc");
    LLVMValueRef ad = LLVMBuildFMul(ctx->builder, lhsReal, rhsImag, "complex.div.ad");
    LLVMValueRef numerReal = LLVMBuildFAdd(ctx->builder, ac, bd, "complex.div.nr");
    LLVMValueRef numerImag = LLVMBuildFSub(ctx->builder, bc, ad, "complex.div.ni");
    LLVMValueRef real = LLVMBuildFDiv(ctx->builder, numerReal, denom, "complex.div.r");
    LLVMValueRef imag = LLVMBuildFDiv(ctx->builder, numerImag, denom, "complex.div.i");
    return cg_build_complex_value(ctx, real, imag, complexType, "complex.div");
}

LLVMValueRef cg_build_complex_eq(CodegenContext* ctx,
                                 LLVMValueRef lhs,
                                 LLVMValueRef rhs,
                                 bool isNotEqual) {
    LLVMValueRef lhsReal = LLVMBuildExtractValue(ctx->builder, lhs, 0, "complex.lhs.r");
    LLVMValueRef lhsImag = LLVMBuildExtractValue(ctx->builder, lhs, 1, "complex.lhs.i");
    LLVMValueRef rhsReal = LLVMBuildExtractValue(ctx->builder, rhs, 0, "complex.rhs.r");
    LLVMValueRef rhsImag = LLVMBuildExtractValue(ctx->builder, rhs, 1, "complex.rhs.i");
    LLVMValueRef realCmp = LLVMBuildFCmp(ctx->builder, LLVMRealOEQ, lhsReal, rhsReal, "complex.eq.r");
    LLVMValueRef imagCmp = LLVMBuildFCmp(ctx->builder, LLVMRealOEQ, lhsImag, rhsImag, "complex.eq.i");
    LLVMValueRef both = LLVMBuildAnd(ctx->builder, realCmp, imagCmp, "complex.eq.and");
    if (isNotEqual) {
        both = LLVMBuildNot(ctx->builder, both, "complex.ne");
    }
    return cg_widen_bool_to_int(ctx, both, "complex.eq.int");
}

static LLVMValueRef cg_build_bitfield_mask(CodegenContext* ctx, LLVMTypeRef storageTy, unsigned width) {
    (void)ctx;
    unsigned storageBits = LLVMGetIntTypeWidth(storageTy);
    if (width == 0) {
        return LLVMConstInt(storageTy, 0, 0);
    }
    if (width >= storageBits) {
        return LLVMConstAllOnes(storageTy);
    }
    uint64_t mask = (width >= 64) ? ~0ULL : ((1ULL << width) - 1ULL);
    return LLVMConstInt(storageTy, mask, 0);
}

LLVMValueRef cg_load_bitfield(CodegenContext* ctx,
                              const CGLValueInfo* info,
                              LLVMTypeRef resultType) {
    if (!info || !info->isBitfield) return NULL;
    LLVMValueRef raw = LLVMBuildLoad2(ctx->builder, info->storageType, info->storagePtr, "bf.load");
    unsigned bitOffset = (unsigned)info->layout.bitOffset;
    LLVMTypeRef storageTy = info->storageType;
    LLVMValueRef shifted = raw;
    if (bitOffset > 0) {
        LLVMValueRef sh = LLVMConstInt(storageTy, bitOffset, 0);
        shifted = LLVMBuildLShr(ctx->builder, raw, sh, "bf.shr");
    }
    LLVMValueRef mask = cg_build_bitfield_mask(ctx, storageTy, (unsigned)info->layout.widthBits);
    LLVMValueRef masked = LLVMBuildAnd(ctx->builder, shifted, mask, "bf.mask");
    if (!resultType) {
        unsigned width = info->layout.widthBits ? (unsigned)info->layout.widthBits : LLVMGetIntTypeWidth(storageTy);
        if (width == 0) width = LLVMGetIntTypeWidth(storageTy);
        resultType = LLVMIntTypeInContext(ctx->llvmContext, width);
    }
    if (LLVMGetTypeKind(resultType) != LLVMIntegerTypeKind) {
        return LLVMBuildBitCast(ctx->builder, masked, resultType, "bf.cast");
    }
    unsigned destBits = LLVMGetIntTypeWidth(resultType);
    if (destBits > LLVMGetIntTypeWidth(storageTy)) {
        if (info->layout.isSigned) {
            return LLVMBuildSExt(ctx->builder, masked, resultType, "bf.sext");
        }
        return LLVMBuildZExt(ctx->builder, masked, resultType, "bf.zext");
    }
    if (destBits < LLVMGetIntTypeWidth(storageTy)) {
        return LLVMBuildTrunc(ctx->builder, masked, resultType, "bf.trunc");
    }
    return masked;
}

bool cg_store_bitfield(CodegenContext* ctx,
                       const CGLValueInfo* info,
                       LLVMValueRef value) {
    if (!info || !info->isBitfield) return false;
    LLVMTypeRef storageTy = info->storageType;
    unsigned bitOffset = (unsigned)info->layout.bitOffset;
    LLVMValueRef mask = cg_build_bitfield_mask(ctx, storageTy, (unsigned)info->layout.widthBits);

    LLVMValueRef casted = LLVMBuildIntCast2(ctx->builder, value, storageTy, info->layout.isSigned, "bf.cast.storage");
    if (info->layout.widthBits < LLVMGetIntTypeWidth(storageTy)) {
        casted = LLVMBuildAnd(ctx->builder, casted, mask, "bf.truncmask");
    }
    LLVMValueRef shifted = casted;
    if (bitOffset > 0) {
        LLVMValueRef sh = LLVMConstInt(storageTy, bitOffset, 0);
        shifted = LLVMBuildShl(ctx->builder, casted, sh, "bf.shl");
    }
    LLVMValueRef shiftedMask = mask;
    if (bitOffset > 0) {
        LLVMValueRef sh = LLVMConstInt(storageTy, bitOffset, 0);
        shiftedMask = LLVMBuildShl(ctx->builder, mask, sh, "bf.mask.shl");
    }
    LLVMValueRef oldVal = LLVMBuildLoad2(ctx->builder, storageTy, info->storagePtr, "bf.old");
    LLVMValueRef notMask = LLVMBuildNot(ctx->builder, shiftedMask, "bf.notmask");
    LLVMValueRef cleared = LLVMBuildAnd(ctx->builder, oldVal, notMask, "bf.cleared");
    LLVMValueRef combined = LLVMBuildOr(ctx->builder, cleared, shifted, "bf.combined");
    LLVMBuildStore(ctx->builder, combined, info->storagePtr);
    return true;
}

LLVMValueRef ensureIntegerLike(CodegenContext* ctx, LLVMValueRef value) {
    if (!value) return NULL;
    LLVMTypeRef ty = LLVMTypeOf(value);
    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
    if (ty && LLVMGetTypeKind(ty) == LLVMIntegerTypeKind) {
        unsigned tyBits = LLVMGetIntTypeWidth(ty);
        unsigned intptrBits = LLVMGetIntTypeWidth(intptrTy);
        if (tyBits != intptrBits) {
            return LLVMBuildIntCast2(ctx->builder, value, intptrTy, 0, "vla.intcast");
        }
        return value;
    }
    if (ty && LLVMGetTypeKind(ty) == LLVMPointerTypeKind) {
        return LLVMBuildPtrToInt(ctx->builder, value, intptrTy, "vla.ptrtoint");
    }
    return LLVMBuildIntCast2(ctx->builder, value, intptrTy, 0, "vla.intcast");
}

bool cg_is_float_type(LLVMTypeRef type) {
    if (!type) return false;
    switch (LLVMGetTypeKind(type)) {
        case LLVMHalfTypeKind:
        case LLVMFloatTypeKind:
        case LLVMDoubleTypeKind:
        case LLVMX86_FP80TypeKind:
        case LLVMFP128TypeKind:
            return true;
        default:
            return false;
    }
}

unsigned cg_float_rank_from_kind(LLVMTypeKind kind) {
    switch (kind) {
        case LLVMHalfTypeKind: return 1;
        case LLVMFloatTypeKind: return 2;
        case LLVMDoubleTypeKind: return 3;
        case LLVMX86_FP80TypeKind: return 4;
        case LLVMFP128TypeKind: return 5;
        default: return 0;
    }
}

bool cg_is_external_decl_function(LLVMValueRef function) {
    if (!function) return false;
    if (!LLVMIsAFunction(function)) return false;
    return LLVMCountBasicBlocks(function) == 0;
}

bool cg_is_known_external_abi_function_name(const char* name) {
    if (!name || !*name) return false;
    return strncmp(name, "SDL_", 4) == 0 ||
           strncmp(name, "TTF_", 4) == 0;
}

LLVMTypeRef cg_external_abi_coerce_param_type(CodegenContext* ctx, LLVMTypeRef paramType) {
    if (!ctx || !paramType) return paramType;
    LLVMTypeKind kind = LLVMGetTypeKind(paramType);
    if (kind != LLVMStructTypeKind && kind != LLVMArrayTypeKind) {
        return paramType;
    }
    LLVMTargetDataRef td = ctx->module ? LLVMGetModuleDataLayout(ctx->module) : NULL;
    if (!td) return paramType;
    uint64_t size = LLVMABISizeOfType(td, paramType);
    if (size == 0 || size > 16) {
        return paramType;
    }
    if (size <= 8) {
        return LLVMInt64TypeInContext(ctx->llvmContext);
    }
    return LLVMArrayType(LLVMInt64TypeInContext(ctx->llvmContext), 2);
}

LLVMValueRef cg_pack_aggregate_for_external_abi(CodegenContext* ctx,
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

    LLVMValueRef slot = cg_build_entry_alloca(ctx, packedType, "call.extabi.pack.slot");
    if (!slot) return value;
    LLVMBuildStore(ctx->builder, LLVMConstNull(packedType), slot);

    LLVMTypeRef sourcePtrType = LLVMPointerType(sourceType, 0);
    LLVMValueRef sourceAddr = LLVMBuildBitCast(ctx->builder, slot, sourcePtrType, "call.extabi.pack.addr");
    LLVMBuildStore(ctx->builder, value, sourceAddr);

    return LLVMBuildLoad2(ctx->builder,
                          packedType,
                          slot,
                          nameHint ? nameHint : "call.extabi.pack");
}

LLVMTypeRef cg_select_float_type(LLVMTypeRef lhs, LLVMTypeRef rhs, LLVMContextRef ctx) {
    if (!lhs || !rhs) return lhs ? lhs : rhs;
    LLVMTypeKind lhsKind = LLVMGetTypeKind(lhs);
    LLVMTypeKind rhsKind = LLVMGetTypeKind(rhs);
    unsigned lhsRank = cg_float_rank_from_kind(lhsKind);
    unsigned rhsRank = cg_float_rank_from_kind(rhsKind);
    if (lhsRank == 0 && rhsRank == 0) {
        return LLVMDoubleTypeInContext(ctx);
    }
    if (lhsRank == 0) return rhs;
    if (rhsRank == 0) return lhs;
    return lhsRank >= rhsRank ? lhs : rhs;
}

LLVMTypeRef vlaInnermostElementLLVM(CodegenContext* ctx, const ParsedType* type) {
    if (!type) return LLVMInt32TypeInContext(ctx->llvmContext);
    ParsedType base = parsedTypeClone(type);
    while (parsedTypeIsDirectArray(&base)) {
        ParsedType next = parsedTypeArrayElementType(&base);
        parsedTypeFree(&base);
        base = next;
    }
    LLVMTypeRef elem = cg_type_from_parsed(ctx, &base);
    parsedTypeFree(&base);
    if (!elem || LLVMGetTypeKind(elem) == LLVMVoidTypeKind) {
        elem = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    return elem;
}

LLVMValueRef computeVLAElementCount(CodegenContext* ctx, const ParsedType* type) {
    if (!ctx || !type) return NULL;
    LLVMValueRef total = NULL;
    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);

    for (size_t i = 0; i < type->derivationCount; ++i) {
        const TypeDerivation* deriv = parsedTypeGetDerivation(type, i);
        if (!deriv || deriv->kind != TYPE_DERIVATION_ARRAY) continue;

        LLVMValueRef dimValue = NULL;
        if (!deriv->as.array.isVLA && deriv->as.array.hasConstantSize && deriv->as.array.constantSize > 0) {
            dimValue = LLVMConstInt(intptrTy, (unsigned long long)deriv->as.array.constantSize, 0);
        } else if (deriv->as.array.sizeExpr) {
            LLVMValueRef evaluated = codegenNode(ctx, deriv->as.array.sizeExpr);
            dimValue = ensureIntegerLike(ctx, evaluated);
        }
        if (!dimValue) {
            dimValue = LLVMConstInt(intptrTy, 1, 0);
        }
        if (!total) {
            total = dimValue;
        } else {
            total = LLVMBuildMul(ctx->builder, total, dimValue, "vla.total");
        }
    }
    return total ? total : LLVMConstInt(intptrTy, 1, 0);
}

ParsedType functionReturnTypeAtDerivation(const ParsedType* type, size_t functionIndex) {
    ParsedType invalid = {0};
    invalid.kind = TYPE_INVALID;
    if (!type) {
        return invalid;
    }
    ParsedType trimmed = parsedTypeClone(type);
    if (trimmed.kind == TYPE_INVALID || functionIndex >= trimmed.derivationCount) {
        parsedTypeFree(&trimmed);
        return invalid;
    }

    size_t removedPointers = 0;
    for (size_t i = 0; i < functionIndex; ++i) {
        const TypeDerivation* d = parsedTypeGetDerivation(&trimmed, i);
        if (d && d->kind == TYPE_DERIVATION_POINTER) {
            removedPointers++;
        }
    }
    if (functionIndex > 0) {
        memmove(trimmed.derivations,
                trimmed.derivations + functionIndex,
                (trimmed.derivationCount - functionIndex) * sizeof(TypeDerivation));
        trimmed.derivationCount -= functionIndex;
    }
    if (removedPointers > 0 && trimmed.pointerDepth > 0) {
        int dec = (int)removedPointers;
        trimmed.pointerDepth = trimmed.pointerDepth > dec ? (trimmed.pointerDepth - dec) : 0;
    }

    ParsedType ret = parsedTypeFunctionReturnType(&trimmed);
    parsedTypeFree(&trimmed);
    return ret;
}

LLVMTypeRef functionTypeFromPointerParsed(CodegenContext* ctx,
                                          const ParsedType* type,
                                          size_t fallbackArgCount,
                                          LLVMValueRef* args) {
    if (!ctx || !type) {
        return NULL;
    }
    const ParsedType* resolved = type;
    if (type->kind == TYPE_NAMED &&
        type->derivationCount == 0 &&
        type->pointerDepth == 0 &&
        !type->isFunctionPointer &&
        ctx->typeCache &&
        type->userTypeName) {
        CGNamedLLVMType* info = cg_type_cache_get_typedef_info(ctx->typeCache, type->userTypeName);
        if (info && info->parsedType.kind != TYPE_INVALID) {
            resolved = &info->parsedType;
        }
    }

    const ParsedType* paramList = NULL;
    size_t paramCount = 0;
    bool isVariadic = resolved->isVariadicFunction;
    size_t functionDerivIndex = (size_t)-1;
    bool haveCallableSignature = false;
    bool usingFpParams = false;

    /* Prefer explicit function derivations and select the best callable layer
     * for this call site (exact arg-count match when possible). */
    size_t bestDistance = (size_t)-1;
    for (size_t i = 0; i < resolved->derivationCount; ++i) {
        const TypeDerivation* deriv = parsedTypeGetDerivation(resolved, i);
        if (!deriv || deriv->kind != TYPE_DERIVATION_FUNCTION) {
            continue;
        }
        size_t candidateCount = deriv->as.function.paramCount;
        size_t distance = (candidateCount > fallbackArgCount)
            ? (candidateCount - fallbackArgCount)
            : (fallbackArgCount - candidateCount);
        if (functionDerivIndex == (size_t)-1 || distance < bestDistance) {
            functionDerivIndex = i;
            bestDistance = distance;
        }
        if (distance == 0) {
            break;
        }
    }
    if (functionDerivIndex != (size_t)-1) {
        const TypeDerivation* deriv = parsedTypeGetDerivation(resolved, functionDerivIndex);
        paramList = deriv ? deriv->as.function.params : NULL;
        paramCount = deriv ? deriv->as.function.paramCount : 0;
        isVariadic = deriv ? deriv->as.function.isVariadic : false;
        haveCallableSignature = true;
    } else if (resolved->isFunctionPointer || resolved->fpParamCount > 0) {
        paramList = resolved->fpParams;
        paramCount = resolved->fpParamCount;
        haveCallableSignature = true;
        usingFpParams = true;
    }
    if (!haveCallableSignature) {
        return NULL;
    }

    /* Some typedef-heavy nested function-pointer shapes only expose fpParams
     * for the innermost callable layer. If that disagrees with this call site,
     * fall back to argument-derived parameter types to preserve ABI argument
     * passing for the actual callee expression. */
    if (usingFpParams &&
        fallbackArgCount > 0 &&
        paramCount > 0 &&
        paramCount != fallbackArgCount) {
        paramList = NULL;
        paramCount = 0;
        isVariadic = false;
    }

    LLVMTypeRef returnType = NULL;
    if (resolved) {
        ParsedType retParsed = (functionDerivIndex != (size_t)-1)
            ? functionReturnTypeAtDerivation(resolved, functionDerivIndex)
            : parsedTypeFunctionReturnType(resolved);
        if (retParsed.kind != TYPE_INVALID) {
            returnType = cg_type_from_parsed(ctx, &retParsed);
        }
        parsedTypeFree(&retParsed);
    }

    if (!returnType) {
        switch (resolved->primitiveType) {
            case TOKEN_INT:    returnType = LLVMInt32TypeInContext(ctx->llvmContext); break;
            case TOKEN_CHAR:   returnType = LLVMInt8TypeInContext(ctx->llvmContext);  break;
            case TOKEN_BOOL:   returnType = LLVMInt1TypeInContext(ctx->llvmContext);  break;
            case TOKEN_VOID:   returnType = LLVMVoidTypeInContext(ctx->llvmContext);  break;
            default: break;
        }
    }
    if (returnType && LLVMGetTypeKind(returnType) == LLVMFunctionTypeKind) {
        returnType = LLVMPointerType(returnType, 0);
    } else if (returnType && LLVMGetTypeKind(returnType) == LLVMArrayTypeKind) {
        returnType = LLVMPointerType(returnType, 0);
    }
    if (!returnType || LLVMGetTypeKind(returnType) == LLVMVoidTypeKind) {
        returnType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    returnType = cg_coerce_function_return_type(ctx, returnType);

    size_t count = paramCount ? paramCount : fallbackArgCount;
    LLVMTypeRef* params = NULL;
    if (count > 0) {
        params = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * count);
        if (!params) return NULL;
        for (size_t i = 0; i < count; ++i) {
            if (paramCount > 0 && paramList) {
                params[i] = cg_lower_parameter_type(ctx, &paramList[i], NULL, NULL);
            } else if (args && i < fallbackArgCount) {
                params[i] = args[i] ? LLVMTypeOf(args[i]) : NULL;
            }
            if (!params[i]) {
                params[i] = LLVMInt32TypeInContext(ctx->llvmContext);
            }
        }
    }

    LLVMTypeRef fnType = LLVMFunctionType(returnType, params, (unsigned)count, isVariadic ? 1 : 0);
    free(params);
    return fnType;
}

bool cg_function_signatures_compatible(const Symbol* lhs, const Symbol* rhs) {
    if (!lhs || !rhs || lhs->kind != SYMBOL_FUNCTION || rhs->kind != SYMBOL_FUNCTION) {
        return false;
    }
    if (!parsedTypesStructurallyEqual(&lhs->type, &rhs->type)) {
        return false;
    }
    if (lhs->signature.hasPrototype != rhs->signature.hasPrototype) {
        return false;
    }
    if (lhs->signature.isVariadic != rhs->signature.isVariadic) {
        return false;
    }
    if (lhs->signature.paramCount != rhs->signature.paramCount) {
        return false;
    }
    for (size_t i = 0; i < lhs->signature.paramCount; ++i) {
        if (!parsedTypesStructurallyEqual(&lhs->signature.params[i], &rhs->signature.params[i])) {
            return false;
        }
    }
    return true;
}

const Symbol* cg_lookup_function_symbol_for_callee(CodegenContext* ctx, ASTNode* callee) {
    if (!ctx || !callee) {
        return NULL;
    }
    const SemanticModel* model = cg_context_get_semantic_model(ctx);
    if (!model) {
        return NULL;
    }

    if (callee->type == AST_IDENTIFIER) {
        const char* name = callee->valueNode.value;
        if (!name) {
            return NULL;
        }
        const Symbol* sym = semanticModelLookupGlobal(model, name);
        if (!sym || sym->kind != SYMBOL_FUNCTION) {
            return NULL;
        }
        return sym;
    }

    if (callee->type == AST_TERNARY_EXPRESSION) {
        const Symbol* lhs = cg_lookup_function_symbol_for_callee(ctx, callee->ternaryExpr.trueExpr);
        const Symbol* rhs = cg_lookup_function_symbol_for_callee(ctx, callee->ternaryExpr.falseExpr);
        if (lhs && rhs && (lhs == rhs || cg_function_signatures_compatible(lhs, rhs))) {
            return lhs;
        }
    }

    return NULL;
}

const Symbol* cg_lookup_global_function_symbol_by_name(CodegenContext* ctx, const char* name) {
    if (!ctx || !name || !name[0]) {
        return NULL;
    }
    const SemanticModel* model = cg_context_get_semantic_model(ctx);
    if (!model) {
        return NULL;
    }
    const Symbol* sym = semanticModelLookupGlobal(model, name);
    if (!sym || sym->kind != SYMBOL_FUNCTION) {
        return NULL;
    }
    return sym;
}

LLVMTypeRef cg_function_type_from_symbol(CodegenContext* ctx, const Symbol* sym) {
    if (!ctx || !sym || sym->kind != SYMBOL_FUNCTION) {
        return NULL;
    }

    LLVMTypeRef retType = cg_type_from_parsed(ctx, &sym->type);
    if (retType && LLVMGetTypeKind(retType) == LLVMFunctionTypeKind) {
        retType = LLVMPointerType(retType, 0);
    } else if (retType && LLVMGetTypeKind(retType) == LLVMArrayTypeKind) {
        retType = LLVMPointerType(retType, 0);
    }
    if (!retType || LLVMGetTypeKind(retType) == LLVMVoidTypeKind) {
        retType = LLVMVoidTypeInContext(ctx->llvmContext);
    }
    retType = cg_coerce_function_return_type(ctx, retType);
    size_t paramCount = sym->signature.paramCount;
    LLVMTypeRef* paramTypes = NULL;
    if (paramCount > 0) {
        paramTypes = (LLVMTypeRef*)calloc(paramCount, sizeof(LLVMTypeRef));
        if (!paramTypes) {
            return NULL;
        }
        for (size_t i = 0; i < paramCount; ++i) {
            paramTypes[i] = cg_lower_parameter_type(ctx, &sym->signature.params[i], NULL, NULL);
            if (!paramTypes[i] || LLVMGetTypeKind(paramTypes[i]) == LLVMVoidTypeKind) {
                paramTypes[i] = LLVMInt32TypeInContext(ctx->llvmContext);
            }
        }
    }

    LLVMTypeRef fnType = LLVMFunctionType(retType,
                                          paramTypes,
                                          (unsigned)paramCount,
                                          sym->signature.isVariadic ? 1 : 0);
    free(paramTypes);
    return fnType;
}
