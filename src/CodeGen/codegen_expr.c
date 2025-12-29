#include "codegen_private.h"

#include "codegen_types.h"
#include "Parser/Helpers/parsed_type.h"
#include "literal_utils.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

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
    if (!type->isFunctionPointer && type->fpParamCount == 0) {
        return NULL;
    }

    LLVMTypeRef returnType = NULL;
    switch (type->primitiveType) {
        case TOKEN_INT:    returnType = LLVMInt32TypeInContext(ctx->llvmContext); break;
        case TOKEN_CHAR:   returnType = LLVMInt8TypeInContext(ctx->llvmContext);  break;
        case TOKEN_BOOL:   returnType = LLVMInt1TypeInContext(ctx->llvmContext);  break;
        case TOKEN_VOID:   returnType = LLVMVoidTypeInContext(ctx->llvmContext);  break;
        default: break;
    }
    if (!returnType) {
        ParsedType retClone = parsedTypeClone(type);
        retClone.isFunctionPointer = false;
        retClone.fpParamCount = 0;
        retClone.fpParams = NULL;
        retClone.derivationCount = 0;
        retClone.pointerDepth = 0;

        returnType = cg_type_from_parsed(ctx, &retClone);
        parsedTypeFree(&retClone);
    }
    if (!returnType || LLVMGetTypeKind(returnType) == LLVMVoidTypeKind ||
        LLVMGetTypeKind(returnType) == LLVMPointerTypeKind) {
        returnType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    size_t count = type->fpParamCount ? type->fpParamCount : fallbackArgCount;
    bool isVariadic = type->isVariadicFunction;
    LLVMTypeRef* params = NULL;
    if (count > 0) {
        params = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * count);
        if (!params) return NULL;
        for (size_t i = 0; i < count; ++i) {
            if (type->fpParamCount > 0 && type->fpParams) {
                params[i] = cg_type_from_parsed(ctx, &type->fpParams[i]);
            } else if (args && i < fallbackArgCount) {
                params[i] = args[i] ? LLVMTypeOf(args[i]) : NULL;
            }
            if (!params[i]) {
                params[i] = LLVMInt32TypeInContext(ctx->llvmContext);
            }
        }
    }

    // If the type carries derivations, prefer the function derivation's variadic flag.
    for (size_t i = 0; i < type->derivationCount; ++i) {
        const TypeDerivation* deriv = parsedTypeGetDerivation(type, i);
        if (deriv && deriv->kind == TYPE_DERIVATION_FUNCTION) {
            isVariadic = deriv->as.function.isVariadic;
            break;
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
    bool lhsIsPointer = LLVMGetTypeKind(lhsType) == LLVMPointerTypeKind;
    bool rhsIsPointer = LLVMGetTypeKind(rhsType) == LLVMPointerTypeKind;
    bool preferUnsigned = cg_expression_is_unsigned(ctx, node->expr.left) ||
                          cg_expression_is_unsigned(ctx, node->expr.right);

    if (strcmp(op, "+") == 0) {
        if (lhsIsPointer && rhsIsPointer) {
            fprintf(stderr, "Error: cannot add two pointer values\n");
            return NULL;
        }
        if (lhsIsPointer) {
            return cg_build_pointer_offset(ctx, L, R, lhsParsed, rhsParsed, false);
        }
        if (rhsIsPointer) {
            return cg_build_pointer_offset(ctx, R, L, rhsParsed, lhsParsed, false);
        }
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "add.cast");
        }
        return LLVMBuildAdd(ctx->builder, L, R, "addtmp");
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
        return LLVMBuildSub(ctx->builder, L, R, "subtmp");
    } else if (strcmp(op, "*") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "mul.cast");
        }
        return LLVMBuildMul(ctx->builder, L, R, "multmp");
    } else if (strcmp(op, "/") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "div.cast");
        }
        if (preferUnsigned) {
            return LLVMBuildUDiv(ctx->builder, L, R, "divtmp");
        }
        return LLVMBuildSDiv(ctx->builder, L, R, "divtmp");
    } else if (strcmp(op, "%") == 0) {
        if (lhsType != rhsType) {
            R = cg_cast_value(ctx, R, lhsType, rhsParsed, lhsParsed, "rem.cast");
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
            current = LLVMBuildLoad2(ctx->builder, targetType, targetPtr, "unary.load");
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
            LLVMBuildStore(ctx->builder, updated, targetPtr);
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
            LLVMBuildStore(ctx->builder, updated, targetPtr);
        }
        return isPostfix ? current : updated;
    }

    if (node->expr.op && strcmp(node->expr.op, "&") == 0) {
        LLVMValueRef addrPtr = NULL;
        LLVMTypeRef addrType = NULL;
        CGLValueInfo tmp;
        if (!codegenLValue(ctx, node->expr.left, &addrPtr, &addrType, NULL, &tmp)) {
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
    LLVMValueRef operand = codegenNode(ctx, node->expr.left);
    if (!operand) {
        fprintf(stderr, "Error: Failed to generate operand for unary expression\n");
        return NULL;
    }

    if (strcmp(node->expr.op, "-") == 0) {
        return LLVMBuildNeg(ctx->builder, operand, "negtmp");
    } else if (strcmp(node->expr.op, "~") == 0) {
        LLVMTypeRef operandType = LLVMTypeOf(operand);
        if (!operandType || LLVMGetTypeKind(operandType) != LLVMIntegerTypeKind) {
            LLVMTypeRef intType = LLVMInt32TypeInContext(ctx->llvmContext);
            operand = cg_cast_value(ctx, operand, intType, operandParsed, NULL, "bnot.cast");
            if (!operand) {
                fprintf(stderr, "Error: Failed to cast operand for bitwise not\n");
                return NULL;
            }
        }
        return LLVMBuildNot(ctx->builder, operand, "nottmp");
    } else if (strcmp(node->expr.op, "!") == 0) {
        LLVMValueRef boolVal = cg_build_truthy(ctx, operand, operandParsed, "lnot.bool");
        if (!boolVal) return NULL;
        LLVMValueRef inverted = LLVMBuildNot(ctx->builder, boolVal, "lnot.tmp");
        return cg_widen_bool_to_int(ctx, inverted, "lnot.int");
    } else if (strcmp(node->expr.op, "*") == 0) {
        LLVMTypeRef ptrType = LLVMTypeOf(operand);
        if (!ptrType || LLVMGetTypeKind(ptrType) != LLVMPointerTypeKind) {
            fprintf(stderr, "Error: Cannot dereference non-pointer value\n");
            return NULL;
        }
        LLVMTypeRef elemType = cg_element_type_from_pointer(ctx, operandParsed, ptrType);
        if (!elemType || LLVMGetTypeKind(elemType) == LLVMVoidTypeKind) {
            elemType = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        return LLVMBuildLoad2(ctx->builder, elemType, operand, "loadtmp");
    }

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
        return targetPtr;
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
            current = LLVMBuildLoad2(ctx->builder, storeType, targetPtr, "compound.load");
        }
        if (!current) {
            fprintf(stderr, "Error: Failed to load target for compound assignment\n");
            return NULL;
        }

        bool targetIsPointer = LLVMGetTypeKind(storeType) == LLVMPointerTypeKind;
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
            storedValue = LLVMBuildAdd(ctx->builder, current, value, "compound.add");
        } else if (strcmp(op, "-=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = LLVMBuildSub(ctx->builder, current, value, "compound.sub");
        } else if (strcmp(op, "*=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            storedValue = LLVMBuildMul(ctx->builder, current, value, "compound.mul");
        } else if (strcmp(op, "/=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            if (cg_expression_is_unsigned(ctx, node->assignment.target)) {
                storedValue = LLVMBuildUDiv(ctx->builder, current, value, "compound.div");
            } else {
                storedValue = LLVMBuildSDiv(ctx->builder, current, value, "compound.div");
            }
        } else if (strcmp(op, "%=") == 0) {
            if (LLVMTypeOf(current) != LLVMTypeOf(value)) {
                value = cg_cast_value(ctx, value, LLVMTypeOf(current), valueParsed, targetParsed, "compound.cast");
            }
            if (cg_expression_is_unsigned(ctx, node->assignment.target)) {
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
    return LLVMBuildStore(ctx->builder, storedValue, targetPtr);
}


LLVMValueRef codegenArrayAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_ARRAY_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenArrayAccess\n");
        return NULL;
    }

    LLVMValueRef arrayPtr = codegenNode(ctx, node->arrayAccess.array);
    LLVMValueRef index = codegenNode(ctx, node->arrayAccess.index);
    if (!arrayPtr || !index) {
        fprintf(stderr, "Error: Array access failed\n");
        return NULL;
    }
    LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
    const ParsedType* arrayParsed = cg_resolve_expression_type(ctx, node->arrayAccess.array);

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
        LLVMTypeRef ptrToElem = LLVMPointerType(elemType, 0);
        if (LLVMTypeOf(arrayPtr) != ptrToElem) {
            arrayPtr = LLVMBuildBitCast(ctx->builder, arrayPtr, ptrToElem, "vla.elem.base");
        }
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

    LLVMTypeRef elementType = derivedElementType;
    if (!elementType) {
        elementType = cg_dereference_ptr_type(ctx, ptrToElemType, "array access");
    }
    LLVMValueRef typedElementPtr = elementPtr;
    LLVMTypeRef loadTy = elementType ? elementType : LLVMInt8TypeInContext(ctx->llvmContext);
    if (!typedElementPtr || !LLVMTypeOf(typedElementPtr) ||
        LLVMGetTypeKind(LLVMTypeOf(typedElementPtr)) != LLVMPointerTypeKind) {
        fprintf(stderr, "Error: invalid pointer for array load\n");
        return NULL;
    }
    LLVMTypeRef desiredPtrTy = LLVMPointerType(loadTy, 0);
    if (LLVMTypeOf(typedElementPtr) != desiredPtrTy) {
        typedElementPtr = LLVMBuildBitCast(ctx->builder, typedElementPtr, desiredPtrTy, "array.elem.typed");
    }
    LLVMValueRef loadVal = LLVMBuildLoad2(ctx->builder, loadTy, typedElementPtr, "arrayLoad");
    CG_DEBUG("[CG] Array element load complete\n");
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
    LLVMTypeRef elemType = cg_dereference_ptr_type(ctx, LLVMTypeOf(pointer), "pointer dereference");
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
    LLVMBuildBr(ctx->builder, mergeBB);

    LLVMPositionBuilderAtEnd(ctx->builder, mergeBB);
    LLVMValueRef phi = LLVMBuildPhi(ctx->builder, LLVMInt1TypeInContext(ctx->llvmContext),
                                    isAnd ? "land.result" : "lor.result");
    LLVMValueRef incomingVals[2];
    LLVMBasicBlockRef incomingBlocks[2];

    if (isAnd) {
        incomingVals[0] = LLVMConstInt(LLVMInt1TypeInContext(ctx->llvmContext), 0, 0);
        incomingBlocks[0] = lhsBlock;
        incomingVals[1] = rhsBool;
        incomingBlocks[1] = rhsEvalBB;
    } else {
        incomingVals[0] = LLVMConstInt(LLVMInt1TypeInContext(ctx->llvmContext), 1, 0);
        incomingBlocks[0] = lhsBlock;
        incomingVals[1] = rhsBool;
        incomingBlocks[1] = rhsEvalBB;
    }
    LLVMAddIncoming(phi, incomingVals, incomingBlocks, 2);
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

    LLVMTypeRef calleeType = NULL;
    if (calleeParsed) {
        calleeType = functionTypeFromPointerParsed(ctx, calleeParsed, node->functionCall.argumentCount, args);
    }
    if (!calleeType) {
        calleeType = LLVMTypeOf(function);
        if (calleeType && LLVMGetTypeKind(calleeType) == LLVMPointerTypeKind) {
            LLVMTypeRef element = cg_dereference_ptr_type(ctx, calleeType, "call target");
            if (element && LLVMGetTypeKind(element) == LLVMFunctionTypeKind) {
                calleeType = element;
            }
        }
    }
    if ((!calleeType || LLVMGetTypeKind(calleeType) != LLVMFunctionTypeKind) && calleeName && ctx) {
        const SemanticModel* model = cg_context_get_semantic_model(ctx);
        if (model) {
            const Symbol* sym = semanticModelLookupGlobal(model, calleeName);
            if (sym && sym->signature.paramCount >= 0) {
                LLVMTypeRef retType = cg_type_from_parsed(ctx, &sym->type);
                if (!retType || LLVMGetTypeKind(retType) == LLVMVoidTypeKind) {
                    retType = LLVMVoidTypeInContext(ctx->llvmContext);
                }
                size_t paramCount = sym->signature.paramCount;
                LLVMTypeRef* paramTypes = NULL;
                if (paramCount > 0) {
                    paramTypes = (LLVMTypeRef*)calloc(paramCount, sizeof(LLVMTypeRef));
                    if (!paramTypes) return NULL;
                    for (size_t i = 0; i < paramCount; ++i) {
                        paramTypes[i] = cg_type_from_parsed(ctx, &sym->signature.params[i]);
                        if (!paramTypes[i] || LLVMGetTypeKind(paramTypes[i]) == LLVMVoidTypeKind) {
                            paramTypes[i] = LLVMInt32TypeInContext(ctx->llvmContext);
                        }
                    }
                }
                calleeType = LLVMFunctionType(retType, paramTypes, (unsigned)paramCount, sym->signature.isVariadic);
                free(paramTypes);
            }
        }
    }
    if (!calleeType || LLVMGetTypeKind(calleeType) != LLVMFunctionTypeKind) {
        fprintf(stderr, "Error: call target is not a function type\n");
        free(args);
        return NULL;
    }

    char* resolvedFnType = LLVMPrintTypeToString(calleeType);
    CG_DEBUG("[CG] Function call resolved type: %s\n", resolvedFnType ? resolvedFnType : "<null>");
    if (resolvedFnType) LLVMDisposeMessage(resolvedFnType);

    size_t argCount = node->functionCall.argumentCount;
    LLVMValueRef* finalArgs = args;
    LLVMValueRef* promotedArgs = NULL;
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

    if (LLVMGetTypeKind(target) == LLVMPointerTypeKind && LLVMGetTypeKind(valueType) == LLVMPointerTypeKind) {
        return LLVMBuildBitCast(ctx->builder, value, target, "ptrcast");
    }

    if (LLVMGetTypeKind(target) == LLVMIntegerTypeKind && LLVMGetTypeKind(valueType) == LLVMIntegerTypeKind) {
        unsigned valueBits = LLVMGetIntTypeWidth(valueType);
        unsigned targetBits = LLVMGetIntTypeWidth(target);
        if (valueBits == targetBits) {
            return LLVMBuildBitCast(ctx->builder, value, target, "intcast");
        } else if (valueBits < targetBits) {
            return LLVMBuildSExt(ctx->builder, value, target, "extcast");
        } else {
            return LLVMBuildTrunc(ctx->builder, value, target, "truncast");
        }
    }

    /* Fallback: rely on bitcast even if the types differ (TODO: semantic-driven casts). */
    return LLVMBuildBitCast(ctx->builder, value, target, "casttmp");
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

    int value = atoi(node->valueNode.value);
    return LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), value, 0);
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


LLVMValueRef codegenStringLiteral(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_STRING_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenStringLiteral\n");
        return NULL;
    }

    const char* payload = NULL;
    LiteralEncoding enc = ast_literal_encoding(node->valueNode.value, &payload);
    if (enc == LIT_ENC_WIDE) {
        const char* text = payload ? payload : node->valueNode.value;
        return LLVMBuildGlobalStringPtr(ctx->builder, text ? text : "", "str");
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
        LLVMValueRef loaded = LLVMBuildLoad2(ctx->builder, loadType, entry->value, node->valueNode.value);
        return loaded;
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
    const SemanticModel* model = cg_context_get_semantic_model(ctx);
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
    return LLVMBuildLoad2(ctx->builder, loadType, global, node->valueNode.value);
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
