#include "codegen_private.h"

#include "codegen_types.h"
#include "Parser/Helpers/parsed_type.h"
#include "literal_utils.h"
#include "Syntax/type_checker.h"
#include "Syntax/layout.h"
#include "Syntax/analyze_expr.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

static bool cg_is_builtin_bool_literal_name(const char* name) {
    if (!name) return false;
    return strcmp(name, "true") == 0 || strcmp(name, "false") == 0;
}

static bool cg_parsed_type_is_pointerish(const ParsedType* t) {
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

static bool cg_eval_builtin_offsetof(CodegenContext* ctx,
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

static bool cg_parsed_type_is_complex_value(const ParsedType* t) {
    return t && (t->isComplex || t->isImaginary);
}

static bool cg_llvm_type_is_complex_value(LLVMTypeRef ty) {
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

static LLVMTypeRef cg_complex_element_type_from_llvm(LLVMTypeRef complexType) {
    if (!cg_llvm_type_is_complex_value(complexType)) {
        return NULL;
    }
    LLVMTypeRef elems[2] = {0};
    LLVMGetStructElementTypes(complexType, elems);
    return elems[0];
}

static LLVMTypeRef cg_complex_element_type(CodegenContext* ctx, const ParsedType* type) {
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

static LLVMValueRef cg_build_complex_value(CodegenContext* ctx,
                                           LLVMValueRef real,
                                           LLVMValueRef imag,
                                           LLVMTypeRef complexType,
                                           const char* nameHint) {
    LLVMValueRef tmp = LLVMGetUndef(complexType);
    tmp = LLVMBuildInsertValue(ctx->builder, tmp, real, 0, nameHint ? nameHint : "complex.r");
    tmp = LLVMBuildInsertValue(ctx->builder, tmp, imag, 1, nameHint ? nameHint : "complex.i");
    return tmp;
}

static LLVMValueRef cg_promote_to_complex(CodegenContext* ctx,
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

static void cg_promote_integer_operands(CodegenContext* ctx,
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

static bool cg_is_unsigned_parsed(const ParsedType* type, LLVMTypeRef llvmType) {
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

static LLVMValueRef cg_build_complex_addsub(CodegenContext* ctx,
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

static LLVMValueRef cg_build_complex_muldiv(CodegenContext* ctx,
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

static LLVMValueRef cg_build_complex_eq(CodegenContext* ctx,
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

static LLVMValueRef cg_load_bitfield(CodegenContext* ctx,
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

static bool cg_store_bitfield(CodegenContext* ctx,
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

static LLVMValueRef ensureIntegerLike(CodegenContext* ctx, LLVMValueRef value) {
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

static bool cg_is_float_type(LLVMTypeRef type) {
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

static unsigned cg_float_rank_from_kind(LLVMTypeKind kind) {
    switch (kind) {
        case LLVMHalfTypeKind: return 1;
        case LLVMFloatTypeKind: return 2;
        case LLVMDoubleTypeKind: return 3;
        case LLVMX86_FP80TypeKind: return 4;
        case LLVMFP128TypeKind: return 5;
        default: return 0;
    }
}

static bool cg_is_external_decl_function(LLVMValueRef function) {
    if (!function) return false;
    if (!LLVMIsAFunction(function)) return false;
    return LLVMCountBasicBlocks(function) == 0;
}

static bool cg_is_system_header_path(const char* file) {
    if (!file || file[0] != '/') return false;
    static const char* kPrefixes[] = {
        "/usr/include/",
        "/usr/local/include/",
        "/opt/homebrew/include/",
        "/Library/Developer/",
        "/Applications/Xcode.app/",
        "/Applications/Xcode-beta.app/",
        "/Library/Frameworks/",
        "/System/Library/"
    };
    for (size_t i = 0; i < sizeof(kPrefixes) / sizeof(kPrefixes[0]); ++i) {
        size_t n = strlen(kPrefixes[i]);
        if (strncmp(file, kPrefixes[i], n) == 0) {
            return true;
        }
    }
    return false;
}

static bool cg_symbol_declared_in_system_header(const Symbol* sym) {
    if (!sym || !sym->definition) return false;
    const char* defFile = sym->definition->location.start.file;
    if (cg_is_system_header_path(defFile)) {
        return true;
    }
    const char* macroFile = sym->definition->macroCallSite.start.file;
    if (cg_is_system_header_path(macroFile)) {
        return true;
    }
    return false;
}

static bool cg_is_known_external_abi_function_name(const char* name) {
    if (!name || !*name) return false;
    return strncmp(name, "SDL_", 4) == 0 ||
           strncmp(name, "TTF_", 4) == 0;
}

static LLVMTypeRef cg_external_abi_coerce_param_type(CodegenContext* ctx, LLVMTypeRef paramType) {
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

static LLVMValueRef cg_pack_aggregate_for_external_abi(CodegenContext* ctx,
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

static LLVMTypeRef cg_select_float_type(LLVMTypeRef lhs, LLVMTypeRef rhs, LLVMContextRef ctx) {
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

static LLVMTypeRef vlaInnermostElementLLVM(CodegenContext* ctx, const ParsedType* type) {
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

static LLVMValueRef computeVLAElementCount(CodegenContext* ctx, const ParsedType* type) {
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

static ParsedType functionReturnTypeAtDerivation(const ParsedType* type, size_t functionIndex) {
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

static LLVMTypeRef functionTypeFromPointerParsed(CodegenContext* ctx,
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

static bool cg_function_signatures_compatible(const Symbol* lhs, const Symbol* rhs) {
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

static const Symbol* cg_lookup_function_symbol_for_callee(CodegenContext* ctx, ASTNode* callee) {
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

static LLVMTypeRef cg_function_type_from_symbol(CodegenContext* ctx, const Symbol* sym) {
    if (!ctx || !sym || sym->kind != SYMBOL_FUNCTION) {
        return NULL;
    }

    ParsedType extractedReturn = parsedTypeFunctionReturnType(&sym->type);
    const ParsedType* returnParsed = &sym->type;
    if (extractedReturn.kind != TYPE_INVALID) {
        returnParsed = &extractedReturn;
    }

    LLVMTypeRef retType = cg_type_from_parsed(ctx, returnParsed);
    if (retType && LLVMGetTypeKind(retType) == LLVMFunctionTypeKind) {
        retType = LLVMPointerType(retType, 0);
    } else if (retType && LLVMGetTypeKind(retType) == LLVMArrayTypeKind) {
        retType = LLVMPointerType(retType, 0);
    }
    if (!retType || LLVMGetTypeKind(retType) == LLVMVoidTypeKind) {
        retType = LLVMVoidTypeInContext(ctx->llvmContext);
    }
    if (extractedReturn.kind != TYPE_INVALID) {
        parsedTypeFree(&extractedReturn);
    }

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

LLVMValueRef codegenBinaryExpression(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_BINARY_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenBinaryExpression\n");
        return NULL;
    }

    const char* op = node->expr.op ? node->expr.op : "";
    LLVMValueRef L = codegenNode(ctx, node->expr.left);
    if (!L) {
        fprintf(stderr, "Error: Failed to generate LHS for binary expression\n");
        return NULL;
    }
    LLVMValueRef Rdbg = NULL;
    (void)Rdbg;
    if (strcmp(op, "&&") == 0) {
        return codegenLogicalAndOr(ctx, L, node->expr.right, true);
    }
    if (strcmp(op, "||") == 0) {
        return codegenLogicalAndOr(ctx, L, node->expr.right, false);
    }

    LLVMValueRef R = codegenNode(ctx, node->expr.right);
    if (!R) {
        fprintf(stderr, "Error: Failed to generate RHS for binary expression\n");
        return NULL;
    }
    const ParsedType* lhsParsed = cg_resolve_expression_type(ctx, node->expr.left);
    const ParsedType* rhsParsed = cg_resolve_expression_type(ctx, node->expr.right);
    if (!lhsParsed &&
        node->expr.left &&
        node->expr.left->type == AST_UNARY_EXPRESSION &&
        node->expr.left->expr.op &&
        strcmp(node->expr.left->expr.op, "&") == 0) {
        const ParsedType* inner = cg_resolve_expression_type(ctx, node->expr.left->expr.left);
        if (inner) {
            static ParsedType lhsAddrFallback;
            parsedTypeFree(&lhsAddrFallback);
            lhsAddrFallback = parsedTypeClone(inner);
            if (lhsAddrFallback.kind != TYPE_INVALID) {
                parsedTypeAddPointerDepth(&lhsAddrFallback, 1);
                lhsParsed = &lhsAddrFallback;
            }
        }
    }
    if (!rhsParsed &&
        node->expr.right &&
        node->expr.right->type == AST_UNARY_EXPRESSION &&
        node->expr.right->expr.op &&
        strcmp(node->expr.right->expr.op, "&") == 0) {
        const ParsedType* inner = cg_resolve_expression_type(ctx, node->expr.right->expr.left);
        if (inner) {
            static ParsedType rhsAddrFallback;
            parsedTypeFree(&rhsAddrFallback);
            rhsAddrFallback = parsedTypeClone(inner);
            if (rhsAddrFallback.kind != TYPE_INVALID) {
                parsedTypeAddPointerDepth(&rhsAddrFallback, 1);
                rhsParsed = &rhsAddrFallback;
            }
        }
    }
    if (getenv("DEBUG_PTR_ARITH") && strcmp(op, "-") == 0) {
        const char* lhsOp = (node->expr.left && node->expr.left->type == AST_UNARY_EXPRESSION)
            ? node->expr.left->expr.op
            : NULL;
        const char* rhsOp = (node->expr.right && node->expr.right->type == AST_UNARY_EXPRESSION)
            ? node->expr.right->expr.op
            : NULL;
        fprintf(stderr,
                "[ptr-arith] bin '-' lhsNode=%d lhsOp=%s rhsNode=%d rhsOp=%s lhsParsed=%p rhsParsed=%p\n",
                node->expr.left ? node->expr.left->type : -1,
                lhsOp ? lhsOp : "<none>",
                node->expr.right ? node->expr.right->type : -1,
                rhsOp ? rhsOp : "<none>",
                (void*)lhsParsed,
                (void*)rhsParsed);
    }
    LLVMTypeRef lhsType = LLVMTypeOf(L);
    LLVMTypeRef rhsType = LLVMTypeOf(R);
    bool lhsIsPointer = lhsType && LLVMGetTypeKind(lhsType) == LLVMPointerTypeKind;
    bool rhsIsPointer = rhsType && LLVMGetTypeKind(rhsType) == LLVMPointerTypeKind;
    bool lhsFloat = cg_is_float_type(lhsType);
    bool rhsFloat = cg_is_float_type(rhsType);
    bool lhsUnsigned = node->expr.left ? cg_expression_is_unsigned(ctx, node->expr.left) : false;
    bool rhsUnsigned = node->expr.right ? cg_expression_is_unsigned(ctx, node->expr.right) : false;
    if (!lhsUnsigned) {
        lhsUnsigned = cg_is_unsigned_parsed(lhsParsed, lhsType);
    }
    if (!rhsUnsigned) {
        rhsUnsigned = cg_is_unsigned_parsed(rhsParsed, rhsType);
    }
    bool preferUnsigned = lhsUnsigned || rhsUnsigned;
    bool lhsComplex = cg_parsed_type_is_complex_value(lhsParsed);
    bool rhsComplex = cg_parsed_type_is_complex_value(rhsParsed);

    if (lhsComplex || rhsComplex) {
        const ParsedType* complexParsed = lhsComplex ? lhsParsed : rhsParsed;
        LLVMTypeRef complexType = cg_type_from_parsed(ctx, complexParsed);
        LLVMTypeRef elemType = cg_complex_element_type(ctx, complexParsed);
        LLVMValueRef lhsC = cg_promote_to_complex(ctx, L, lhsParsed, complexType, elemType);
        LLVMValueRef rhsC = cg_promote_to_complex(ctx, R, rhsParsed, complexType, elemType);

        if (strcmp(op, "+") == 0) {
            return cg_build_complex_addsub(ctx, lhsC, rhsC, complexType, false);
        }
        if (strcmp(op, "-") == 0) {
            return cg_build_complex_addsub(ctx, lhsC, rhsC, complexType, true);
        }
        if (strcmp(op, "*") == 0) {
            return cg_build_complex_muldiv(ctx, lhsC, rhsC, complexType, false);
        }
        if (strcmp(op, "/") == 0) {
            return cg_build_complex_muldiv(ctx, lhsC, rhsC, complexType, true);
        }
        if (strcmp(op, "==") == 0) {
            return cg_build_complex_eq(ctx, lhsC, rhsC, false);
        }
        if (strcmp(op, "!=") == 0) {
            return cg_build_complex_eq(ctx, lhsC, rhsC, true);
        }
        fprintf(stderr, "Error: Unsupported complex operator %s\n", op);
        return NULL;
    }

    bool useFloatOps = (lhsFloat || rhsFloat) && !lhsIsPointer && !rhsIsPointer;
    if (useFloatOps) {
        LLVMTypeRef floatType = cg_select_float_type(lhsType, rhsType, ctx->llvmContext);
        if (!floatType || LLVMGetTypeKind(floatType) == LLVMVoidTypeKind) {
            floatType = LLVMDoubleTypeInContext(ctx->llvmContext);
        }
        if (LLVMTypeOf(L) != floatType) {
            L = cg_cast_value(ctx, L, floatType, lhsParsed, NULL, "float.cast.l");
        }
        if (LLVMTypeOf(R) != floatType) {
            R = cg_cast_value(ctx, R, floatType, rhsParsed, NULL, "float.cast.r");
        }
        lhsType = floatType;
        rhsType = floatType;
    } else if (!lhsIsPointer && !rhsIsPointer) {
        cg_promote_integer_operands(ctx,
                                    &L,
                                    &R,
                                    &lhsType,
                                    &rhsType,
                                    lhsUnsigned,
                                    rhsUnsigned);
    }

    if (strcmp(op, "+") == 0) {
        if (lhsIsPointer && rhsIsPointer) {
            fprintf(stderr, "Error: cannot add two pointer values\n");
            return NULL;
        }
        if (lhsIsPointer) {
            LLVMValueRef res = cg_build_pointer_offset(ctx, L, R, lhsParsed, rhsParsed, false);
            if (!res) return NULL;
            if (lhsType && LLVMGetTypeKind(lhsType) == LLVMPointerTypeKind) {
                res = LLVMBuildPointerCast(ctx->builder, res, lhsType, "ptr.arith.cast");
            }
            return res;
        }
        if (rhsIsPointer) {
            LLVMValueRef res = cg_build_pointer_offset(ctx, R, L, rhsParsed, lhsParsed, false);
            if (!res) return NULL;
            if (rhsType && LLVMGetTypeKind(rhsType) == LLVMPointerTypeKind) {
                res = LLVMBuildPointerCast(ctx->builder, res, rhsType, "ptr.arith.cast");
            }
            return res;
        }
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "add.cast");
        }
        return useFloatOps
            ? LLVMBuildFAdd(ctx->builder, L, R, "faddtmp")
            : LLVMBuildAdd(ctx->builder, L, R, "addtmp");
    } else if (strcmp(op, "-") == 0) {
        if (lhsIsPointer && rhsIsPointer) {
            return cg_build_pointer_difference(ctx, L, R, lhsParsed, rhsParsed);
        }
        if (lhsIsPointer) {
            return cg_build_pointer_offset(ctx, L, R, lhsParsed, rhsParsed, true);
        }
        if (rhsIsPointer) {
            fprintf(stderr, "Error: cannot subtract pointer from integer\n");
            return NULL;
        }
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "sub.cast");
        }
        return useFloatOps
            ? LLVMBuildFSub(ctx->builder, L, R, "fsubtmp")
            : LLVMBuildSub(ctx->builder, L, R, "subtmp");
    } else if (strcmp(op, "*") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "mul.cast");
        }
        return useFloatOps
            ? LLVMBuildFMul(ctx->builder, L, R, "fmultmp")
            : LLVMBuildMul(ctx->builder, L, R, "multmp");
    } else if (strcmp(op, "/") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "div.cast");
        }
        if (useFloatOps) {
            return LLVMBuildFDiv(ctx->builder, L, R, "fdivtmp");
        }
        if (preferUnsigned) {
            return LLVMBuildUDiv(ctx->builder, L, R, "divtmp");
        }
        return LLVMBuildSDiv(ctx->builder, L, R, "divtmp");
    } else if (strcmp(op, "%") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "rem.cast");
        }
        if (useFloatOps) {
            return LLVMBuildFRem(ctx->builder, L, R, "fmodtmp");
        }
        if (preferUnsigned) {
            return LLVMBuildURem(ctx->builder, L, R, "modtmp");
        }
        return LLVMBuildSRem(ctx->builder, L, R, "modtmp");
    } else if (strcmp(op, "==") == 0 ||
               strcmp(op, "!=") == 0 ||
               strcmp(op, "<") == 0 ||
               strcmp(op, "<=") == 0 ||
               strcmp(op, ">") == 0 ||
               strcmp(op, ">=") == 0) {
        if (lhsIsPointer || rhsIsPointer) {
            if (lhsIsPointer && !rhsIsPointer) {
                LLVMTypeRef ptrType = LLVMTypeOf(L);
                LLVMValueRef casted = cg_cast_value(ctx, R, cg_get_intptr_type(ctx), rhsParsed, NULL, "ptr.cmp.int");
                R = LLVMBuildIntToPtr(ctx->builder, casted, ptrType, "ptr.cmp.inttoptr");
                rhsIsPointer = true;
            } else if (!lhsIsPointer && rhsIsPointer) {
                LLVMTypeRef ptrType = LLVMTypeOf(R);
                LLVMValueRef casted = cg_cast_value(ctx, L, cg_get_intptr_type(ctx), lhsParsed, NULL, "ptr.cmp.int");
                L = LLVMBuildIntToPtr(ctx->builder, casted, ptrType, "ptr.cmp.inttoptr");
                lhsIsPointer = true;
            }
            if (!lhsIsPointer || !rhsIsPointer) {
                fprintf(stderr, "Error: pointer comparison requires pointer-compatible operands\n");
                return NULL;
            }
            LLVMTypeRef ptrTy = LLVMTypeOf(L);
            if (ptrTy != LLVMTypeOf(R)) {
                R = LLVMBuildBitCast(ctx->builder, R, ptrTy, "ptr.cmp.cast");
            }
            LLVMValueRef lhsInt = LLVMBuildPtrToInt(ctx->builder, L, cg_get_intptr_type(ctx), "ptr.cmp.lhs");
            LLVMValueRef rhsInt = LLVMBuildPtrToInt(ctx->builder, R, cg_get_intptr_type(ctx), "ptr.cmp.rhs");
            LLVMIntPredicate pred = cg_select_int_predicate(op, true);
            LLVMValueRef cmp = LLVMBuildICmp(ctx->builder, pred, lhsInt, rhsInt, "ptrcmp");
            return cg_widen_bool_to_int(ctx, cmp, "ptrcmp.int");
        }
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "cmp.cast");
        }
        if (useFloatOps) {
            LLVMRealPredicate pred = LLVMRealOEQ;
            if (strcmp(op, "==") == 0) pred = LLVMRealOEQ;
            else if (strcmp(op, "!=") == 0) pred = LLVMRealUNE;
            else if (strcmp(op, "<") == 0) pred = LLVMRealOLT;
            else if (strcmp(op, "<=") == 0) pred = LLVMRealOLE;
            else if (strcmp(op, ">") == 0) pred = LLVMRealOGT;
            else if (strcmp(op, ">=") == 0) pred = LLVMRealOGE;
            LLVMValueRef cmp = LLVMBuildFCmp(ctx->builder, pred, L, R, "fcmp");
            return cg_widen_bool_to_int(ctx, cmp, "cmp.int");
        }
        LLVMIntPredicate pred = cg_select_int_predicate(op, preferUnsigned);
        LLVMValueRef cmp = LLVMBuildICmp(ctx->builder, pred, L, R, "cmptmp");
        return cg_widen_bool_to_int(ctx, cmp, "cmp.int");
    } else if (strcmp(op, "&") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "and.cast");
        }
        return LLVMBuildAnd(ctx->builder, L, R, "andtmp");
    } else if (strcmp(op, "|") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "or.cast");
        }
        return LLVMBuildOr(ctx->builder, L, R, "ortmp");
    } else if (strcmp(op, "^") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "xor.cast");
        }
        return LLVMBuildXor(ctx->builder, L, R, "xortmp");
    } else if (strcmp(op, "<<") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "shl.cast");
        }
        return LLVMBuildShl(ctx->builder, L, R, "shltmp");
    } else if (strcmp(op, ">>") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "shr.cast");
        }
        if (preferUnsigned) {
            return LLVMBuildLShr(ctx->builder, L, R, "shrtmp");
        }
        return LLVMBuildAShr(ctx->builder, L, R, "shrtmp");
    }

    fprintf(stderr, "Error: Unsupported binary operator %s\n", op);
    return NULL;
}


LLVMValueRef codegenUnaryExpression(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_UNARY_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenUnaryExpression\n");
        return NULL;
    }

    if (node->expr.op && (strcmp(node->expr.op, "++") == 0 || strcmp(node->expr.op, "--") == 0)) {
        bool isIncrement = (node->expr.op[0] == '+');
        bool isPostfix = node->expr.isPostfix;

        LLVMValueRef targetPtr = NULL;
        LLVMTypeRef targetType = NULL;
        const ParsedType* targetParsed = NULL;
        CGLValueInfo lvalInfo;
        if (!codegenLValue(ctx, node->expr.left, &targetPtr, &targetType, &targetParsed, &lvalInfo)) {
            fprintf(stderr, "Error: ++/-- operand must be assignable\n");
            return NULL;
        }

        LLVMValueRef current = NULL;
        if (lvalInfo.isBitfield) {
            unsigned width = lvalInfo.layout.widthBits ? (unsigned)lvalInfo.layout.widthBits : LLVMGetIntTypeWidth(lvalInfo.storageType);
            if (width == 0) width = LLVMGetIntTypeWidth(lvalInfo.storageType);
            LLVMTypeRef fieldType = LLVMIntTypeInContext(ctx->llvmContext, width);
            current = cg_load_bitfield(ctx, &lvalInfo, fieldType);
        } else {
            current = cg_build_load(ctx, targetType, targetPtr, "unary.load", targetParsed);
        }
        if (!current) {
            fprintf(stderr, "Error: Failed to load ++/-- operand\n");
            return NULL;
        }
        if (LLVMGetTypeKind(targetType) == LLVMPointerTypeKind) {
            LLVMValueRef one = LLVMConstInt(cg_get_intptr_type(ctx), 1, 0);
            LLVMValueRef updated = cg_build_pointer_offset(ctx, current, one, targetParsed, NULL, !isIncrement);
            if (!updated) {
                return NULL;
            }
            cg_build_store(ctx, updated, targetPtr, targetParsed);
            return isPostfix ? current : updated;
        }

        if (!targetType || LLVMGetTypeKind(targetType) != LLVMIntegerTypeKind) {
            targetType = LLVMInt32TypeInContext(ctx->llvmContext);
            current = cg_cast_value(ctx, current, targetType, targetParsed, NULL, "unary.cast");
        }
        LLVMValueRef one = LLVMConstInt(targetType, 1, 0);
        LLVMValueRef updated = isIncrement
            ? LLVMBuildAdd(ctx->builder, current, one, "unary.inc")
            : LLVMBuildSub(ctx->builder, current, one, "unary.dec");

        if (lvalInfo.isBitfield) {
            if (!cg_store_bitfield(ctx, &lvalInfo, updated)) return NULL;
        } else {
            cg_build_store(ctx, updated, targetPtr, targetParsed);
        }
        return isPostfix ? current : updated;
    }

    if (node->expr.op && strcmp(node->expr.op, "&") == 0) {
        LLVMValueRef addrPtr = NULL;
        LLVMTypeRef addrType = NULL;
        CGLValueInfo tmp;
        if (!codegenLValue(ctx, node->expr.left, &addrPtr, &addrType, NULL, &tmp)) {
            if (node->expr.left && node->expr.left->type == AST_IDENTIFIER) {
                const SemanticModel* model = cg_context_get_semantic_model(ctx);
                if (model) {
                    const Symbol* sym = semanticModelLookupGlobal(model, node->expr.left->valueNode.value);
                    if (sym && sym->kind == SYMBOL_FUNCTION) {
                        LLVMValueRef fn = LLVMGetNamedFunction(ctx->module, node->expr.left->valueNode.value);
                        if (fn) {
                            return fn;
                        }
                    }
                }
            }
            fprintf(stderr, "Error: Address-of requires an lvalue\n");
            return NULL;
        }
        if (tmp.isBitfield) {
            fprintf(stderr, "Error: Address-of bitfield is invalid\n");
            return NULL;
        }
        return addrPtr;
    }

    const ParsedType* operandParsed = cg_resolve_expression_type(ctx, node->expr.left);
    ParsedType operandParsedCopy = parsedTypeClone(operandParsed);
    ParsedType derivedPointerParsed = parsedTypeClone(NULL);
    bool hasDerivedPointerParsed = false;
    const ParsedType* stableParsed = operandParsedCopy.kind != TYPE_INVALID ? &operandParsedCopy : operandParsed;
    LLVMValueRef operand = codegenNode(ctx, node->expr.left);
    if (!operand) {
        parsedTypeFree(&operandParsedCopy);
        fprintf(stderr, "Error: Failed to generate operand for unary expression\n");
        return NULL;
    }

    if (strcmp(node->expr.op, "-") == 0) {
        if (cg_parsed_type_is_complex_value(stableParsed) &&
            LLVMGetTypeKind(LLVMTypeOf(operand)) == LLVMStructTypeKind) {
            LLVMTypeRef complexType = LLVMTypeOf(operand);
            LLVMValueRef realPart = LLVMBuildExtractValue(ctx->builder, operand, 0, "complex.neg.r");
            LLVMValueRef imagPart = LLVMBuildExtractValue(ctx->builder, operand, 1, "complex.neg.i");
            LLVMValueRef realNeg = LLVMBuildFNeg(ctx->builder, realPart, "complex.neg.rn");
            LLVMValueRef imagNeg = LLVMBuildFNeg(ctx->builder, imagPart, "complex.neg.in");
            parsedTypeFree(&operandParsedCopy);
            if (hasDerivedPointerParsed) parsedTypeFree(&derivedPointerParsed);
            return cg_build_complex_value(ctx, realNeg, imagNeg, complexType, "complex.neg");
        }
        LLVMTypeRef operandType = LLVMTypeOf(operand);
        parsedTypeFree(&operandParsedCopy);
        if (hasDerivedPointerParsed) parsedTypeFree(&derivedPointerParsed);
        if (operandType && cg_is_float_type(operandType)) {
            return LLVMBuildFNeg(ctx->builder, operand, "fnegtmp");
        }
        return LLVMBuildNeg(ctx->builder, operand, "negtmp");
    } else if (strcmp(node->expr.op, "~") == 0) {
        LLVMTypeRef operandType = LLVMTypeOf(operand);
        if (!operandType || LLVMGetTypeKind(operandType) != LLVMIntegerTypeKind) {
            LLVMTypeRef intType = LLVMInt32TypeInContext(ctx->llvmContext);
            operand = cg_cast_value(ctx, operand, intType, stableParsed, NULL, "bnot.cast");
            if (!operand) {
                parsedTypeFree(&operandParsedCopy);
                fprintf(stderr, "Error: Failed to cast operand for bitwise not\n");
                return NULL;
            }
        }
        parsedTypeFree(&operandParsedCopy);
        return LLVMBuildNot(ctx->builder, operand, "nottmp");
    } else if (strcmp(node->expr.op, "!") == 0) {
        LLVMValueRef boolVal = cg_build_truthy(ctx, operand, stableParsed, "lnot.bool");
        parsedTypeFree(&operandParsedCopy);
        if (!boolVal) return NULL;
        LLVMValueRef inverted = LLVMBuildNot(ctx->builder, boolVal, "lnot.tmp");
        return cg_widen_bool_to_int(ctx, inverted, "lnot.int");
    } else if (strcmp(node->expr.op, "*") == 0) {
        if (!cg_parsed_type_is_pointerish(stableParsed) &&
            node->expr.left &&
            node->expr.left->type == AST_UNARY_EXPRESSION &&
            node->expr.left->expr.op &&
            strcmp(node->expr.left->expr.op, "*") == 0) {
            const ParsedType* innerOperand = cg_resolve_expression_type(ctx, node->expr.left->expr.left);
            ParsedType innerOperandCopy = parsedTypeClone(innerOperand);
            derivedPointerParsed = parsedTypePointerTargetType(&innerOperandCopy);
            parsedTypeFree(&innerOperandCopy);
            if (derivedPointerParsed.kind != TYPE_INVALID) {
                stableParsed = &derivedPointerParsed;
                hasDerivedPointerParsed = true;
            }
        }
        size_t arrayDerivCount = stableParsed
            ? parsedTypeCountDerivationsOfKind(stableParsed, TYPE_DERIVATION_ARRAY)
            : 0;
        if (stableParsed &&
            !cg_parsed_type_is_pointerish(stableParsed) &&
            arrayDerivCount > 1) {
            parsedTypeFree(&operandParsedCopy);
            if (hasDerivedPointerParsed) parsedTypeFree(&derivedPointerParsed);
            return operand;
        }
        LLVMTypeRef ptrType = LLVMTypeOf(operand);
        if (!ptrType || LLVMGetTypeKind(ptrType) != LLVMPointerTypeKind) {
            parsedTypeFree(&operandParsedCopy);
            if (hasDerivedPointerParsed) parsedTypeFree(&derivedPointerParsed);
            fprintf(stderr, "Error: Cannot dereference non-pointer value\n");
            return NULL;
        }
        LLVMTypeRef elemType = NULL;
        ASTNode* baseExpr = node->expr.left;
        size_t derefCount = 1;
        while (baseExpr &&
               baseExpr->type == AST_UNARY_EXPRESSION &&
               baseExpr->expr.op &&
               strcmp(baseExpr->expr.op, "*") == 0) {
            derefCount++;
            baseExpr = baseExpr->expr.left;
        }
        if (baseExpr) {
            const ParsedType* baseType = NULL;
            if (baseExpr->type == AST_IDENTIFIER && ctx && ctx->currentScope) {
                NamedValue* entry = cg_scope_lookup(ctx->currentScope, baseExpr->valueNode.value);
                if (entry && entry->parsedType) {
                    baseType = entry->parsedType;
                }
            }
            if (!baseType) {
                baseType = cg_resolve_expression_type(ctx, baseExpr);
            }
            ParsedType chain = parsedTypeClone(baseType);
            bool ok = chain.kind != TYPE_INVALID;
            for (size_t i = 0; i < derefCount && ok; ++i) {
                ParsedType next = parsedTypePointerTargetType(&chain);
                parsedTypeFree(&chain);
                chain = next;
                ok = chain.kind != TYPE_INVALID;
            }
            if (ok) {
                elemType = cg_type_from_parsed(ctx, &chain);
            }
            parsedTypeFree(&chain);
        }
        if (!elemType) {
            elemType = cg_element_type_from_pointer(ctx, stableParsed, ptrType);
        }
        if (!elemType || LLVMGetTypeKind(elemType) == LLVMVoidTypeKind) {
            elemType = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        parsedTypeFree(&operandParsedCopy);
        if (hasDerivedPointerParsed) parsedTypeFree(&derivedPointerParsed);
        return LLVMBuildLoad2(ctx->builder, elemType, operand, "loadtmp");
    }

    parsedTypeFree(&operandParsedCopy);
    if (hasDerivedPointerParsed) parsedTypeFree(&derivedPointerParsed);
    fprintf(stderr, "Error: Unsupported unary operator %s\n", node->expr.op);
    return NULL;
}


LLVMValueRef codegenTernaryExpression(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_TERNARY_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenTernaryExpression\n");
        return NULL;
    }

    LLVMValueRef condition = codegenNode(ctx, node->ternaryExpr.condition);
    if (!condition) {
        fprintf(stderr, "Error: Failed to generate condition for ternary expression\n");
        return NULL;
    }

    condition = cg_build_truthy(ctx, condition, NULL, "ternaryCond");
    if (!condition) {
        fprintf(stderr, "Error: Failed to convert ternary condition to boolean\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));

    LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(func, "ternaryTrue");
    LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(func, "ternaryFalse");
    LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(func, "ternaryMerge");

    LLVMBuildCondBr(ctx->builder, condition, trueBB, falseBB);

    LLVMPositionBuilderAtEnd(ctx->builder, trueBB);
    LLVMValueRef trueValue = codegenNode(ctx, node->ternaryExpr.trueExpr);
    const ParsedType* trueParsed = cg_resolve_expression_type(ctx, node->ternaryExpr.trueExpr);
    LLVMBasicBlockRef trueEndBB = LLVMGetInsertBlock(ctx->builder);
    if (!LLVMGetBasicBlockTerminator(trueEndBB)) {
        LLVMBuildBr(ctx->builder, mergeBB);
    }

    LLVMPositionBuilderAtEnd(ctx->builder, falseBB);
    LLVMValueRef falseValue = codegenNode(ctx, node->ternaryExpr.falseExpr);
    const ParsedType* falseParsed = cg_resolve_expression_type(ctx, node->ternaryExpr.falseExpr);
    LLVMBasicBlockRef falseEndBB = LLVMGetInsertBlock(ctx->builder);
    if (!LLVMGetBasicBlockTerminator(falseEndBB)) {
        LLVMBuildBr(ctx->builder, mergeBB);
    }

    LLVMPositionBuilderAtEnd(ctx->builder, mergeBB);
    // If both arms are void, just merge control flow.
    if (!trueValue && !falseValue) {
        return NULL;
    }

    LLVMTypeRef mergedType = NULL;
    if (trueValue && falseValue) {
        LLVMTypeRef trueTy = LLVMTypeOf(trueValue);
        LLVMTypeRef falseTy = LLVMTypeOf(falseValue);
        LLVMTypeKind trueKind = trueTy ? LLVMGetTypeKind(trueTy) : LLVMVoidTypeKind;
        LLVMTypeKind falseKind = falseTy ? LLVMGetTypeKind(falseTy) : LLVMVoidTypeKind;
        if (trueKind == LLVMPointerTypeKind && falseKind == LLVMPointerTypeKind) {
            mergedType = trueTy;
        } else if (trueKind == LLVMPointerTypeKind &&
                   falseKind == LLVMIntegerTypeKind &&
                   LLVMIsAConstantInt(falseValue) &&
                   LLVMConstIntGetSExtValue(falseValue) == 0) {
            mergedType = trueTy;
        } else if (falseKind == LLVMPointerTypeKind &&
                   trueKind == LLVMIntegerTypeKind &&
                   LLVMIsAConstantInt(trueValue) &&
                   LLVMConstIntGetSExtValue(trueValue) == 0) {
            mergedType = falseTy;
        }
        if (!mergedType) {
            mergedType = cg_merge_types_for_phi(ctx, trueParsed, falseParsed, trueValue, falseValue);
        }
    } else if (trueValue) {
        mergedType = LLVMTypeOf(trueValue);
    } else {
        mergedType = LLVMTypeOf(falseValue);
    }
    if (!mergedType) {
        mergedType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    if (trueValue && LLVMTypeOf(trueValue) != mergedType) {
        trueValue = cg_cast_value(ctx, trueValue, mergedType, trueParsed, falseParsed, "ternary.true.cast");
    }
    if (falseValue && LLVMTypeOf(falseValue) != mergedType) {
        falseValue = cg_cast_value(ctx, falseValue, mergedType, falseParsed, trueParsed, "ternary.false.cast");
    }

    LLVMValueRef phi = LLVMBuildPhi(ctx->builder, mergedType, "ternaryResult");
    LLVMValueRef incomingVals[2];
    LLVMBasicBlockRef incomingBlocks[2];
    int count = 0;
    if (trueValue) {
        incomingVals[count] = trueValue;
        incomingBlocks[count] = trueEndBB;
        count++;
    }
    if (falseValue) {
        incomingVals[count] = falseValue;
        incomingBlocks[count] = falseEndBB;
        count++;
    }
    LLVMAddIncoming(phi, incomingVals, incomingBlocks, count);

    return phi;
}

LLVMValueRef codegenCommaExpression(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_COMMA_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenCommaExpression\n");
        return NULL;
    }

    LLVMValueRef lastValue = NULL;
    for (size_t i = 0; i < node->commaExpr.exprCount; ++i) {
        ASTNode* expr = node->commaExpr.expressions ? node->commaExpr.expressions[i] : NULL;
        if (!expr) {
            continue;
        }
        lastValue = codegenNode(ctx, expr);
    }
    return lastValue;
}


LLVMValueRef codegenAssignment(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_ASSIGNMENT) {
        fprintf(stderr, "Error: Invalid node type for codegenAssignment\n");
        return NULL;
    }

    CG_DEBUG("[CG] Assignment node\n");
    ASTNode* target = node->assignment.target;
    LLVMValueRef targetPtr = NULL;
    LLVMTypeRef targetType = NULL;
    const ParsedType* targetParsed = NULL;
    CGLValueInfo lvalInfo;
    LLVMValueRef srcPtr = NULL;
    LLVMTypeRef srcType = NULL;
    const ParsedType* srcParsed = NULL;
    if (!codegenLValue(ctx, target, &targetPtr, &targetType, &targetParsed, &lvalInfo)) {
        fprintf(stderr,
                "Error: Unsupported assignment target type %d at line %d\n",
                target ? target->type : -1,
                target ? target->line : -1);
        return NULL;
    }
    if (getenv("DEBUG_FLEX_ASSIGN")) {
        fprintf(stderr,
                "[DBG] assign flex flags: isFlexElement=%d size=%llu isBitfield=%d\n",
                lvalInfo.isFlexElement,
                (unsigned long long)lvalInfo.flexElemSize,
                lvalInfo.isBitfield);
    }

    LLVMValueRef value = codegenNode(ctx, node->assignment.value);
    if (!value) {
        fprintf(stderr, "Error: Assignment value failed to generate\n");
        return NULL;
    }
    const ParsedType* valueParsed = cg_resolve_expression_type(ctx, node->assignment.value);
    const char* op = node->assignment.op ? node->assignment.op : "=";

    bool destIsAggregate = targetType && (LLVMGetTypeKind(targetType) == LLVMStructTypeKind ||
                                          LLVMGetTypeKind(targetType) == LLVMArrayTypeKind);
    if (destIsAggregate) {
        bool isComplexTarget =
            cg_parsed_type_is_complex_value(targetParsed) || cg_llvm_type_is_complex_value(targetType);

        if (strcmp(op, "=") != 0) {
            if (!isComplexTarget ||
                (strcmp(op, "+=") != 0 && strcmp(op, "-=") != 0 &&
                 strcmp(op, "*=") != 0 && strcmp(op, "/=") != 0)) {
                fprintf(stderr, "Error: Unsupported compound assignment operator %s for aggregate target\n", op);
                return NULL;
            }

            LLVMValueRef current = cg_build_load(ctx, targetType, targetPtr, "complex.compound.load", targetParsed);
            if (!current) {
                fprintf(stderr, "Error: Failed to load complex target for compound assignment\n");
                return NULL;
            }

            LLVMTypeRef complexType = targetType;
            LLVMTypeRef elemType = cg_complex_element_type(ctx, targetParsed);
            LLVMTypeRef inferredElemType = cg_complex_element_type_from_llvm(complexType);
            if (inferredElemType) {
                elemType = inferredElemType;
            }
            LLVMValueRef lhsComplex = cg_promote_to_complex(ctx, current, targetParsed, complexType, elemType);
            LLVMValueRef rhsComplex = cg_promote_to_complex(ctx, value, valueParsed, complexType, elemType);
            if (!lhsComplex || !rhsComplex) {
                fprintf(stderr, "Error: Failed to promote complex operands for compound assignment\n");
                return NULL;
            }

            LLVMValueRef result = NULL;
            if (strcmp(op, "+=") == 0 || strcmp(op, "-=") == 0) {
                bool isSub = (strcmp(op, "-=") == 0);
                result = cg_build_complex_addsub(ctx, lhsComplex, rhsComplex, complexType, isSub);
            } else if (strcmp(op, "*=") == 0) {
                result = cg_build_complex_muldiv(ctx, lhsComplex, rhsComplex, complexType, false);
            } else if (strcmp(op, "/=") == 0) {
                result = cg_build_complex_muldiv(ctx, lhsComplex, rhsComplex, complexType, true);
            }
            if (!result) {
                fprintf(stderr, "Error: Failed to build complex compound assignment result\n");
                return NULL;
            }
            cg_build_store(ctx, result, targetPtr, targetParsed);
            return result;
        }

        bool rhsIsLValue = node->assignment.value &&
                           (node->assignment.value->type == AST_IDENTIFIER ||
                            node->assignment.value->type == AST_ARRAY_ACCESS ||
                            node->assignment.value->type == AST_POINTER_ACCESS ||
                            node->assignment.value->type == AST_DOT_ACCESS ||
                            node->assignment.value->type == AST_COMPOUND_LITERAL);
        if (!rhsIsLValue || !codegenLValue(ctx, node->assignment.value, &srcPtr, &srcType, &srcParsed, NULL)) {
            LLVMValueRef tmp = cg_build_entry_alloca(ctx, targetType, "agg.tmp");
            LLVMBuildStore(ctx->builder, value, tmp);
            srcPtr = tmp;
            srcType = targetType;
            srcParsed = targetParsed;
        }
        uint64_t bytes = 0;
        uint32_t align = 0;
        cg_size_align_for_type(ctx, targetParsed, targetType, &bytes, &align);
        LLVMTypeRef i8Ptr = LLVMPointerType(LLVMInt8TypeInContext(ctx->llvmContext), 0);
        LLVMValueRef dstCast = LLVMBuildBitCast(ctx->builder, targetPtr, i8Ptr, "agg.dst");
        LLVMValueRef srcCast = LLVMBuildBitCast(ctx->builder, srcPtr, i8Ptr, "agg.src");
        LLVMValueRef sizeVal = LLVMConstInt(LLVMInt64TypeInContext(ctx->llvmContext), bytes, 0);
        unsigned alignVal = align ? align : 1;
        LLVMBuildMemCpy(ctx->builder,
                        dstCast,
                        alignVal,
                        srcCast,
                        alignVal,
                        sizeVal);
        return value ? value : targetPtr;
    }

    LLVMTypeRef storeType = targetType;
    if (lvalInfo.isBitfield) {
        unsigned width = lvalInfo.layout.widthBits ? (unsigned)lvalInfo.layout.widthBits : LLVMGetIntTypeWidth(lvalInfo.storageType);
        if (width == 0) width = LLVMGetIntTypeWidth(lvalInfo.storageType);
        storeType = LLVMIntTypeInContext(ctx->llvmContext, width);
    }
    if (!storeType || LLVMGetTypeKind(storeType) == LLVMVoidTypeKind) {
        storeType = cg_dereference_ptr_type(ctx, LLVMTypeOf(targetPtr), "assignment target");
    }
    if (!storeType || LLVMGetTypeKind(storeType) == LLVMVoidTypeKind) {
        storeType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    if (lvalInfo.isFlexElement && lvalInfo.flexElemSize > 0) {
        unsigned bits = (unsigned)(lvalInfo.flexElemSize * 8);
        if (bits == 0) bits = 8;
        if (bits > 64) bits = 64;
        storeType = LLVMIntTypeInContext(ctx->llvmContext, bits);
    }
    LLVMValueRef storedValue = value;

    if (op && strcmp(op, "=") != 0) {
        LLVMValueRef current = NULL;
        if (lvalInfo.isBitfield) {
            current = cg_load_bitfield(ctx, &lvalInfo, storeType);
        } else {
            current = cg_build_load(ctx, storeType, targetPtr, "compound.load", targetParsed);
        }
        if (!current) {
            fprintf(stderr, "Error: Failed to load target for compound assignment\n");
            return NULL;
        }

        bool targetIsPointer = LLVMGetTypeKind(storeType) == LLVMPointerTypeKind;
        bool targetIsFloat = cg_is_float_type(LLVMTypeOf(current));
        if (targetIsPointer && (strcmp(op, "+=") == 0 || strcmp(op, "-=") == 0)) {
            bool isSubtract = (strcmp(op, "-=") == 0);
            storedValue = cg_build_pointer_offset(ctx, current, value, targetParsed, valueParsed, isSubtract);
            if (!storedValue) {
                return NULL;
            }
        } else if (strcmp(op, "+=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = targetIsFloat
                ? LLVMBuildFAdd(ctx->builder, current, value, "compound.fadd")
                : LLVMBuildAdd(ctx->builder, current, value, "compound.add");
        } else if (strcmp(op, "-=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = targetIsFloat
                ? LLVMBuildFSub(ctx->builder, current, value, "compound.fsub")
                : LLVMBuildSub(ctx->builder, current, value, "compound.sub");
        } else if (strcmp(op, "*=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = targetIsFloat
                ? LLVMBuildFMul(ctx->builder, current, value, "compound.fmul")
                : LLVMBuildMul(ctx->builder, current, value, "compound.mul");
        } else if (strcmp(op, "/=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            if (targetIsFloat) {
                storedValue = LLVMBuildFDiv(ctx->builder, current, value, "compound.fdiv");
            } else if (cg_expression_is_unsigned(ctx, node->assignment.target)) {
                storedValue = LLVMBuildUDiv(ctx->builder, current, value, "compound.div");
            } else {
                storedValue = LLVMBuildSDiv(ctx->builder, current, value, "compound.div");
            }
        } else if (strcmp(op, "%=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            if (targetIsFloat) {
                storedValue = LLVMBuildFRem(ctx->builder, current, value, "compound.frem");
            } else if (cg_expression_is_unsigned(ctx, node->assignment.target)) {
                storedValue = LLVMBuildURem(ctx->builder, current, value, "compound.rem");
            } else {
                storedValue = LLVMBuildSRem(ctx->builder, current, value, "compound.rem");
            }
        } else if (strcmp(op, "&=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = LLVMBuildAnd(ctx->builder, current, value, "compound.and");
        } else if (strcmp(op, "|=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = LLVMBuildOr(ctx->builder, current, value, "compound.or");
        } else if (strcmp(op, "^=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = LLVMBuildXor(ctx->builder, current, value, "compound.xor");
        } else if (strcmp(op, "<<=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = LLVMBuildShl(ctx->builder, current, value, "compound.shl");
        } else if (strcmp(op, ">>=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            if (cg_expression_is_unsigned(ctx, node->assignment.target)) {
                storedValue = LLVMBuildLShr(ctx->builder, current, value, "compound.lshr");
            } else {
                storedValue = LLVMBuildAShr(ctx->builder, current, value, "compound.ashr");
            }
        } else {
            fprintf(stderr, "Error: Unsupported compound assignment operator %s\n", op);
            return NULL;
        }
    }

    storedValue = cg_cast_value(ctx,
                                storedValue,
                                storeType,
                                NULL,
                                targetParsed,
                                "assign.cast");
    if (lvalInfo.isBitfield) {
        if (!cg_store_bitfield(ctx, &lvalInfo, storedValue)) {
            return NULL;
        }
        return storedValue;
    }
    if (lvalInfo.isFlexElement && lvalInfo.flexElemSize > 0) {
        if (getenv("DEBUG_FLEX_ASSIGN")) {
            char* tstr = storeType ? LLVMPrintTypeToString(storeType) : NULL;
            fprintf(stderr, "[DBG] flex store type=%s size=%llu\n", tstr ? tstr : "<null>",
                    (unsigned long long)lvalInfo.flexElemSize);
            if (tstr) LLVMDisposeMessage(tstr);
        }
        LLVMTypeRef i8Ptr = LLVMPointerType(LLVMInt8TypeInContext(ctx->llvmContext), 0);
        LLVMValueRef dstCast = LLVMBuildBitCast(ctx->builder, targetPtr, i8Ptr, "flex.dst");
        LLVMValueRef srcCast = NULL;
        ASTNode* rhs = node->assignment.value;
        bool rhsIsLValue = rhs &&
                           (rhs->type == AST_IDENTIFIER || rhs->type == AST_ARRAY_ACCESS ||
                            rhs->type == AST_POINTER_ACCESS || rhs->type == AST_DOT_ACCESS ||
                            rhs->type == AST_COMPOUND_LITERAL);
        if (rhsIsLValue &&
            codegenLValue(ctx, rhs, &srcPtr, &srcType, &srcParsed, NULL)) {
            srcCast = LLVMBuildBitCast(ctx->builder, srcPtr, i8Ptr, "flex.src");
        } else {
            LLVMValueRef tmp = cg_build_entry_alloca(ctx, storeType, "flex.tmp");
            LLVMBuildStore(ctx->builder, storedValue, tmp);
            srcCast = LLVMBuildBitCast(ctx->builder, tmp, i8Ptr, "flex.src.tmp");
        }
        LLVMValueRef sizeVal =
            LLVMConstInt(LLVMInt64TypeInContext(ctx->llvmContext), lvalInfo.flexElemSize, 0);
        LLVMBuildMemCpy(ctx->builder, dstCast, 1, srcCast, 1, sizeVal);
        return storedValue;
    }
    cg_build_store(ctx, storedValue, targetPtr, targetParsed);
    return storedValue;
}


LLVMValueRef codegenArrayAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_ARRAY_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenArrayAccess\n");
        return NULL;
    }
    const char* dbgRun = getenv("DEBUG_RUN");
    if (dbgRun) fprintf(stderr, "[CG] array access begin\n");

    LLVMValueRef arrayPtr = NULL;
    LLVMTypeRef arrayType = NULL;
    const ParsedType* arrayParsed = NULL;
    CGLValueInfo arrLValInfo;
    bool haveLVal =
        codegenLValue(ctx, node->arrayAccess.array, &arrayPtr, &arrayType, &arrayParsed, &arrLValInfo);
    if (!haveLVal) {
        arrayPtr = codegenNode(ctx, node->arrayAccess.array);
    }
    /* For pointer variables (not true arrays), arrayPtr currently holds the address of the
       pointer object (e.g., an alloca of `int*`). Load the pointer value so we index the
       pointee rather than the stack slot. */
    bool baseIsDirectArray = (arrayParsed && parsedTypeIsDirectArray(arrayParsed)) ||
                             (arrayType && LLVMGetTypeKind(arrayType) == LLVMArrayTypeKind);
    bool baseNeedsLoad = haveLVal && arrayPtr && !baseIsDirectArray;
    if (baseNeedsLoad) {
        LLVMTypeRef loadTy = arrayType;
        if (!loadTy) {
            LLVMTypeRef elem = NULL;
            if (arrayPtr && LLVMGetTypeKind(LLVMTypeOf(arrayPtr)) == LLVMPointerTypeKind) {
                LLVMTypeRef ptrTy = LLVMTypeOf(arrayPtr);
                if (!LLVMPointerTypeIsOpaque(ptrTy)) {
                    elem = LLVMGetElementType(ptrTy);
                }
            }
            loadTy = elem ? elem : LLVMPointerType(LLVMInt8TypeInContext(ctx->llvmContext), 0);
        }
        arrayPtr = LLVMBuildLoad2(ctx->builder, loadTy, arrayPtr, "array.ptr.load");
        arrayType = loadTy;
    }
    LLVMValueRef index = codegenNode(ctx, node->arrayAccess.index);
    if (!arrayPtr || !index) {
        fprintf(stderr, "Error: Array access failed\n");
        return NULL;
    }
    if (dbgRun) {
        char* atStr = LLVMPrintTypeToString(LLVMTypeOf(arrayPtr));
        fprintf(stderr, "[CG] arrayPtr LLVM type=%s\n", atStr ? atStr : "<null>");
        if (atStr) LLVMDisposeMessage(atStr);
    }
    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
    if (!arrayParsed) {
        arrayParsed = cg_resolve_expression_type(ctx, node->arrayAccess.array);
    }
    const ParsedType* refinedCallParsed =
        cg_refine_function_call_result_type(ctx, node->arrayAccess.array);
    if (refinedCallParsed) {
        arrayParsed = refinedCallParsed;
    }
    if (dbgRun && arrayParsed) {
        fprintf(stderr, "[CG] array parsed kind=%d prim=%d name=%s ptrDepth=%d derivs=%zu\n",
                arrayParsed->kind,
                arrayParsed->primitiveType,
                arrayParsed->userTypeName ? arrayParsed->userTypeName : "<none>",
                arrayParsed->pointerDepth,
                arrayParsed->derivationCount);
    }

    if (arrayParsed && parsedTypeIsDirectArray(arrayParsed) && parsedTypeHasVLA(arrayParsed)) {
        size_t arrayDims = 0;
        for (size_t i = 0; i < arrayParsed->derivationCount; ++i) {
            const TypeDerivation* deriv = parsedTypeGetDerivation(arrayParsed, i);
            if (deriv && deriv->kind == TYPE_DERIVATION_ARRAY) {
                arrayDims++;
            }
        }
        LLVMValueRef stride = LLVMConstInt(intptrTy, 1, 0);
        for (size_t i = 1; i < arrayParsed->derivationCount; ++i) {
            const TypeDerivation* deriv = parsedTypeGetDerivation(arrayParsed, i);
            if (!deriv || deriv->kind != TYPE_DERIVATION_ARRAY) continue;
            LLVMValueRef dim = NULL;
            if (!deriv->as.array.isVLA && deriv->as.array.hasConstantSize && deriv->as.array.constantSize > 0) {
                dim = LLVMConstInt(intptrTy, (unsigned long long)deriv->as.array.constantSize, 0);
            } else if (deriv->as.array.sizeExpr) {
                LLVMValueRef evaluated = codegenNode(ctx, deriv->as.array.sizeExpr);
                dim = ensureIntegerLike(ctx, evaluated);
            }
            if (!dim) continue;
            stride = LLVMBuildMul(ctx->builder, stride, dim, "vla.stride");
        }

        LLVMValueRef offset = ensureIntegerLike(ctx, index);
        if (!offset) return NULL;
        if (arrayDims > 1) {
            offset = LLVMBuildMul(ctx->builder, offset, stride, "vla.offset");
        }

        LLVMTypeRef elemType = vlaInnermostElementLLVM(ctx, arrayParsed);
        LLVMValueRef elementPtr = LLVMBuildGEP2(ctx->builder, elemType, arrayPtr, &offset, 1, "vla.elem.ptr");

        bool hasMoreDims = arrayDims > 1;
        if (hasMoreDims) {
            return elementPtr;
        }
        LLVMValueRef loadVal = LLVMBuildLoad2(ctx->builder, elemType, elementPtr, "arrayLoad");
        return loadVal;
    }
    char* arrTyStr = LLVMPrintTypeToString(LLVMTypeOf(arrayPtr));
    CG_DEBUG("[CG] Array access base type: %s\n", arrTyStr ? arrTyStr : "<null>");
    if (arrTyStr) LLVMDisposeMessage(arrTyStr);
    LLVMTypeRef aggregateHint = NULL;
    if (arrayParsed && parsedTypeIsDirectArray(arrayParsed)) {
        aggregateHint = cg_type_from_parsed(ctx, arrayParsed);
    } else if (arrayType && LLVMGetTypeKind(arrayType) == LLVMArrayTypeKind) {
        aggregateHint = arrayType;
    }
    LLVMTypeRef elementHint = cg_element_type_hint_from_parsed(ctx, arrayParsed);
    if (!elementHint && arrayType && LLVMGetTypeKind(arrayType) == LLVMPointerTypeKind) {
        LLVMTypeRef elem = LLVMGetElementType(arrayType);
        if (elem) {
            elementHint = elem;
        }
    }
    LLVMTypeRef derivedElementType = NULL;
    LLVMValueRef elementPtr = buildArrayElementPointer(ctx,
                                                       arrayPtr,
                                                       index,
                                                       arrayParsed,
                                                       aggregateHint,
                                                       elementHint,
                                                       &derivedElementType);
    if (!elementPtr) {
        fprintf(stderr, "Error: Failed to compute array element pointer\n");
        return NULL;
    }
    if (arrayParsed && cg_parsed_type_is_pointerish(arrayParsed)) {
        ParsedType pointed = parsedTypePointerTargetType(arrayParsed);
        bool pointerToVLAArray =
            pointed.kind != TYPE_INVALID &&
            parsedTypeIsDirectArray(&pointed) &&
            parsedTypeHasVLA(&pointed);
        if (getenv("DEBUG_RUN")) {
            fprintf(stderr,
                    "[CG] ptr-array access pointerToVLA=%d basePtrDepth=%d baseDerivs=%zu pointedDerivs=%zu\n",
                    pointerToVLAArray ? 1 : 0,
                    arrayParsed->pointerDepth,
                    arrayParsed->derivationCount,
                    pointed.derivationCount);
        }
        parsedTypeFree(&pointed);
        if (pointerToVLAArray) {
            return elementPtr;
        }
    }
    CG_DEBUG("[CG] Array element pointer computed\n");
    LLVMTypeRef ptrToElemType = LLVMTypeOf(elementPtr);
    char* ptrElemStr = ptrToElemType ? LLVMPrintTypeToString(ptrToElemType) : NULL;
    CG_DEBUG("[CG] Array element pointer LLVM type=%s\n", ptrElemStr ? ptrElemStr : "<null>");
    if (ptrElemStr) LLVMDisposeMessage(ptrElemStr);
    if (dbgRun) {
        char* hintStr = elementHint ? LLVMPrintTypeToString(elementHint) : NULL;
        char* derivedStr = derivedElementType ? LLVMPrintTypeToString(derivedElementType) : NULL;
        fprintf(stderr, "[CG] elementHint=%s derived=%s\n",
                hintStr ? hintStr : "<null>",
                derivedStr ? derivedStr : "<null>");
        if (hintStr) LLVMDisposeMessage(hintStr);
        if (derivedStr) LLVMDisposeMessage(derivedStr);
    }

    LLVMTypeRef elementType = derivedElementType ? derivedElementType : elementHint;
    if (!elementType) {
        elementType = cg_dereference_ptr_type(ctx, ptrToElemType, "array access");
    }
    if (!elementType || LLVMGetTypeKind(elementType) == LLVMVoidTypeKind) {
        if (LLVMGetTypeKind(ptrToElemType) == LLVMPointerTypeKind && LLVMGetElementType(ptrToElemType) == NULL) {
            fprintf(stderr, "Error: opaque pointer array access without element hint\n");
            return NULL;
        }
        elementType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    if (dbgRun) {
        char* etStr = LLVMPrintTypeToString(elementType);
        fprintf(stderr, "[CG] array elem type %s\n", etStr ? etStr : "<null>");
        if (etStr) LLVMDisposeMessage(etStr);
    }
    LLVMValueRef typedElementPtr = elementPtr;
    LLVMTypeRef loadTy = elementType ? elementType : LLVMInt8TypeInContext(ctx->llvmContext);
    if (loadTy && LLVMGetTypeKind(loadTy) == LLVMFunctionTypeKind) {
        loadTy = LLVMPointerType(loadTy, 0);
    }
    if (!typedElementPtr || !LLVMTypeOf(typedElementPtr) ||
        LLVMGetTypeKind(LLVMTypeOf(typedElementPtr)) != LLVMPointerTypeKind) {
        fprintf(stderr, "Error: invalid pointer for array load\n");
        return NULL;
    }
    if (loadTy && LLVMGetTypeKind(loadTy) == LLVMArrayTypeKind) {
        LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
        LLVMValueRef idxs[2] = { zero, zero };
        return LLVMBuildGEP2(ctx->builder, loadTy, typedElementPtr, idxs, 2, "array.elem.decay");
    }
    if (dbgRun) fprintf(stderr, "[CG] array load begin\n");
    LLVMTypeRef expectedPtrTy = LLVMPointerType(loadTy, 0);
    if (LLVMTypeOf(typedElementPtr) != expectedPtrTy) {
        typedElementPtr = LLVMBuildBitCast(ctx->builder, typedElementPtr, expectedPtrTy, "array.elem.cast");
    }
    LLVMValueRef loadVal = LLVMBuildLoad2(ctx->builder, loadTy, typedElementPtr, "arrayLoad");
    CG_DEBUG("[CG] Array element load complete\n");
    if (dbgRun) fprintf(stderr, "[CG] array access end\n");
    return loadVal;
}


LLVMValueRef codegenPointerAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_POINTER_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenPointerAccess\n");
        return NULL;
    }
    LLVMValueRef fieldPtr = NULL;
    LLVMTypeRef fieldType = NULL;
    const ParsedType* fieldParsed = NULL;
    CGLValueInfo info;
    if (!codegenLValue(ctx, node, &fieldPtr, &fieldType, &fieldParsed, &info)) {
        const char* fieldName = node->memberAccess.field ? node->memberAccess.field : "<unknown>";
        fprintf(stderr,
                "Error: Unknown field in pointer access: %s at line %d\n",
                fieldName,
                node->line);
        return NULL;
    }
    if (info.isBitfield) {
        unsigned width = info.layout.widthBits ? (unsigned)info.layout.widthBits : LLVMGetIntTypeWidth(info.storageType);
        if (width == 0) width = LLVMGetIntTypeWidth(info.storageType);
        LLVMTypeRef resultTy = LLVMIntTypeInContext(ctx->llvmContext, width);
        return cg_load_bitfield(ctx, &info, resultTy);
    }
    if (fieldType && LLVMGetTypeKind(fieldType) == LLVMArrayTypeKind) {
        LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
        LLVMValueRef idxs[2] = { zero, zero };
        return LLVMBuildGEP2(ctx->builder, fieldType, fieldPtr, idxs, 2, "ptr_field_decay");
    }
    if (!fieldType || LLVMGetTypeKind(fieldType) == LLVMVoidTypeKind) {
        fieldType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    return LLVMBuildLoad2(ctx->builder, fieldType, fieldPtr, "ptr_field");
}


LLVMValueRef codegenDotAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_DOT_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenDotAccess\n");
        return NULL;
    }
    LLVMValueRef fieldPtr = NULL;
    LLVMTypeRef fieldType = NULL;
    const ParsedType* fieldParsed = NULL;
    CGLValueInfo info;
    if (!codegenLValue(ctx, node, &fieldPtr, &fieldType, &fieldParsed, &info)) {
        const char* fieldName = node->memberAccess.field ? node->memberAccess.field : "<unknown>";
        fprintf(stderr, "Error: Unknown field in dot access: %s\n", fieldName);
        return NULL;
    }
    if (info.isBitfield) {
        unsigned width = info.layout.widthBits ? (unsigned)info.layout.widthBits : LLVMGetIntTypeWidth(info.storageType);
        if (width == 0) width = LLVMGetIntTypeWidth(info.storageType);
        LLVMTypeRef resultTy = LLVMIntTypeInContext(ctx->llvmContext, width);
        LLVMValueRef result = cg_load_bitfield(ctx, &info, resultTy);
        CG_DEBUG("[CG] Dot access bitfield load complete\n");
        return result;
    }
    if (fieldType && LLVMGetTypeKind(fieldType) == LLVMArrayTypeKind) {
        LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
        LLVMValueRef idxs[2] = { zero, zero };
        return LLVMBuildGEP2(ctx->builder, fieldType, fieldPtr, idxs, 2, "dot_field_decay");
    }
    if (!fieldType || LLVMGetTypeKind(fieldType) == LLVMVoidTypeKind) {
        fieldType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    return LLVMBuildLoad2(ctx->builder, fieldType, fieldPtr, "dot_field");
}


LLVMValueRef codegenPointerDereference(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_POINTER_DEREFERENCE) {
        fprintf(stderr, "Error: Invalid node type for codegenPointerDereference\n");
        return NULL;
    }

    LLVMValueRef pointer = codegenNode(ctx, node->pointerDeref.pointer);
    if (!pointer) {
        fprintf(stderr, "Error: Failed to generate pointer dereference\n");
        return NULL;
    }
    LLVMTypeRef elemType = NULL;
    /* Prefer semantic type information if available. */
    const ParsedType* ptrParsed = cg_resolve_expression_type(ctx, node->pointerDeref.pointer);
    if (ptrParsed && ptrParsed->pointerDepth > 0) {
        elemType = cg_element_type_from_pointer_parsed(ctx, ptrParsed);
        if (elemType && LLVMGetTypeKind(elemType) == LLVMPointerTypeKind) {
            ParsedType base = parsedTypeClone(ptrParsed);
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
            LLVMTypeRef forced = cg_type_from_parsed(ctx, &base);
            parsedTypeFree(&base);
            if (forced && LLVMGetTypeKind(forced) != LLVMPointerTypeKind && LLVMGetTypeKind(forced) != LLVMVoidTypeKind) {
                elemType = forced;
            }
        }
    }
    /* If the operand is an identifier, try to use its stored element type to avoid opaque-pointer ambiguity. */
    if (!elemType && node->pointerDeref.pointer && node->pointerDeref.pointer->type == AST_IDENTIFIER) {
        const char* name = node->pointerDeref.pointer->valueNode.value;
        NamedValue* entry = cg_scope_lookup(ctx->currentScope, name);
        if (entry && entry->elementType) {
            elemType = entry->elementType;
        } else if (entry && entry->parsedType && entry->parsedType->pointerDepth > 0) {
            elemType = cg_element_type_from_pointer_parsed(ctx, entry->parsedType);
        }
    }
    if (!elemType) {
        elemType = cg_dereference_ptr_type(ctx, LLVMTypeOf(pointer), "pointer dereference");
    }
    if (!elemType || LLVMGetTypeKind(elemType) == LLVMVoidTypeKind) {
        elemType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    return LLVMBuildLoad2(ctx->builder, elemType, pointer, "ptrLoad");
}


LLVMValueRef codegenLogicalAndOr(CodegenContext* ctx,
                                        LLVMValueRef lhsValue,
                                        ASTNode* rhsNode,
                                        bool isAnd) {
    if (!ctx || !rhsNode) {
        return NULL;
    }

    LLVMValueRef lhsBool = cg_build_truthy(ctx, lhsValue, NULL, isAnd ? "land.lhs" : "lor.lhs");
    if (!lhsBool) {
        fprintf(stderr, "Error: Failed to convert logical operand to boolean\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef lhsBlock = LLVMGetInsertBlock(ctx->builder);
    LLVMBasicBlockRef rhsBB = LLVMAppendBasicBlock(func, isAnd ? "land.rhs" : "lor.rhs");
    LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(func, isAnd ? "land.end" : "lor.end");

    if (isAnd) {
        LLVMBuildCondBr(ctx->builder, lhsBool, rhsBB, mergeBB);
    } else {
        LLVMBuildCondBr(ctx->builder, lhsBool, mergeBB, rhsBB);
    }

    LLVMPositionBuilderAtEnd(ctx->builder, rhsBB);
    LLVMValueRef rhsValue = codegenNode(ctx, rhsNode);
    if (!rhsValue) {
        fprintf(stderr, "Error: Failed to generate logical RHS\n");
        return NULL;
    }
    LLVMValueRef rhsBool = cg_build_truthy(ctx, rhsValue, NULL, isAnd ? "land.rhs.bool" : "lor.rhs.bool");
    if (!rhsBool) {
        fprintf(stderr, "Error: Failed to convert logical RHS to boolean\n");
        return NULL;
    }
    LLVMBasicBlockRef rhsEvalBB = LLVMGetInsertBlock(ctx->builder);
    bool rhsBranched = false;
    if (!LLVMGetBasicBlockTerminator(rhsEvalBB)) {
        LLVMBuildBr(ctx->builder, mergeBB);
        rhsBranched = true;
    }

    LLVMPositionBuilderAtEnd(ctx->builder, mergeBB);
    LLVMValueRef phi = LLVMBuildPhi(ctx->builder, LLVMInt1TypeInContext(ctx->llvmContext),
                                    isAnd ? "land.result" : "lor.result");
    LLVMValueRef incomingVals[2];
    LLVMBasicBlockRef incomingBlocks[2];
    unsigned incomingCount = 0;

    incomingVals[incomingCount] = LLVMConstInt(LLVMInt1TypeInContext(ctx->llvmContext),
                                               isAnd ? 0 : 1,
                                               0);
    incomingBlocks[incomingCount] = lhsBlock;
    incomingCount++;
    if (rhsBranched) {
        incomingVals[incomingCount] = rhsBool;
        incomingBlocks[incomingCount] = rhsEvalBB;
        incomingCount++;
    }
    LLVMAddIncoming(phi, incomingVals, incomingBlocks, incomingCount);
    return cg_widen_bool_to_int(ctx, phi, isAnd ? "land.int" : "lor.int");
}


static unsigned cg_default_int_bits(CodegenContext* ctx) {
    const TargetLayout* tl = cg_context_get_target_layout(ctx);
    return tl ? (unsigned)tl->intBits : 32;
}

static LLVMValueRef cg_apply_default_promotion(CodegenContext* ctx,
                                               LLVMValueRef arg,
                                               ASTNode* argNode) {
    if (!ctx || !arg) return arg;
    LLVMTypeRef ty = LLVMTypeOf(arg);
    if (!ty) return arg;

    LLVMContextRef llvmCtx = cg_context_get_llvm_context(ctx);
    LLVMTypeKind kind = LLVMGetTypeKind(ty);
    if (kind == LLVMFloatTypeKind) {
        return LLVMBuildFPExt(ctx->builder, arg, LLVMDoubleTypeInContext(llvmCtx), "vararg.fp.promote");
    }

    if (kind == LLVMIntegerTypeKind) {
        unsigned bits = LLVMGetIntTypeWidth(ty);
        unsigned targetBits = cg_default_int_bits(ctx);
        if (bits < targetBits) {
            bool isUnsigned = cg_expression_is_unsigned(ctx, argNode);
            LLVMTypeRef dest = LLVMIntTypeInContext(llvmCtx, targetBits);
            return LLVMBuildIntCast2(ctx->builder, arg, dest, !isUnsigned, "vararg.int.promote");
        }
    }

    return arg;
}

static LLVMValueRef cg_emit_va_intrinsic(CodegenContext* ctx,
                                         const char* intrinsicName,
                                         LLVMValueRef listValue) {
    if (!ctx || !intrinsicName || !listValue) return NULL;
    LLVMTypeRef listTy = LLVMTypeOf(listValue);
    if (!listTy || LLVMGetTypeKind(listTy) != LLVMPointerTypeKind) {
        return NULL;
    }
    LLVMContextRef llvmCtx = cg_context_get_llvm_context(ctx);
    LLVMTypeRef i8Ptr = LLVMPointerType(LLVMInt8TypeInContext(llvmCtx), 0);
    LLVMValueRef casted = LLVMBuildBitCast(ctx->builder, listValue, i8Ptr, "va.list.cast");

    LLVMTypeRef fnTy = LLVMFunctionType(LLVMVoidTypeInContext(llvmCtx), &i8Ptr, 1, 0);
    LLVMValueRef fn = LLVMGetNamedFunction(ctx->module, intrinsicName);
    if (!fn) {
        fn = LLVMAddFunction(ctx->module, intrinsicName, fnTy);
    }
    (void)LLVMBuildCall2(ctx->builder, fnTy, fn, &casted, 1, "");
    return NULL;
}

static LLVMValueRef cg_emit_rewritten_builtin_call(CodegenContext* ctx,
                                                   const char* targetName,
                                                   LLVMTypeRef returnType,
                                                   LLVMValueRef* sourceArgs,
                                                   size_t sourceCount,
                                                   const size_t* keepIndices,
                                                   size_t keepCount,
                                                   unsigned fixedParamCount,
                                                   const LLVMTypeRef* fixedParamTypes,
                                                   const bool* fixedParamSigned,
                                                   bool isVariadic,
                                                   const char* callName) {
    if (!ctx || !targetName || !returnType || !sourceArgs || !keepIndices || keepCount == 0) {
        return NULL;
    }

    if (fixedParamCount > keepCount) {
        fixedParamCount = (unsigned)keepCount;
    }

    LLVMValueRef* callArgs = (LLVMValueRef*)calloc(keepCount, sizeof(LLVMValueRef));
    if (!callArgs) {
        return NULL;
    }
    for (size_t i = 0; i < keepCount; ++i) {
        size_t idx = keepIndices[i];
        if (idx >= sourceCount || !sourceArgs[idx]) {
            free(callArgs);
            return NULL;
        }
        callArgs[i] = sourceArgs[idx];
        if (i < fixedParamCount && fixedParamTypes && fixedParamTypes[i] && LLVMTypeOf(callArgs[i]) != fixedParamTypes[i]) {
            LLVMTypeRef fromTy = LLVMTypeOf(callArgs[i]);
            LLVMTypeRef toTy = fixedParamTypes[i];
            LLVMTypeKind fromKind = fromTy ? LLVMGetTypeKind(fromTy) : LLVMVoidTypeKind;
            LLVMTypeKind toKind = LLVMGetTypeKind(toTy);
            if (fromKind == LLVMPointerTypeKind && toKind == LLVMPointerTypeKind) {
                callArgs[i] = LLVMBuildBitCast(ctx->builder, callArgs[i], toTy, "builtin.arg.ptrcast");
            } else if (fromKind == LLVMIntegerTypeKind && toKind == LLVMIntegerTypeKind) {
                unsigned fromBits = LLVMGetIntTypeWidth(fromTy);
                unsigned toBits = LLVMGetIntTypeWidth(toTy);
                if (fromBits != toBits) {
                    bool isSigned = fixedParamSigned ? fixedParamSigned[i] : false;
                    callArgs[i] = LLVMBuildIntCast2(ctx->builder, callArgs[i], toTy, isSigned, "builtin.arg.intcast");
                }
            } else if (fromKind == LLVMPointerTypeKind && toKind == LLVMIntegerTypeKind) {
                callArgs[i] = LLVMBuildPtrToInt(ctx->builder, callArgs[i], toTy, "builtin.arg.ptrtoint");
            } else if (fromKind == LLVMIntegerTypeKind && toKind == LLVMPointerTypeKind) {
                callArgs[i] = LLVMBuildIntToPtr(ctx->builder, callArgs[i], toTy, "builtin.arg.inttoptr");
            } else {
                free(callArgs);
                return NULL;
            }
        }
    }

    LLVMTypeRef* loweredParamTypes = NULL;
    if (fixedParamCount > 0) {
        LLVMTypeRef* paramTypes = (LLVMTypeRef*)calloc(fixedParamCount, sizeof(LLVMTypeRef));
        if (!paramTypes) {
            free(callArgs);
            return NULL;
        }
        for (unsigned i = 0; i < fixedParamCount; ++i) {
            if (fixedParamTypes && fixedParamTypes[i]) {
                paramTypes[i] = fixedParamTypes[i];
            } else {
                paramTypes[i] = LLVMTypeOf(callArgs[i]);
            }
            if (!paramTypes[i]) {
                paramTypes[i] = LLVMInt32TypeInContext(ctx->llvmContext);
            }
        }
        loweredParamTypes = paramTypes;
    }

    LLVMTypeRef fnTy = LLVMFunctionType(returnType,
                                        loweredParamTypes,
                                        fixedParamCount,
                                        isVariadic ? 1 : 0);
    free(loweredParamTypes);

    LLVMValueRef fn = LLVMGetNamedFunction(ctx->module, targetName);
    if (!fn) {
        fn = LLVMAddFunction(ctx->module, targetName, fnTy);
    }
    LLVMValueRef callee = fn;
    LLVMTypeRef calleePtrTy = LLVMPointerType(fnTy, 0);
    if (LLVMTypeOf(callee) != calleePtrTy) {
        callee = LLVMBuildBitCast(ctx->builder, callee, calleePtrTy, "builtin.call.cast");
    }

    bool returnsVoid = LLVMGetTypeKind(returnType) == LLVMVoidTypeKind;
    const char* emittedName = returnsVoid ? "" : (callName ? callName : "builtin.call");
    LLVMValueRef call = LLVMBuildCall2(ctx->builder,
                                       fnTy,
                                       callee,
                                       callArgs,
                                       (unsigned)keepCount,
                                       emittedName);
    free(callArgs);
    return returnsVoid ? NULL : call;
}


LLVMValueRef codegenFunctionCall(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_FUNCTION_CALL) {
        fprintf(stderr, "Error: Invalid node type for codegenFunctionCall\n");
        return NULL;
    }

    if (!node->functionCall.callee) {
        fprintf(stderr, "Error: Unsupported callee in function call\n");
        return NULL;
    }

    const ParsedType* calleeParsed = cg_resolve_expression_type(ctx, node->functionCall.callee);
    const char* calleeName = node->functionCall.callee->type == AST_IDENTIFIER
        ? node->functionCall.callee->valueNode.value
        : NULL;

    if (calleeName &&
        (strcmp(calleeName, "__builtin_offsetof") == 0 ||
         strcmp(calleeName, "offsetof") == 0)) {
        size_t offset = 0;
        bool ok = false;
        if (node->functionCall.argumentCount == 2 &&
            node->functionCall.arguments[0] &&
            node->functionCall.arguments[0]->type == AST_PARSED_TYPE &&
            node->functionCall.arguments[1] &&
            node->functionCall.arguments[1]->type == AST_IDENTIFIER) {
            ok = cg_eval_builtin_offsetof(ctx,
                                          &node->functionCall.arguments[0]->parsedTypeNode.parsed,
                                          node->functionCall.arguments[1]->valueNode.value,
                                          &offset);
        }
        const TargetLayout* tl = cg_context_get_target_layout(ctx);
        unsigned bits = (tl && tl->pointerBits) ? (unsigned)tl->pointerBits : 64;
        LLVMTypeRef offTy = LLVMIntTypeInContext(ctx->llvmContext, bits);
        return LLVMConstInt(offTy, ok ? (unsigned long long)offset : 0ULL, false);
    }
    if (calleeName && strcmp(calleeName, "__builtin_constant_p") == 0) {
        /* Conservative fallback: treat as non-constant when no dedicated
           constant-folding proof is available during codegen. */
        return LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, false);
    }

    LLVMValueRef function = codegenNode(ctx, node->functionCall.callee);
    if (!function) {
        fprintf(stderr, "Error: Undefined function %s\n",
                calleeName ? calleeName : "<expr>");
        return NULL;
    }

    // Opaque-pointer friendly: prefer semantic signature, only peel one level of pointer-to-function.

    bool isVaStartBuiltin = calleeName &&
        (strcmp(calleeName, "va_start") == 0 || strcmp(calleeName, "__builtin_va_start") == 0);
    bool isVaArgBuiltin = calleeName &&
        (strcmp(calleeName, "__builtin_va_arg") == 0 || strcmp(calleeName, "va_arg") == 0);
    bool isVaEndBuiltin = calleeName &&
        (strcmp(calleeName, "va_end") == 0 || strcmp(calleeName, "__builtin_va_end") == 0);
    bool isVaCopyBuiltin = calleeName &&
        (strcmp(calleeName, "__builtin_va_copy") == 0 || strcmp(calleeName, "va_copy") == 0);
    bool isObjectSizeBuiltin = calleeName && strcmp(calleeName, "__builtin_object_size") == 0;
    bool isFabsfBuiltin = calleeName && strcmp(calleeName, "__builtin_fabsf") == 0;
    bool isFabsBuiltin = calleeName && strcmp(calleeName, "__builtin_fabs") == 0;
    bool isFabslBuiltin = calleeName && strcmp(calleeName, "__builtin_fabsl") == 0;
    bool isInffBuiltin = calleeName && strcmp(calleeName, "__builtin_inff") == 0;
    bool isInfBuiltin = calleeName && strcmp(calleeName, "__builtin_inf") == 0;
    bool isInflBuiltin = calleeName && strcmp(calleeName, "__builtin_infl") == 0;
    bool isNanfBuiltin = calleeName && strcmp(calleeName, "__builtin_nanf") == 0;
    bool isNanBuiltin = calleeName && strcmp(calleeName, "__builtin_nan") == 0;
    bool isNanlBuiltin = calleeName && strcmp(calleeName, "__builtin_nanl") == 0;
    bool isHugeValfBuiltin = calleeName && strcmp(calleeName, "__builtin_huge_valf") == 0;
    bool isHugeValBuiltin = calleeName && strcmp(calleeName, "__builtin_huge_val") == 0;
    bool isHugeVallBuiltin = calleeName && strcmp(calleeName, "__builtin_huge_vall") == 0;
    bool isSnprintfChkBuiltin = calleeName && strcmp(calleeName, "__builtin___snprintf_chk") == 0;
    bool isVsnprintfChkBuiltin = calleeName && strcmp(calleeName, "__builtin___vsnprintf_chk") == 0;
    bool isSprintfChkBuiltin = calleeName && strcmp(calleeName, "__builtin___sprintf_chk") == 0;
    bool isMemcpyChkBuiltin = calleeName && strcmp(calleeName, "__builtin___memcpy_chk") == 0;
    bool isMemsetChkBuiltin = calleeName && strcmp(calleeName, "__builtin___memset_chk") == 0;
    bool isStrncpyChkBuiltin = calleeName && strcmp(calleeName, "__builtin___strncpy_chk") == 0;
    bool isStrncatChkBuiltin = calleeName && strcmp(calleeName, "__builtin___strncat_chk") == 0;
    bool isStrcpyChkBuiltin = calleeName && strcmp(calleeName, "__builtin___strcpy_chk") == 0;
    bool isStrcatChkBuiltin = calleeName && strcmp(calleeName, "__builtin___strcat_chk") == 0;
    bool isMemmoveChkBuiltin = calleeName && strcmp(calleeName, "__builtin___memmove_chk") == 0;

    LLVMValueRef* args = NULL;
    if (node->functionCall.argumentCount > 0) {
        args = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * node->functionCall.argumentCount);
        if (!args) return NULL;
        for (size_t i = 0; i < node->functionCall.argumentCount; i++) {
            if ((isVaStartBuiltin || isVaArgBuiltin || isVaEndBuiltin) && i == 0) {
                /* Keep va_list operand as an lvalue pointer for builtin varargs lowering. */
                args[i] = NULL;
                continue;
            }
            if (isVaCopyBuiltin && i < 2) {
                /* va_copy operands should be handled via lvalue loads/stores. */
                args[i] = NULL;
                continue;
            }
            args[i] = codegenNode(ctx, node->functionCall.arguments[i]);
        }
    }

    LLVMTypeRef i8PtrType = LLVMPointerType(LLVMInt8TypeInContext(ctx->llvmContext), 0);
    LLVMTypeRef sizeType = cg_get_intptr_type(ctx);
    LLVMTypeRef intType = LLVMInt32TypeInContext(ctx->llvmContext);

    if (isObjectSizeBuiltin) {
        free(args);
        /* Unknown object-size information: return (size_t)-1 like compilers do in fallback mode. */
        LLVMTypeRef resultTy = NULL;
        const Symbol* builtinSym = cg_lookup_function_symbol_for_callee(ctx, node->functionCall.callee);
        if (builtinSym && builtinSym->kind == SYMBOL_FUNCTION) {
            resultTy = cg_type_from_parsed(ctx, &builtinSym->type);
        }
        if (!resultTy || LLVMGetTypeKind(resultTy) != LLVMIntegerTypeKind) {
            resultTy = cg_get_intptr_type(ctx);
        }
        return LLVMConstAllOnes(resultTy);
    }
    if (isInffBuiltin || isInfBuiltin || isInflBuiltin ||
        isHugeValfBuiltin || isHugeValBuiltin || isHugeVallBuiltin) {
        free(args);
        LLVMTypeRef infTy = (isInffBuiltin || isHugeValfBuiltin)
            ? LLVMFloatTypeInContext(ctx->llvmContext)
            : LLVMDoubleTypeInContext(ctx->llvmContext);
        return LLVMConstReal(infTy, INFINITY);
    }
    if (isNanfBuiltin || isNanBuiltin || isNanlBuiltin) {
        free(args);
        LLVMTypeRef nanTy = isNanfBuiltin
            ? LLVMFloatTypeInContext(ctx->llvmContext)
            : LLVMDoubleTypeInContext(ctx->llvmContext);
        return LLVMConstReal(nanTy, NAN);
    }
    if (isFabsfBuiltin || isFabsBuiltin || isFabslBuiltin) {
        if (node->functionCall.argumentCount < 1) {
            free(args);
            return NULL;
        }
        const size_t keepIndices[] = {0};
        const char* targetName = isFabsfBuiltin ? "fabsf" : "fabs";
        LLVMTypeRef retTy = isFabsfBuiltin
            ? LLVMFloatTypeInContext(ctx->llvmContext)
            : LLVMDoubleTypeInContext(ctx->llvmContext);
        LLVMTypeRef argTy = retTy;
        LLVMValueRef call = cg_emit_rewritten_builtin_call(ctx,
                                                           targetName,
                                                           retTy,
                                                           args,
                                                           node->functionCall.argumentCount,
                                                           keepIndices,
                                                           1,
                                                           1,
                                                           &argTy,
                                                           (bool[]){false},
                                                           false,
                                                           isFabsfBuiltin
                                                               ? "fabsf.builtin.lowered"
                                                               : (isFabslBuiltin
                                                                      ? "fabsl.builtin.lowered"
                                                                      : "fabs.builtin.lowered"));
        free(args);
        return call;
    }
    if (isSnprintfChkBuiltin) {
        if (node->functionCall.argumentCount < 5) {
            free(args);
            return NULL;
        }
        size_t rewrittenCount = node->functionCall.argumentCount - 2;
        size_t* keepIndices = (size_t*)calloc(rewrittenCount, sizeof(size_t));
        if (!keepIndices) {
            free(args);
            return NULL;
        }
        keepIndices[0] = 0;
        keepIndices[1] = 1;
        for (size_t i = 2; i < rewrittenCount; ++i) {
            keepIndices[i] = i + 2;
        }
        LLVMValueRef call = cg_emit_rewritten_builtin_call(ctx,
                                                           "snprintf",
                                                           intType,
                                                           args,
                                                           node->functionCall.argumentCount,
                                                           keepIndices,
                                                           rewrittenCount,
                                                           3,
                                                           (LLVMTypeRef[]){i8PtrType, sizeType, i8PtrType},
                                                           (bool[]){false, false, false},
                                                           true,
                                                           "snprintf.chk.lowered");
        free(keepIndices);
        free(args);
        return call;
    }
    if (isVsnprintfChkBuiltin) {
        if (node->functionCall.argumentCount < 6) {
            free(args);
            return NULL;
        }
        const size_t keepIndices[] = {0, 1, 4, 5};
        LLVMValueRef call = cg_emit_rewritten_builtin_call(ctx,
                                                           "vsnprintf",
                                                           intType,
                                                           args,
                                                           node->functionCall.argumentCount,
                                                           keepIndices,
                                                           4,
                                                           4,
                                                           (LLVMTypeRef[]){i8PtrType, sizeType, i8PtrType, LLVMTypeOf(args[5])},
                                                           (bool[]){false, false, false, false},
                                                           false,
                                                           "vsnprintf.chk.lowered");
        free(args);
        return call;
    }
    if (isSprintfChkBuiltin) {
        if (node->functionCall.argumentCount < 4) {
            free(args);
            return NULL;
        }
        size_t rewrittenCount = node->functionCall.argumentCount - 2;
        size_t* keepIndices = (size_t*)calloc(rewrittenCount, sizeof(size_t));
        if (!keepIndices) {
            free(args);
            return NULL;
        }
        keepIndices[0] = 0;
        keepIndices[1] = 3;
        for (size_t i = 2; i < rewrittenCount; ++i) {
            keepIndices[i] = i + 2;
        }
        LLVMValueRef call = cg_emit_rewritten_builtin_call(ctx,
                                                           "sprintf",
                                                           intType,
                                                           args,
                                                           node->functionCall.argumentCount,
                                                           keepIndices,
                                                           rewrittenCount,
                                                           2,
                                                           (LLVMTypeRef[]){i8PtrType, i8PtrType},
                                                           (bool[]){false, false},
                                                           true,
                                                           "sprintf.chk.lowered");
        free(keepIndices);
        free(args);
        return call;
    }
    if (isMemcpyChkBuiltin) {
        if (node->functionCall.argumentCount < 4) {
            free(args);
            return NULL;
        }
        const size_t keepIndices[] = {0, 1, 2};
        LLVMValueRef call = cg_emit_rewritten_builtin_call(ctx,
                                                           "memcpy",
                                                           i8PtrType,
                                                           args,
                                                           node->functionCall.argumentCount,
                                                           keepIndices,
                                                           3,
                                                           3,
                                                           (LLVMTypeRef[]){i8PtrType, i8PtrType, sizeType},
                                                           (bool[]){false, false, false},
                                                           false,
                                                           "memcpy.chk.lowered");
        free(args);
        return call;
    }
    if (isMemsetChkBuiltin) {
        if (node->functionCall.argumentCount < 4) {
            free(args);
            return NULL;
        }
        const size_t keepIndices[] = {0, 1, 2};
        LLVMValueRef call = cg_emit_rewritten_builtin_call(ctx,
                                                           "memset",
                                                           i8PtrType,
                                                           args,
                                                           node->functionCall.argumentCount,
                                                           keepIndices,
                                                           3,
                                                           3,
                                                           (LLVMTypeRef[]){i8PtrType, intType, sizeType},
                                                           (bool[]){false, true, false},
                                                           false,
                                                           "memset.chk.lowered");
        free(args);
        return call;
    }
    if (isStrncpyChkBuiltin) {
        if (node->functionCall.argumentCount < 4) {
            free(args);
            return NULL;
        }
        const size_t keepIndices[] = {0, 1, 2};
        LLVMValueRef call = cg_emit_rewritten_builtin_call(ctx,
                                                           "strncpy",
                                                           i8PtrType,
                                                           args,
                                                           node->functionCall.argumentCount,
                                                           keepIndices,
                                                           3,
                                                           3,
                                                           (LLVMTypeRef[]){i8PtrType, i8PtrType, sizeType},
                                                           (bool[]){false, false, false},
                                                           false,
                                                           "strncpy.chk.lowered");
        free(args);
        return call;
    }
    if (isStrncatChkBuiltin) {
        if (node->functionCall.argumentCount < 4) {
            free(args);
            return NULL;
        }
        const size_t keepIndices[] = {0, 1, 2};
        LLVMValueRef call = cg_emit_rewritten_builtin_call(ctx,
                                                           "strncat",
                                                           i8PtrType,
                                                           args,
                                                           node->functionCall.argumentCount,
                                                           keepIndices,
                                                           3,
                                                           3,
                                                           (LLVMTypeRef[]){i8PtrType, i8PtrType, sizeType},
                                                           (bool[]){false, false, false},
                                                           false,
                                                           "strncat.chk.lowered");
        free(args);
        return call;
    }
    if (isStrcpyChkBuiltin) {
        if (node->functionCall.argumentCount < 3) {
            free(args);
            return NULL;
        }
        const size_t keepIndices[] = {0, 1};
        LLVMValueRef call = cg_emit_rewritten_builtin_call(ctx,
                                                           "strcpy",
                                                           i8PtrType,
                                                           args,
                                                           node->functionCall.argumentCount,
                                                           keepIndices,
                                                           2,
                                                           2,
                                                           (LLVMTypeRef[]){i8PtrType, i8PtrType},
                                                           (bool[]){false, false},
                                                           false,
                                                           "strcpy.chk.lowered");
        free(args);
        return call;
    }
    if (isStrcatChkBuiltin) {
        if (node->functionCall.argumentCount < 3) {
            free(args);
            return NULL;
        }
        const size_t keepIndices[] = {0, 1};
        LLVMValueRef call = cg_emit_rewritten_builtin_call(ctx,
                                                           "strcat",
                                                           i8PtrType,
                                                           args,
                                                           node->functionCall.argumentCount,
                                                           keepIndices,
                                                           2,
                                                           2,
                                                           (LLVMTypeRef[]){i8PtrType, i8PtrType},
                                                           (bool[]){false, false},
                                                           false,
                                                           "strcat.chk.lowered");
        free(args);
        return call;
    }
    if (isMemmoveChkBuiltin) {
        if (node->functionCall.argumentCount < 4) {
            free(args);
            return NULL;
        }
        const size_t keepIndices[] = {0, 1, 2};
        LLVMValueRef call = cg_emit_rewritten_builtin_call(ctx,
                                                           "memmove",
                                                           i8PtrType,
                                                           args,
                                                           node->functionCall.argumentCount,
                                                           keepIndices,
                                                           3,
                                                           3,
                                                           (LLVMTypeRef[]){i8PtrType, i8PtrType, sizeType},
                                                           (bool[]){false, false, false},
                                                           false,
                                                           "memmove.chk.lowered");
        free(args);
        return call;
    }

    if (isVaStartBuiltin) {
        LLVMValueRef result = NULL;
        if (node->functionCall.argumentCount >= 1) {
            LLVMValueRef listPtr = NULL;
            LLVMTypeRef listTy = NULL;
            CGLValueInfo linfo;
            if (codegenLValue(ctx, node->functionCall.arguments[0], &listPtr, &listTy, NULL, &linfo)) {
                result = cg_emit_va_intrinsic(ctx, "llvm.va_start", listPtr);
            } else if (args) {
                result = cg_emit_va_intrinsic(ctx, "llvm.va_start", args[0]);
            }
        }
        free(args);
        if (result) return result;
        return LLVMConstNull(LLVMInt32TypeInContext(ctx->llvmContext));
    }
    if (isVaArgBuiltin) {
        LLVMValueRef val = NULL;
        if (node->functionCall.argumentCount >= 2 &&
            node->functionCall.arguments[1] &&
            node->functionCall.arguments[1]->type == AST_PARSED_TYPE) {
            LLVMTypeRef resTy = cg_type_from_parsed(ctx, &node->functionCall.arguments[1]->parsedTypeNode.parsed);
            if (resTy) {
                LLVMValueRef listPtr = NULL;
                LLVMTypeRef listTy = NULL;
                CGLValueInfo linfo;
                if (codegenLValue(ctx, node->functionCall.arguments[0], &listPtr, &listTy, NULL, &linfo)) {
                    val = LLVMBuildVAArg(ctx->builder, listPtr, resTy, "vaarg");
                } else if (args && node->functionCall.argumentCount >= 1 && args[0]) {
                    val = LLVMBuildVAArg(ctx->builder, args[0], resTy, "vaarg");
                }
            }
        }
        free(args);
        if (val) return val;
        return LLVMConstNull(LLVMInt32TypeInContext(ctx->llvmContext));
    }
    if (isVaEndBuiltin) {
        LLVMValueRef result = NULL;
        if (node->functionCall.argumentCount >= 1) {
            LLVMValueRef listPtr = NULL;
            LLVMTypeRef listTy = NULL;
            CGLValueInfo linfo;
            if (codegenLValue(ctx, node->functionCall.arguments[0], &listPtr, &listTy, NULL, &linfo)) {
                result = cg_emit_va_intrinsic(ctx, "llvm.va_end", listPtr);
            } else if (args) {
                result = cg_emit_va_intrinsic(ctx, "llvm.va_end", args[0]);
            }
        }
        free(args);
        if (result) return result;
        return LLVMConstNull(LLVMInt32TypeInContext(ctx->llvmContext));
    }
    if (isVaCopyBuiltin) {
        LLVMValueRef result = NULL;
        if (node->functionCall.argumentCount >= 2) {
            LLVMValueRef dstPtr = NULL;
            LLVMTypeRef dstTy = NULL;
            CGLValueInfo dstInfo;
            LLVMValueRef srcPtr = NULL;
            LLVMTypeRef srcTy = NULL;
            CGLValueInfo srcInfo;
            LLVMValueRef srcVal = NULL;

            bool haveDst = codegenLValue(ctx, node->functionCall.arguments[0], &dstPtr, &dstTy, NULL, &dstInfo);
            bool haveSrc = codegenLValue(ctx, node->functionCall.arguments[1], &srcPtr, &srcTy, NULL, &srcInfo);

            if (haveSrc && srcPtr) {
                LLVMTypeRef loadTy = srcTy;
                if (!loadTy) {
                    loadTy = cg_dereference_ptr_type(ctx, LLVMTypeOf(srcPtr), "va_copy src");
                }
                if (loadTy && LLVMGetTypeKind(loadTy) != LLVMVoidTypeKind) {
                    srcVal = LLVMBuildLoad2(ctx->builder, loadTy, srcPtr, "va.copy.load");
                }
            } else if (args) {
                srcVal = args[1];
            }

            if (haveDst && dstPtr && srcVal) {
                LLVMTypeRef storeTy = dstTy;
                if (!storeTy) {
                    storeTy = cg_dereference_ptr_type(ctx, LLVMTypeOf(dstPtr), "va_copy dst");
                }
                if (!storeTy || LLVMGetTypeKind(storeTy) == LLVMVoidTypeKind) {
                    storeTy = LLVMTypeOf(srcVal);
                }
                if (storeTy && LLVMTypeOf(srcVal) != storeTy) {
                    LLVMTypeKind srcKind = LLVMGetTypeKind(LLVMTypeOf(srcVal));
                    LLVMTypeKind dstKind = LLVMGetTypeKind(storeTy);
                    if (srcKind == LLVMPointerTypeKind && dstKind == LLVMPointerTypeKind) {
                        srcVal = LLVMBuildBitCast(ctx->builder, srcVal, storeTy, "va.copy.cast");
                    } else if (srcKind == LLVMIntegerTypeKind && dstKind == LLVMIntegerTypeKind) {
                        bool srcSigned = !cg_expression_is_unsigned(ctx, node->functionCall.arguments[1]);
                        srcVal = LLVMBuildIntCast2(ctx->builder, srcVal, storeTy, srcSigned, "va.copy.cast");
                    }
                }
                result = LLVMBuildStore(ctx->builder, srcVal, dstPtr);
            }
        }
        free(args);
        if (result) return result;
        return LLVMConstNull(LLVMInt32TypeInContext(ctx->llvmContext));
    }

    if (calleeName && strcmp(calleeName, "__builtin_expect") == 0) {
        if (node->functionCall.argumentCount >= 2 && args && args[0] && args[1]) {
            LLVMValueRef val = args[0];
            LLVMValueRef expectVal = args[1];
            LLVMTypeRef valTy = LLVMTypeOf(val);
            if (!valTy || LLVMGetTypeKind(valTy) != LLVMIntegerTypeKind) {
                valTy = cg_get_intptr_type(ctx);
                val = ensureIntegerLike(ctx, val);
            }
            LLVMTypeRef expectTy = LLVMTypeOf(expectVal);
            if (!expectTy || LLVMGetTypeKind(expectTy) != LLVMIntegerTypeKind ||
                LLVMGetIntTypeWidth(expectTy) != LLVMGetIntTypeWidth(valTy)) {
                expectVal = LLVMBuildIntCast2(ctx->builder, expectVal, valTy, 0, "expect.cast");
            }

            unsigned bits = LLVMGetIntTypeWidth(valTy);
            if (bits == 0) bits = 64;
            char name[32];
            snprintf(name, sizeof(name), "llvm.expect.i%u", bits);
            LLVMTypeRef fnTy = LLVMFunctionType(valTy, (LLVMTypeRef[]){ valTy, valTy }, 2, 0);
            LLVMValueRef fn = LLVMGetNamedFunction(ctx->module, name);
            if (!fn) {
                fn = LLVMAddFunction(ctx->module, name, fnTy);
            }
            LLVMValueRef call = LLVMBuildCall2(ctx->builder, fnTy, fn, (LLVMValueRef[]){ val, expectVal }, 2, "expect");
            free(args);
            return call;
        }
        free(args);
        return LLVMConstNull(LLVMInt32TypeInContext(ctx->llvmContext));
    }

    const Symbol* sym = cg_lookup_function_symbol_for_callee(ctx, node->functionCall.callee);
    bool noPrototype = sym && sym->kind == SYMBOL_FUNCTION && !sym->signature.hasPrototype;
    bool externalAbiEligible =
        cg_symbol_declared_in_system_header(sym) ||
        cg_is_known_external_abi_function_name(calleeName);

    LLVMTypeRef calleeType = NULL;
    if (sym && !noPrototype) {
        calleeType = cg_function_type_from_symbol(ctx, sym);
    }
    if (calleeParsed && !noPrototype) {
        if (!calleeType) {
            calleeType = functionTypeFromPointerParsed(ctx,
                                                       calleeParsed,
                                                       node->functionCall.argumentCount,
                                                       args);
        }
    }
    if (!calleeType && !noPrototype) {
        calleeType = LLVMTypeOf(function);
        if (calleeType && LLVMGetTypeKind(calleeType) == LLVMPointerTypeKind) {
            LLVMTypeRef element = cg_dereference_ptr_type(ctx, calleeType, "call target");
            if (element && LLVMGetTypeKind(element) == LLVMFunctionTypeKind) {
                calleeType = element;
            }
        }
    }
    if ((!calleeType || LLVMGetTypeKind(calleeType) != LLVMFunctionTypeKind) &&
        sym && !noPrototype) {
        calleeType = cg_function_type_from_symbol(ctx, sym);
    }

    size_t argCount = node->functionCall.argumentCount;
    LLVMValueRef* finalArgs = args;
    LLVMValueRef* promotedArgs = NULL;
    LLVMValueRef* externalAbiArgs = NULL;
    LLVMTypeRef* externalAbiParamTypes = NULL;
    bool externalAbiCallAdjusted = false;

    if (noPrototype) {
        if (argCount > 0 && args) {
            promotedArgs = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * argCount);
            if (promotedArgs) {
                for (size_t i = 0; i < argCount; ++i) {
                    promotedArgs[i] = cg_apply_default_promotion(ctx, args[i], node->functionCall.arguments[i]);
                }
                finalArgs = promotedArgs;
            }
        }

        LLVMTypeRef retType = NULL;
        if (sym) {
            retType = cg_type_from_parsed(ctx, &sym->type);
            if (retType && LLVMGetTypeKind(retType) == LLVMFunctionTypeKind) {
                retType = LLVMPointerType(retType, 0);
            } else if (retType && LLVMGetTypeKind(retType) == LLVMArrayTypeKind) {
                retType = LLVMPointerType(retType, 0);
            }
        }
        if (!retType || LLVMGetTypeKind(retType) == LLVMVoidTypeKind) {
            retType = LLVMVoidTypeInContext(ctx->llvmContext);
        }

        LLVMTypeRef* paramTypes = NULL;
        if (argCount > 0) {
            paramTypes = (LLVMTypeRef*)calloc(argCount, sizeof(LLVMTypeRef));
            if (!paramTypes) return NULL;
            for (size_t i = 0; i < argCount; ++i) {
                LLVMValueRef val = finalArgs ? finalArgs[i] : NULL;
                LLVMTypeRef argTy = val ? LLVMTypeOf(val) : LLVMInt32TypeInContext(ctx->llvmContext);
                LLVMTypeRef coercedTy = externalAbiEligible
                    ? cg_external_abi_coerce_param_type(ctx, argTy)
                    : argTy;
                if (externalAbiEligible && coercedTy != argTy && finalArgs && val) {
                    finalArgs[i] = cg_pack_aggregate_for_external_abi(ctx, val, coercedTy, "call.noproto.extabi");
                    val = finalArgs[i];
                    argTy = LLVMTypeOf(val);
                }
                paramTypes[i] = coercedTy ? coercedTy : argTy;
                if (!paramTypes[i]) {
                    paramTypes[i] = LLVMInt32TypeInContext(ctx->llvmContext);
                }
            }
        }
        calleeType = LLVMFunctionType(retType, paramTypes, (unsigned)argCount, 0);
        free(paramTypes);
    }

    if ((!calleeType || LLVMGetTypeKind(calleeType) != LLVMFunctionTypeKind) &&
        function &&
        LLVMGetTypeKind(LLVMTypeOf(function)) == LLVMPointerTypeKind) {
        /* Opaque pointer fallback: infer a callable signature from the call
         * site so function-pointer expressions still lower correctly. */
        LLVMTypeRef inferredRet = NULL;
        const ParsedType* inferredRetParsed = cg_refine_function_call_result_type(ctx, node);
        if (!inferredRetParsed) {
            inferredRetParsed = cg_resolve_expression_type(ctx, node);
        }
        if (inferredRetParsed) {
            inferredRet = cg_type_from_parsed(ctx, inferredRetParsed);
            if (inferredRet && LLVMGetTypeKind(inferredRet) == LLVMFunctionTypeKind) {
                inferredRet = LLVMPointerType(inferredRet, 0);
            } else if (inferredRet && LLVMGetTypeKind(inferredRet) == LLVMArrayTypeKind) {
                inferredRet = LLVMPointerType(inferredRet, 0);
            }
        }
        if (!inferredRet) {
            inferredRet = LLVMInt32TypeInContext(ctx->llvmContext);
        }

        LLVMTypeRef* inferredParams = NULL;
        if (argCount > 0) {
            inferredParams = (LLVMTypeRef*)calloc(argCount, sizeof(LLVMTypeRef));
            if (!inferredParams) {
                if (promotedArgs) free(promotedArgs);
                free(args);
                return NULL;
            }
            for (size_t i = 0; i < argCount; ++i) {
                LLVMValueRef val = finalArgs ? finalArgs[i] : NULL;
                inferredParams[i] = val ? LLVMTypeOf(val) : LLVMInt32TypeInContext(ctx->llvmContext);
                if (!inferredParams[i]) {
                    inferredParams[i] = LLVMInt32TypeInContext(ctx->llvmContext);
                }
            }
        }
        calleeType = LLVMFunctionType(inferredRet, inferredParams, (unsigned)argCount, 0);
        free(inferredParams);
        function = LLVMBuildBitCast(ctx->builder,
                                    function,
                                    LLVMPointerType(calleeType, 0),
                                    "call.infer.cast");
    }

    if (!calleeType || LLVMGetTypeKind(calleeType) != LLVMFunctionTypeKind) {
        fprintf(stderr, "Error: call target is not a function type\n");
        if (promotedArgs) free(promotedArgs);
        free(args);
        return NULL;
    }

    if (!noPrototype) {
        const ParsedType* refinedReturnParsed = cg_refine_function_call_result_type(ctx, node);
        if (refinedReturnParsed) {
            LLVMTypeRef refinedReturn = cg_type_from_parsed(ctx, refinedReturnParsed);
            if (refinedReturn && LLVMGetTypeKind(refinedReturn) == LLVMFunctionTypeKind) {
                refinedReturn = LLVMPointerType(refinedReturn, 0);
            } else if (refinedReturn && LLVMGetTypeKind(refinedReturn) == LLVMArrayTypeKind) {
                refinedReturn = LLVMPointerType(refinedReturn, 0);
            }
            if (refinedReturn && LLVMGetTypeKind(refinedReturn) != LLVMVoidTypeKind) {
                LLVMTypeRef currentReturn = LLVMGetReturnType(calleeType);
                const ParsedType* semanticCallResult = cg_resolve_expression_type(ctx, node);
                bool semanticResultPointerish = false;
                if (semanticCallResult) {
                    if (semanticCallResult->pointerDepth > 0 || semanticCallResult->isFunctionPointer) {
                        semanticResultPointerish = true;
                    } else {
                        for (size_t d = 0; d < semanticCallResult->derivationCount; ++d) {
                            const TypeDerivation* deriv = parsedTypeGetDerivation(semanticCallResult, d);
                            if (deriv && deriv->kind == TYPE_DERIVATION_POINTER) {
                                semanticResultPointerish = true;
                                break;
                            }
                        }
                    }
                }
                bool unsafePointerDowngrade =
                    semanticResultPointerish &&
                    currentReturn &&
                    refinedReturn &&
                    LLVMGetTypeKind(currentReturn) == LLVMPointerTypeKind &&
                    LLVMGetTypeKind(refinedReturn) != LLVMPointerTypeKind;
                if (!unsafePointerDowngrade && currentReturn != refinedReturn) {
                    unsigned fixedCount = LLVMCountParamTypes(calleeType);
                    LLVMTypeRef* fixedTypes = NULL;
                    if (fixedCount > 0) {
                        fixedTypes = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * fixedCount);
                        if (fixedTypes) {
                            LLVMGetParamTypes(calleeType, fixedTypes);
                        }
                    }
                    bool isVar = LLVMIsFunctionVarArg(calleeType);
                    LLVMTypeRef rebuilt = LLVMFunctionType(refinedReturn, fixedTypes, fixedCount, isVar ? 1 : 0);
                    free(fixedTypes);
                    calleeType = rebuilt;
                    LLVMTypeRef fnPtrType = LLVMPointerType(calleeType, 0);
                    function = LLVMBuildBitCast(ctx->builder, function, fnPtrType, "call.retfix.cast");
                }
            }
        }
    }

    char* resolvedFnType = LLVMPrintTypeToString(calleeType);
    CG_DEBUG("[CG] Function call resolved type: %s\n", resolvedFnType ? resolvedFnType : "<null>");
    if (resolvedFnType) LLVMDisposeMessage(resolvedFnType);
    if (!noPrototype && sym && sym->signature.hasPrototype && sym->signature.paramCount > 0 && args) {
        size_t fixedCount = sym->signature.paramCount;
        if (fixedCount > argCount) {
            fixedCount = argCount;
        }
        for (size_t i = 0; i < fixedCount; ++i) {
            const ParsedType* fromParsed = cg_resolve_expression_type(ctx, node->functionCall.arguments[i]);

            if (externalAbiEligible) {
                LLVMTypeRef extParamTy = cg_type_from_parsed(ctx, &sym->signature.params[i]);
                if (extParamTy && LLVMGetTypeKind(extParamTy) == LLVMFunctionTypeKind) {
                    extParamTy = LLVMPointerType(extParamTy, 0);
                } else if (extParamTy && LLVMGetTypeKind(extParamTy) == LLVMArrayTypeKind) {
                    extParamTy = LLVMPointerType(extParamTy, 0);
                }
                if (!extParamTy || LLVMGetTypeKind(extParamTy) == LLVMVoidTypeKind) {
                    continue;
                }
                args[i] = cg_cast_value(ctx,
                                        args[i],
                                        extParamTy,
                                        fromParsed,
                                        &sym->signature.params[i],
                                        "call.arg.extabi.cast");
                continue;
            }

            bool passIndirect = false;
            LLVMTypeRef valueParamTy = NULL;
            LLVMTypeRef paramTy = cg_lower_parameter_type(ctx,
                                                          &sym->signature.params[i],
                                                          &passIndirect,
                                                          &valueParamTy);
            if (!paramTy || LLVMGetTypeKind(paramTy) == LLVMVoidTypeKind) {
                continue;
            }
            if (passIndirect && valueParamTy) {
                LLVMValueRef argVal = args[i];
                if (argVal && LLVMTypeOf(argVal) != valueParamTy) {
                    argVal = cg_cast_value(ctx,
                                           argVal,
                                           valueParamTy,
                                           fromParsed,
                                           &sym->signature.params[i],
                                           "call.arg.byval.cast");
                }
                if (argVal && LLVMTypeOf(argVal) == valueParamTy) {
                    LLVMValueRef tmp = cg_build_entry_alloca(ctx, valueParamTy, "call.arg.byval.tmp");
                    LLVMBuildStore(ctx->builder, argVal, tmp);
                    args[i] = tmp;
                }
            } else {
                args[i] = cg_cast_value(ctx,
                                        args[i],
                                        paramTy,
                                        fromParsed,
                                        &sym->signature.params[i],
                                        "call.arg.cast");
            }
        }
    }
    if (noPrototype) {
        LLVMTypeRef fnPtrTy = LLVMPointerType(calleeType, 0);
        function = LLVMBuildBitCast(ctx->builder, function, fnPtrTy, "call.cast");
    } else {
        bool isVariadic = LLVMIsFunctionVarArg(calleeType);
        unsigned fixedParams = LLVMCountParamTypes(calleeType);

        if (isVariadic && argCount > fixedParams && args) {
            promotedArgs = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * argCount);
            if (promotedArgs) {
                for (size_t i = 0; i < argCount; ++i) {
                    LLVMValueRef val = args[i];
                    if (i >= fixedParams) {
                        val = cg_apply_default_promotion(ctx, val, node->functionCall.arguments[i]);
                    }
                    promotedArgs[i] = val;
                }
                finalArgs = promotedArgs;
            }
        }
    }

    if (!noPrototype &&
        function &&
        LLVMIsAFunction(function) &&
        externalAbiEligible &&
        cg_is_external_decl_function(function)) {
        unsigned fixedParamCount = LLVMCountParamTypes(calleeType);
        bool isVariadic = LLVMIsFunctionVarArg(calleeType) != 0;
        if (fixedParamCount > 0) {
            externalAbiParamTypes = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * fixedParamCount);
            if (externalAbiParamTypes) {
                LLVMGetParamTypes(calleeType, externalAbiParamTypes);
                if (argCount > 0) {
                    externalAbiArgs = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * argCount);
                    if (externalAbiArgs) {
                        for (size_t i = 0; i < argCount; ++i) {
                            externalAbiArgs[i] = finalArgs[i];
                        }
                    }
                }

                for (unsigned i = 0; i < fixedParamCount; ++i) {
                    LLVMTypeRef sourceTy = externalAbiParamTypes[i];
                    LLVMTypeRef desiredTy = sourceTy;
                    if (sym && i < sym->signature.paramCount) {
                        LLVMTypeRef parsedTy = cg_type_from_parsed(ctx, &sym->signature.params[i]);
                        if (parsedTy && LLVMGetTypeKind(parsedTy) == LLVMFunctionTypeKind) {
                            parsedTy = LLVMPointerType(parsedTy, 0);
                        } else if (parsedTy && LLVMGetTypeKind(parsedTy) == LLVMArrayTypeKind) {
                            parsedTy = LLVMPointerType(parsedTy, 0);
                        }
                        if (parsedTy && LLVMGetTypeKind(parsedTy) != LLVMVoidTypeKind) {
                            desiredTy = parsedTy;
                        }
                    }
                    LLVMTypeRef coercedTy = cg_external_abi_coerce_param_type(ctx, desiredTy);
                    externalAbiParamTypes[i] = coercedTy;
                    if (coercedTy != sourceTy) {
                        externalAbiCallAdjusted = true;
                    }
                    if (externalAbiArgs && i < argCount) {
                        LLVMValueRef arg = externalAbiArgs[i];
                        if (arg && LLVMTypeOf(arg) != coercedTy) {
                            LLVMTypeKind argKind = LLVMGetTypeKind(LLVMTypeOf(arg));
                            LLVMTypeKind dstKind = LLVMGetTypeKind(coercedTy);
                            if ((argKind == LLVMStructTypeKind || argKind == LLVMArrayTypeKind) &&
                                (dstKind == LLVMIntegerTypeKind || dstKind == LLVMArrayTypeKind)) {
                                arg = cg_pack_aggregate_for_external_abi(ctx, arg, coercedTy, "call.extabi.arg");
                            } else {
                                const ParsedType* fromParsed = NULL;
                                const ParsedType* toParsed = NULL;
                                if (sym && i < sym->signature.paramCount) {
                                    toParsed = &sym->signature.params[i];
                                }
                                if (node->functionCall.arguments && i < node->functionCall.argumentCount) {
                                    fromParsed = cg_resolve_expression_type(ctx, node->functionCall.arguments[i]);
                                }
                                arg = cg_cast_value(ctx,
                                                    arg,
                                                    coercedTy,
                                                    fromParsed,
                                                    toParsed,
                                                    "call.extabi.cast");
                            }
                            externalAbiArgs[i] = arg;
                            externalAbiCallAdjusted = true;
                        }
                    }
                }

                if (externalAbiCallAdjusted) {
                    LLVMTypeRef adjustedType = LLVMFunctionType(LLVMGetReturnType(calleeType),
                                                                externalAbiParamTypes,
                                                                fixedParamCount,
                                                                isVariadic ? 1 : 0);
                    calleeType = adjustedType;
                    function = LLVMBuildBitCast(ctx->builder,
                                                function,
                                                LLVMPointerType(calleeType, 0),
                                                "call.extabi.cast");
                    if (externalAbiArgs) {
                        finalArgs = externalAbiArgs;
                    }
                }
            }
        }
    }

    LLVMTypeRef callRetTy = LLVMGetReturnType(calleeType);
    bool callReturnsVoid = callRetTy && LLVMGetTypeKind(callRetTy) == LLVMVoidTypeKind;
    LLVMValueRef call = LLVMBuildCall2(ctx->builder,
                                       calleeType,
                                       function,
                                       finalArgs,
                                       argCount,
                                       callReturnsVoid ? "" : "calltmp");
    /* Mirror the callee's calling convention onto the call instruction so
     * LLVM lowers it consistently with the function declaration. */
    unsigned fnCC = LLVMGetFunctionCallConv(function);
    if (fnCC != 0) {
        LLVMSetInstructionCallConv(call, fnCC);
    }
    if (externalAbiParamTypes) free(externalAbiParamTypes);
    if (externalAbiArgs) free(externalAbiArgs);
    if (promotedArgs) free(promotedArgs);
    free(args);
    return callReturnsVoid ? NULL : call;
}


