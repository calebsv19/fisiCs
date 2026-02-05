#include "codegen_private.h"

#include "codegen_types.h"
#include "Parser/Helpers/parsed_type.h"
#include "literal_utils.h"
#include "Syntax/type_checker.h"
#include "Syntax/layout.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

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
    TypeInfo info = typeInfoFromParsedType(type, scope);
    if (info.category != TYPEINFO_STRUCT && info.category != TYPEINFO_UNION) return false;
    CCTagKind kind = (info.category == TYPEINFO_STRUCT) ? CC_TAG_STRUCT : CC_TAG_UNION;
    const CCTagFieldLayout* layouts = NULL;
    size_t count = 0;
    if (!cc_get_tag_field_layouts(cctx, kind, info.userTypeName, &layouts, &count) || !layouts) {
        size_t sz = 0, al = 0;
        if (!layout_struct_union(cctx, scope, kind, info.userTypeName, &sz, &al)) return false;
        if (!cc_get_tag_field_layouts(cctx, kind, info.userTypeName, &layouts, &count) || !layouts) {
            return false;
        }
    }
    for (size_t i = 0; i < count; ++i) {
        const CCTagFieldLayout* lay = &layouts[i];
        if (!lay->name) continue;
        if (strcmp(lay->name, fieldName) == 0) {
            if (lay->isBitfield && !lay->isZeroWidth) return false;
            *offsetOut = lay->byteOffset;
            return true;
        }
    }
    return false;
}

