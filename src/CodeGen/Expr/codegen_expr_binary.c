// SPDX-License-Identifier: Apache-2.0

#include "codegen_expr_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