LLVMValueRef codegenCastExpression(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_CAST_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenCastExpression\n");
        return NULL;
    }

    LLVMValueRef value = codegenNode(ctx, node->castExpr.expression);
    if (!value) {
        return NULL;
    }

    LLVMTypeRef target = cg_type_from_parsed(ctx, &node->castExpr.castType);
    if (!target || LLVMGetTypeKind(target) == LLVMVoidTypeKind) {
        return value; /* no conversion performed yet */
    }

    LLVMTypeRef valueType = LLVMTypeOf(value);
    if (valueType == target) {
        return value;
    }

    const ParsedType* fromParsed = cg_resolve_expression_type(ctx, node->castExpr.expression);
    return cg_cast_value(ctx, value, target, fromParsed, &node->castExpr.castType, "casttmp");
}

static LLVMValueRef cg_materialize_compound_literal(CodegenContext* ctx,
                                                    ASTNode* node,
                                                    LLVMTypeRef* outLiteralType) {
    if (!ctx || !node || node->type != AST_COMPOUND_LITERAL) {
        return NULL;
    }
    LLVMTypeRef literalType = cg_type_from_parsed(ctx, &node->compoundLiteral.literalType);
    if (!literalType || LLVMGetTypeKind(literalType) == LLVMVoidTypeKind) {
        literalType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    if (outLiteralType) {
        *outLiteralType = literalType;
    }

    if (node->compoundLiteral.isStaticStorage) {
        LLVMValueRef constInit = cg_build_const_initializer(ctx,
                                                            node,
                                                            literalType,
                                                            &node->compoundLiteral.literalType);
        if (constInit) {
            static unsigned counter = 0;
            char name[64];
            snprintf(name, sizeof(name), ".compound.static.%u", counter++);
            LLVMValueRef global = LLVMAddGlobal(ctx->module, literalType, name);
            LLVMSetLinkage(global, LLVMPrivateLinkage);
            LLVMSetGlobalConstant(global, 1);
            LLVMSetInitializer(global, constInit);
            LLVMSetUnnamedAddr(global, LLVMGlobalUnnamedAddr);
            return global;
        }
    }

    LLVMValueRef tmp = cg_build_entry_alloca(ctx, literalType, "compound.tmp");
    if (!cg_store_compound_literal_into_ptr(ctx,
                                            tmp,
                                            literalType,
                                            &node->compoundLiteral.literalType,
                                            node)) {
        return NULL;
    }
    return tmp;
}

LLVMValueRef cg_emit_compound_literal_pointer(CodegenContext* ctx, ASTNode* node, LLVMTypeRef* outType) {
    LLVMTypeRef literalType = NULL;
    LLVMValueRef ptr = cg_materialize_compound_literal(ctx, node, &literalType);
    if (!ptr) return NULL;
    if (outType) {
        *outType = literalType;
    }
    return ptr;
}

LLVMValueRef codegenCompoundLiteral(CodegenContext* ctx, ASTNode* node) {
    if (!node || node->type != AST_COMPOUND_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenCompoundLiteral\n");
        return NULL;
    }

    LLVMTypeRef literalType = NULL;
    LLVMValueRef ptr = cg_materialize_compound_literal(ctx, node, &literalType);
    if (!ptr || !literalType || LLVMGetTypeKind(literalType) == LLVMVoidTypeKind) {
        fprintf(stderr, "Error: Failed to emit compound literal\n");
        return NULL;
    }

    return LLVMBuildLoad2(ctx->builder, literalType, ptr, "compound.value");
}


LLVMValueRef codegenNumberLiteral(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_NUMBER_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenNumberLiteral\n");
        return NULL;
    }

    const ParsedType* parsed = cg_resolve_expression_type(ctx, node);
    if (parsed && parsed->kind == TYPE_PRIMITIVE &&
        (parsed->primitiveType == TOKEN_FLOAT || parsed->primitiveType == TOKEN_DOUBLE)) {
        const char* text = node->valueNode.value ? node->valueNode.value : "0";
        char buf[128];
        size_t len = strlen(text);
        size_t outLen = 0;
        for (size_t i = 0; i < len && outLen + 1 < sizeof(buf); ++i) {
            char c = text[i];
            if (c == 'i' || c == 'I' || c == 'j' || c == 'J' ||
                c == 'f' || c == 'F' || c == 'l' || c == 'L') {
                continue;
            }
            buf[outLen++] = c;
        }
        buf[outLen] = '\0';
        double value = strtod(buf, NULL);
        LLVMTypeRef floatTy = cg_type_from_parsed(ctx, parsed);
        if (floatTy && LLVMGetTypeKind(floatTy) == LLVMStructTypeKind) {
            floatTy = LLVMDoubleTypeInContext(ctx->llvmContext);
        }
        if (!floatTy || LLVMGetTypeKind(floatTy) == LLVMVoidTypeKind) {
            floatTy = LLVMDoubleTypeInContext(ctx->llvmContext);
        }
        return LLVMConstReal(floatTy, value);
    }

    const ParsedType* parsedInt = cg_resolve_expression_type(ctx, node);
    LLVMTypeRef ty = parsedInt ? cg_type_from_parsed(ctx, parsedInt) : NULL;
    if (!ty || LLVMGetTypeKind(ty) == LLVMVoidTypeKind) {
        ty = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    IntegerLiteralInfo info = {0};
    unsigned long long value = 0;
    if (parse_integer_literal_info(node->valueNode.value ? node->valueNode.value : "0",
                                   cg_context_get_target_layout(ctx),
                                   &info) && info.ok) {
        value = info.value;
    }

    return LLVMConstInt(ty, value, (info.ok && !info.isUnsigned) ? 1 : 0);
}


LLVMValueRef codegenCharLiteral(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_CHAR_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenCharLiteral\n");
        return NULL;
    }

    const char* payload = NULL;
    ast_literal_encoding(node->valueNode.value, &payload);
    long long value = 0;
    if (payload) {
        char* endptr = NULL;
        value = strtoll(payload, &endptr, 0);
        if (endptr == payload) {
            value = (unsigned char)payload[0];
        }
    }
    LLVMTypeRef ty = LLVMInt32TypeInContext(ctx->llvmContext);
    return LLVMConstInt(ty, (unsigned long long)value, 0);
}