static bool cg_parsed_type_is_complex_value(const ParsedType* t) {
    return t && (t->isComplex || t->isImaginary);
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
        *lhsValue = LLVMBuildIntCast2(ctx->builder, *lhsValue, targetType, lhsUnsigned, "int.promote.l");
        *lhsType = targetType;
    }
    if (rhsBits != targetBits) {
        *rhsValue = LLVMBuildIntCast2(ctx->builder, *rhsValue, targetType, rhsUnsigned, "int.promote.r");
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
    if (ty && LLVMGetTypeKind(ty) == LLVMIntegerTypeKind) {
        return value;
    }
    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
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

static LLVMTypeRef functionTypeFromPointerParsed(CodegenContext* ctx,
                                                 const ParsedType* type,
                                                 size_t fallbackArgCount,
                                                 LLVMValueRef* args) {
    if (!ctx || !type) {
        return NULL;
    }
    const ParsedType* resolved = type;
    if (type->kind == TYPE_NAMED && ctx->typeCache && type->userTypeName) {
        CGNamedLLVMType* info = cg_type_cache_get_typedef_info(ctx->typeCache, type->userTypeName);
        if (info && info->parsedType.kind != TYPE_INVALID) {
            resolved = &info->parsedType;
        }
    }

    const ParsedType* paramList = NULL;
    size_t paramCount = 0;
    bool isVariadic = resolved->isVariadicFunction;

    if (resolved->isFunctionPointer || resolved->fpParamCount > 0) {
        paramList = resolved->fpParams;
        paramCount = resolved->fpParamCount;
    } else {
        for (size_t i = 0; i < resolved->derivationCount; ++i) {
            const TypeDerivation* deriv = parsedTypeGetDerivation(resolved, i);
            if (deriv && deriv->kind == TYPE_DERIVATION_FUNCTION) {
                paramList = deriv->as.function.params;
                paramCount = deriv->as.function.paramCount;
                isVariadic = deriv->as.function.isVariadic;
                break;
            }
        }
        if (!paramList && paramCount == 0) {
            return NULL;
        }
    }

    LLVMTypeRef returnType = NULL;
    size_t funcIndex = (size_t)-1;
    if (resolved) {
        for (size_t i = 0; i < resolved->derivationCount; ++i) {
            const TypeDerivation* deriv = parsedTypeGetDerivation(resolved, i);
            if (deriv && deriv->kind == TYPE_DERIVATION_FUNCTION) {
                funcIndex = i;
                break;
            }
        }
    }
    if (funcIndex != (size_t)-1) {
        ParsedType retParsed = parsedTypeClone(resolved);
        if (funcIndex + 1 < retParsed.derivationCount) {
            memmove(retParsed.derivations,
                    retParsed.derivations + funcIndex + 1,
                    (retParsed.derivationCount - funcIndex - 1) * sizeof(TypeDerivation));
        }
        retParsed.derivationCount -= (funcIndex + 1);
        if (retParsed.derivationCount == 0 && retParsed.derivations) {
            free(retParsed.derivations);
            retParsed.derivations = NULL;
        }
        retParsed.pointerDepth = 0;
        retParsed.isFunctionPointer = false;
        retParsed.fpParamCount = 0;
        retParsed.fpParams = NULL;
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
                params[i] = cg_type_from_parsed(ctx, &paramList[i]);
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
    LLVMTypeRef lhsType = LLVMTypeOf(L);
    LLVMTypeRef rhsType = LLVMTypeOf(R);
    bool lhsIsPointer = lhsType && LLVMGetTypeKind(lhsType) == LLVMPointerTypeKind;
    bool rhsIsPointer = rhsType && LLVMGetTypeKind(rhsType) == LLVMPointerTypeKind;
    bool lhsFloat = cg_is_float_type(lhsType);
    bool rhsFloat = cg_is_float_type(rhsType);
    bool lhsUnsigned = cg_is_unsigned_parsed(lhsParsed, lhsType);
    bool rhsUnsigned = cg_is_unsigned_parsed(rhsParsed, rhsType);
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
            return cg_build_pointer_difference(ctx, L, R, lhsParsed ? lhsParsed : rhsParsed);
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
            else if (strcmp(op, "!=") == 0) pred = LLVMRealONE;
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
    LLVMBuildBr(ctx->builder, mergeBB);

    LLVMPositionBuilderAtEnd(ctx->builder, falseBB);
    LLVMValueRef falseValue = codegenNode(ctx, node->ternaryExpr.falseExpr);
    const ParsedType* falseParsed = cg_resolve_expression_type(ctx, node->ternaryExpr.falseExpr);
    LLVMBuildBr(ctx->builder, mergeBB);

    LLVMPositionBuilderAtEnd(ctx->builder, mergeBB);
    // If both arms are void, just merge control flow.
    if (!trueValue && !falseValue) {
        return NULL;
    }

    LLVMTypeRef mergedType = NULL;
    if (trueValue && falseValue) {
        mergedType = cg_merge_types_for_phi(ctx, trueParsed, falseParsed, trueValue, falseValue);
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
        incomingBlocks[count] = trueBB;
        count++;
    }
    if (falseValue) {
        incomingVals[count] = falseValue;
        incomingBlocks[count] = falseBB;
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
        fprintf(stderr, "Error: Unsupported assignment target type %d\n", target ? target->type : -1);
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

    bool destIsAggregate = targetType && (LLVMGetTypeKind(targetType) == LLVMStructTypeKind ||
                                          LLVMGetTypeKind(targetType) == LLVMArrayTypeKind);
    if (destIsAggregate) {
        if (!codegenLValue(ctx, node->assignment.value, &srcPtr, &srcType, &srcParsed, NULL)) {
            LLVMValueRef tmp = LLVMBuildAlloca(ctx->builder, targetType, "agg.tmp");
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

    const char* op = node->assignment.op ? node->assignment.op : "=";
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
            LLVMValueRef tmp = LLVMBuildAlloca(ctx->builder, storeType, "flex.tmp");
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
    if (haveLVal && arrayPtr && (!arrayParsed || !parsedTypeIsDirectArray(arrayParsed))) {
        LLVMTypeRef loadTy = arrayType;
        if (!loadTy) {
            LLVMTypeRef elem = NULL;
            if (arrayPtr && LLVMGetTypeKind(LLVMTypeOf(arrayPtr)) == LLVMPointerTypeKind) {
                elem = LLVMGetElementType(LLVMTypeOf(arrayPtr));
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
    if (!typedElementPtr || !LLVMTypeOf(typedElementPtr) ||
        LLVMGetTypeKind(LLVMTypeOf(typedElementPtr)) != LLVMPointerTypeKind) {
        fprintf(stderr, "Error: invalid pointer for array load\n");
        return NULL;
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
        fprintf(stderr, "Error: Unknown field in pointer access\n");
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
        fprintf(stderr, "Error: Unknown field in dot access\n");
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
            return LLVMBuildIntCast2(ctx->builder, arg, dest, isUnsigned, "vararg.int.promote");
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
    return LLVMBuildCall2(ctx->builder, fnTy, fn, &casted, 1, intrinsicName);
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

    LLVMValueRef function = codegenNode(ctx, node->functionCall.callee);
    if (!function) {
        fprintf(stderr, "Error: Undefined function %s\n",
                calleeName ? calleeName : "<expr>");
        return NULL;
    }

    // Opaque-pointer friendly: prefer semantic signature, only peel one level of pointer-to-function.

    LLVMValueRef* args = NULL;
    if (node->functionCall.argumentCount > 0) {
        args = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * node->functionCall.argumentCount);
        if (!args) return NULL;
        for (size_t i = 0; i < node->functionCall.argumentCount; i++) {
            args[i] = codegenNode(ctx, node->functionCall.arguments[i]);
        }
    }

    if (calleeName &&
        (strcmp(calleeName, "va_start") == 0 || strcmp(calleeName, "__builtin_va_start") == 0)) {
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
    if (calleeName &&
        (strcmp(calleeName, "__builtin_va_arg") == 0 || strcmp(calleeName, "va_arg") == 0)) {
        LLVMValueRef val = NULL;
        if (node->functionCall.argumentCount >= 2 &&
            node->functionCall.arguments[1] &&
            node->functionCall.arguments[1]->type == AST_PARSED_TYPE) {
            LLVMTypeRef resTy = cg_type_from_parsed(ctx, &node->functionCall.arguments[1]->parsedTypeNode.parsed);
            if (resTy && args) {
                val = LLVMBuildVAArg(ctx->builder, args[0], resTy, "vaarg");
            }
        }
        free(args);
        if (val) return val;
        return LLVMConstNull(LLVMInt32TypeInContext(ctx->llvmContext));
    }
    if (calleeName &&
        (strcmp(calleeName, "va_end") == 0 || strcmp(calleeName, "__builtin_va_end") == 0)) {
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

    const Symbol* sym = NULL;
    bool noPrototype = false;
    if (calleeName && ctx) {
        const SemanticModel* model = cg_context_get_semantic_model(ctx);
        if (model) {
            sym = semanticModelLookupGlobal(model, calleeName);
            if (sym && sym->kind == SYMBOL_FUNCTION && !sym->signature.hasPrototype) {
                noPrototype = true;
            }
        }
    }

    LLVMTypeRef calleeType = NULL;
    if (calleeParsed && !noPrototype) {
        calleeType = functionTypeFromPointerParsed(ctx, calleeParsed, node->functionCall.argumentCount, args);
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
    if ((!calleeType || LLVMGetTypeKind(calleeType) != LLVMFunctionTypeKind) && calleeName && ctx && !noPrototype) {
        const SemanticModel* model = cg_context_get_semantic_model(ctx);
        if (model) {
            const Symbol* lookup = semanticModelLookupGlobal(model, calleeName);
            if (lookup) {
                LLVMTypeRef retType = cg_type_from_parsed(ctx, &lookup->type);
                if (!retType || LLVMGetTypeKind(retType) == LLVMVoidTypeKind) {
                    retType = LLVMVoidTypeInContext(ctx->llvmContext);
                }
                size_t paramCount = lookup->signature.paramCount;
                LLVMTypeRef* paramTypes = NULL;
                if (paramCount > 0) {
                    paramTypes = (LLVMTypeRef*)calloc(paramCount, sizeof(LLVMTypeRef));
                    if (!paramTypes) return NULL;
                    for (size_t i = 0; i < paramCount; ++i) {
                        paramTypes[i] = cg_type_from_parsed(ctx, &lookup->signature.params[i]);
                        if (!paramTypes[i] || LLVMGetTypeKind(paramTypes[i]) == LLVMVoidTypeKind) {
                            paramTypes[i] = LLVMInt32TypeInContext(ctx->llvmContext);
                        }
                    }
                }
                calleeType = LLVMFunctionType(retType, paramTypes, (unsigned)paramCount, lookup->signature.isVariadic);
                free(paramTypes);
            }
        }
    }

    size_t argCount = node->functionCall.argumentCount;
    LLVMValueRef* finalArgs = args;
    LLVMValueRef* promotedArgs = NULL;

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
                paramTypes[i] = val ? LLVMTypeOf(val) : LLVMInt32TypeInContext(ctx->llvmContext);
                if (!paramTypes[i]) {
                    paramTypes[i] = LLVMInt32TypeInContext(ctx->llvmContext);
                }
            }
        }
        calleeType = LLVMFunctionType(retType, paramTypes, (unsigned)argCount, 0);
        free(paramTypes);
    }

    if (!calleeType || LLVMGetTypeKind(calleeType) != LLVMFunctionTypeKind) {
        fprintf(stderr, "Error: call target is not a function type\n");
        if (promotedArgs) free(promotedArgs);
        free(args);
        return NULL;
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
            LLVMTypeRef paramTy = cg_type_from_parsed(ctx, &sym->signature.params[i]);
            if (!paramTy || LLVMGetTypeKind(paramTy) == LLVMVoidTypeKind) {
                continue;
            }
            const ParsedType* fromParsed = cg_resolve_expression_type(ctx, node->functionCall.arguments[i]);
            args[i] = cg_cast_value(ctx,
                                    args[i],
                                    paramTy,
                                    fromParsed,
                                    &sym->signature.params[i],
                                    "call.arg.cast");
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

    LLVMValueRef call = LLVMBuildCall2(ctx->builder,
                                       calleeType,
                                       function,
                                       finalArgs,
                                       argCount,
                                       "calltmp");
    /* Mirror the callee's calling convention onto the call instruction so
     * LLVM lowers it consistently with the function declaration. */
    unsigned fnCC = LLVMGetFunctionCallConv(function);
    if (fnCC != 0) {
        LLVMSetInstructionCallConv(call, fnCC);
    }
    if (promotedArgs) free(promotedArgs);
    free(args);
    return call;
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

    LLVMValueRef tmp = LLVMBuildAlloca(ctx->builder, literalType, "compound.tmp");
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

    NamedValue* entry = cg_scope_lookup(ctx->currentScope, node->valueNode.value);
    if (entry) {
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
            LLVMValueRef elementCount = computeVLAElementCount(ctx, parsed);
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