static LLVMValueRef cg_build_wide_string_global(CodegenContext* ctx,
                                                const char* rawPayload) {
    if (!ctx) return NULL;
    uint32_t* codepoints = NULL;
    size_t count = 0;
    LiteralDecodeResult res = decode_c_string_literal_wide(rawPayload ? rawPayload : "",
                                                           32,
                                                           &codepoints,
                                                           &count);
    (void)res;
    LLVMTypeRef elemTy = LLVMInt32TypeInContext(ctx->llvmContext);
    LLVMTypeRef arrayTy = LLVMArrayType(elemTy, (unsigned)(count + 1));
    LLVMValueRef* values = (LLVMValueRef*)calloc(count + 1, sizeof(LLVMValueRef));
    if (!values) {
        free(codepoints);
        return NULL;
    }
    for (size_t i = 0; i < count; ++i) {
        values[i] = LLVMConstInt(elemTy, codepoints ? codepoints[i] : 0, 0);
    }
    values[count] = LLVMConstInt(elemTy, 0, 0);

    static unsigned counter = 0;
    char name[32];
    snprintf(name, sizeof(name), ".wstr.%u", counter++);
    LLVMValueRef global = LLVMAddGlobal(ctx->module, arrayTy, name);
    LLVMSetLinkage(global, LLVMPrivateLinkage);
    LLVMSetInitializer(global, LLVMConstArray(elemTy, values, (unsigned)(count + 1)));
    LLVMSetGlobalConstant(global, true);
    LLVMSetUnnamedAddr(global, LLVMGlobalUnnamedAddr);

    LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
    LLVMValueRef indices[2] = { zero, zero };
    LLVMValueRef gep = LLVMConstGEP2(arrayTy, global, indices, 2);

    free(values);
    free(codepoints);
    return gep;
}

LLVMValueRef codegenStringLiteral(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_STRING_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenStringLiteral\n");
        return NULL;
    }

    const char* payload = NULL;
    LiteralEncoding enc = ast_literal_encoding(node->valueNode.value, &payload);
    if (enc == LIT_ENC_WIDE) {
        return cg_build_wide_string_global(ctx, payload ? payload : "");
    }

    char* decoded = NULL;
    size_t decodedLen = 0;
    LiteralDecodeResult res = decode_c_string_literal(payload ? payload : "", 8, &decoded, &decodedLen);
    (void)res;
    if (decoded) {
        LLVMValueRef strConst = LLVMConstStringInContext(ctx->llvmContext, decoded, (unsigned)decodedLen, false);
        LLVMTypeRef arrTy = LLVMTypeOf(strConst);
        static unsigned counter = 0;
        char name[32];
        snprintf(name, sizeof(name), ".str.%u", counter++);
        LLVMValueRef global = LLVMAddGlobal(ctx->module, arrTy, name);
        LLVMSetLinkage(global, LLVMPrivateLinkage);
        LLVMSetInitializer(global, strConst);
        LLVMSetGlobalConstant(global, true);
        LLVMSetUnnamedAddr(global, LLVMGlobalUnnamedAddr);
        LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
        LLVMValueRef indices[2] = { zero, zero };
        LLVMValueRef gep = LLVMConstGEP2(arrTy, global, indices, 2);
        free(decoded);
        return gep;
    }

    const char* text = payload ? payload : node->valueNode.value;
    return LLVMBuildGlobalStringPtr(ctx->builder, text ? text : "", "str");
}


LLVMValueRef codegenIdentifier(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_IDENTIFIER) {
        fprintf(stderr, "Error: Invalid node type for codegenIdentifier\n");
        return NULL;
    }

    if (node->valueNode.value && strcmp(node->valueNode.value, "__func__") == 0) {
        const char* fnName = ctx->currentFunctionName ? ctx->currentFunctionName : "";
        return LLVMBuildGlobalStringPtr(ctx->builder, fnName, "__func__");
    }

    NamedValue* entry = cg_scope_lookup(ctx->currentScope, node->valueNode.value);
    if (entry) {
        bool entryIsArray = entry->parsedType && parsedTypeIsDirectArray(entry->parsedType);
        if (entryIsArray) {
            LLVMTypeRef basePtrTy = LLVMTypeOf(entry->value);
            if (basePtrTy &&
                LLVMGetTypeKind(basePtrTy) == LLVMPointerTypeKind &&
                !LLVMPointerTypeIsOpaque(basePtrTy)) {
                LLVMTypeRef pointee = LLVMGetElementType(basePtrTy);
                if (pointee && LLVMGetTypeKind(pointee) == LLVMArrayTypeKind) {
                    LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
                    LLVMValueRef idxs[2] = { zero, zero };
                    return LLVMBuildGEP2(ctx->builder, pointee, entry->value, idxs, 2, "array.decay");
                }
            }
            return entry->value;
        }
        if (entry->addressOnly) {
            return entry->value;
        }

        LLVMTypeRef loadType = entry->type;
        if (!loadType || LLVMGetTypeKind(loadType) == LLVMVoidTypeKind) {
            loadType = cg_dereference_ptr_type(ctx, LLVMTypeOf(entry->value), "identifier load");
        }
        if (!loadType || LLVMGetTypeKind(loadType) == LLVMVoidTypeKind) {
            loadType = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        return cg_build_load(ctx, loadType, entry->value, node->valueNode.value, entry->parsedType);
    }

    const SemanticModel* model = cg_context_get_semantic_model(ctx);
    if (model) {
        const Symbol* sym = semanticModelLookupGlobal(model, node->valueNode.value);
        if (sym &&
            sym->kind == SYMBOL_VARIABLE &&
            sym->hasConstValue &&
            cg_is_builtin_bool_literal_name(sym->name)) {
            LLVMTypeRef boolConstType = cg_type_from_parsed(ctx, &sym->type);
            if (!boolConstType || LLVMGetTypeKind(boolConstType) != LLVMIntegerTypeKind) {
                boolConstType = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            return LLVMConstInt(boolConstType, (unsigned long long)sym->constValue, 0);
        }
        if (sym && sym->kind == SYMBOL_ENUM && sym->hasConstValue) {
            LLVMTypeRef enumType = cg_type_from_parsed(ctx, &sym->type);
            if (!enumType || LLVMGetTypeKind(enumType) != LLVMIntegerTypeKind) {
                enumType = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            return LLVMConstInt(enumType, (unsigned long long)sym->constValue, 1);
        }
    }

    LLVMValueRef global = LLVMGetNamedGlobal(ctx->module, node->valueNode.value);
    if (!global) {
        LLVMValueRef fn = LLVMGetNamedFunction(ctx->module, node->valueNode.value);
        if (fn) {
            return fn;
        }
        fprintf(stderr, "Error: Undefined variable %s\n", node->valueNode.value);
        return NULL;
    }

    LLVMTypeRef loadType = cg_dereference_ptr_type(ctx, LLVMTypeOf(global), "global load");
    if (!loadType || LLVMGetTypeKind(loadType) == LLVMVoidTypeKind) {
        loadType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    const ParsedType* parsedType = NULL;
    if (model) {
        const Symbol* sym = semanticModelLookupGlobal(model, node->valueNode.value);
        if (sym) {
            parsedType = &sym->type;
        }
    }
    cg_scope_insert(ctx->currentScope,
                    node->valueNode.value,
                    global,
                    loadType,
                    true,
                    false,
                    NULL,
                    parsedType);
    return cg_build_load(ctx, loadType, global, node->valueNode.value, parsedType);
}


LLVMValueRef codegenSizeof(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_SIZEOF) {
        fprintf(stderr, "Error: Invalid node type for codegenSizeof\n");
        return NULL;
    }

    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
    const ParsedType* parsed = NULL;
    if (node->expr.left) {
        ASTNode* operand = node->expr.left;
        if (operand->type == AST_STRING_LITERAL) {
            const char* payload = NULL;
            LiteralEncoding enc = ast_literal_encoding(operand->valueNode.value, &payload);
            const TargetLayout* tl = cg_context_get_target_layout(ctx);
            uint64_t elemSize = 1;
            unsigned charBits = (unsigned)((tl && tl->charBits) ? tl->charBits : 8);
            if (enc == LIT_ENC_WIDE) {
                unsigned wcharBits = charBits;
                const SemanticModel* model = cg_context_get_semantic_model(ctx);
                const Symbol* sym = model ? semanticModelLookupGlobal(model, "wchar_t") : NULL;
                if (sym && sym->kind == SYMBOL_TYPEDEF) {
                    LLVMTypeRef wcharTy = cg_type_from_parsed(ctx, &sym->type);
                    uint64_t sz = 0;
                    if (cg_size_align_for_type(ctx, &sym->type, wcharTy, &sz, NULL) && sz > 0) {
                        elemSize = sz;
                        wcharBits = (unsigned)(elemSize * 8);
                    }
                } else {
                    elemSize = (uint64_t)((charBits + 7) / 8);
                    wcharBits = (unsigned)(elemSize * 8);
                }

                LiteralDecodeResult res = decode_c_string_literal_wide(payload ? payload : "",
                                                                       (int)wcharBits,
                                                                       NULL,
                                                                       NULL);
                if (!res.ok) return NULL;
                return LLVMConstInt(intptrTy, (res.length + 1) * elemSize, 0);
            }

            LiteralDecodeResult res = decode_c_string_literal(payload ? payload : "",
                                                              (int)charBits,
                                                              NULL,
                                                              NULL);
            if (!res.ok) return NULL;
            elemSize = (uint64_t)((charBits + 7) / 8);
            if (!elemSize) elemSize = 1;
            return LLVMConstInt(intptrTy, (res.length + 1) * elemSize, 0);
        }
        if (operand->type == AST_PARSED_TYPE) {
            parsed = &operand->parsedTypeNode.parsed;
        } else {
            parsed = cg_resolve_expression_type(ctx, operand);
        }
    }

    if (parsed) {
        if (parsedTypeHasVLA(parsed)) {
            LLVMValueRef elementCount = NULL;
            if (node->expr.left && node->expr.left->type == AST_IDENTIFIER && ctx->currentScope) {
                NamedValue* named = cg_scope_lookup(ctx->currentScope, node->expr.left->valueNode.value);
                if (named && named->vlaElementCount) {
                    elementCount = named->vlaElementCount;
                }
            }
            if (!elementCount) {
                elementCount = computeVLAElementCount(ctx, parsed);
            }
            if (!elementCount) return NULL;
            elementCount = ensureIntegerLike(ctx, elementCount);
            if (!elementCount) return NULL;
            LLVMTypeRef elemType = vlaInnermostElementLLVM(ctx, parsed);
            unsigned long long elemSize = LLVMABISizeOfType(LLVMGetModuleDataLayout(ctx->module), elemType);
            LLVMValueRef elemSizeVal = LLVMConstInt(intptrTy, elemSize, 0);
            return LLVMBuildMul(ctx->builder, elementCount, elemSizeVal, "sizeof.vla");
        }

        LLVMTypeRef ty = cg_type_from_parsed(ctx, parsed);
        if (!ty || LLVMGetTypeKind(ty) == LLVMVoidTypeKind) {
            ty = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        uint64_t semanticSize = 0;
        if (cg_size_align_for_type(ctx, parsed, ty, &semanticSize, NULL) && semanticSize > 0) {
            return LLVMConstInt(intptrTy, semanticSize, 0);
        }
        unsigned long long abiSize = LLVMABISizeOfType(LLVMGetModuleDataLayout(ctx->module), ty);
        return LLVMConstInt(intptrTy, abiSize, 0);
    }

    LLVMTypeRef type = LLVMInt32TypeInContext(ctx->llvmContext);
    if (node->expr.left) {
        ASTNode* operand = node->expr.left;
        LLVMValueRef value = codegenNode(ctx, operand);
        if (value) {
            type = LLVMTypeOf(value);
        }
    }

    return LLVMSizeOf(type);
}

LLVMValueRef codegenAlignof(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_ALIGNOF) {
        fprintf(stderr, "Error: Invalid node type for codegenAlignof\n");
        return NULL;
    }
    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
    const ParsedType* parsed = NULL;
    if (node->expr.left) {
        ASTNode* operand = node->expr.left;
        if (operand->type == AST_PARSED_TYPE) {
            parsed = &operand->parsedTypeNode.parsed;
        } else {
            parsed = cg_resolve_expression_type(ctx, operand);
        }
    }
    if (parsed) {
        LLVMTypeRef ty = NULL;
        if (parsedTypeHasVLA(parsed)) {
            ty = vlaInnermostElementLLVM(ctx, parsed);
        } else {
            ty = cg_type_from_parsed(ctx, parsed);
        }
        if (!ty || LLVMGetTypeKind(ty) == LLVMVoidTypeKind) {
            ty = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        unsigned align = LLVMABIAlignmentOfType(LLVMGetModuleDataLayout(ctx->module), ty);
        return LLVMConstInt(intptrTy, align, 0);
    }
    LLVMTypeRef type = LLVMInt32TypeInContext(ctx->llvmContext);
    if (node->expr.left) {
        LLVMValueRef value = codegenNode(ctx, node->expr.left);
        if (value) type = LLVMTypeOf(value);
    }
    unsigned align = LLVMABIAlignmentOfType(LLVMGetModuleDataLayout(ctx->module), type);
    return LLVMConstInt(intptrTy, align, 0);
}


LLVMValueRef codegenHeapAllocation(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_HEAP_ALLOCATION) {
        fprintf(stderr, "Error: Invalid node type for codegenHeapAllocation\n");
        return NULL;
    }

    LLVMTypeRef allocType = LLVMStructType(NULL, 0, 0);
    LLVMValueRef size = LLVMSizeOf(allocType);
    LLVMValueRef mallocFunc = LLVMGetNamedFunction(ctx->module, "malloc");

    if (!mallocFunc) {
        fprintf(stderr, "Error: malloc function not found\n");
        return NULL;
    }

    LLVMValueRef args[] = {size};
    return LLVMBuildCall2(ctx->builder, LLVMTypeOf(mallocFunc), mallocFunc, args, 1, "mallocCall");
}
