// SPDX-License-Identifier: Apache-2.0

#include "codegen_expr_internal.h"

#include <stdio.h>

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
    bool trueFallsThrough = (LLVMGetBasicBlockTerminator(trueEndBB) == NULL);

    LLVMPositionBuilderAtEnd(ctx->builder, falseBB);
    LLVMValueRef falseValue = codegenNode(ctx, node->ternaryExpr.falseExpr);
    const ParsedType* falseParsed = cg_resolve_expression_type(ctx, node->ternaryExpr.falseExpr);
    LLVMBasicBlockRef falseEndBB = LLVMGetInsertBlock(ctx->builder);
    bool falseFallsThrough = (LLVMGetBasicBlockTerminator(falseEndBB) == NULL);

    LLVMValueRef phiTrueValue = trueFallsThrough ? trueValue : NULL;
    LLVMValueRef phiFalseValue = falseFallsThrough ? falseValue : NULL;

    if (!trueFallsThrough && !falseFallsThrough) {
        return NULL;
    }

    // If both arms are void, only merge control flow.
    if (!phiTrueValue && !phiFalseValue) {
        if (trueFallsThrough) {
            LLVMPositionBuilderAtEnd(ctx->builder, trueEndBB);
            if (!LLVMGetBasicBlockTerminator(trueEndBB)) {
                LLVMBuildBr(ctx->builder, mergeBB);
            }
        }
        if (falseFallsThrough) {
            LLVMPositionBuilderAtEnd(ctx->builder, falseEndBB);
            if (!LLVMGetBasicBlockTerminator(falseEndBB)) {
                LLVMBuildBr(ctx->builder, mergeBB);
            }
        }
        LLVMPositionBuilderAtEnd(ctx->builder, mergeBB);
        return NULL;
    }

    LLVMTypeRef mergedType = NULL;
    if (phiTrueValue && phiFalseValue) {
        LLVMTypeRef trueTy = LLVMTypeOf(phiTrueValue);
        LLVMTypeRef falseTy = LLVMTypeOf(phiFalseValue);
        LLVMTypeKind trueKind = trueTy ? LLVMGetTypeKind(trueTy) : LLVMVoidTypeKind;
        LLVMTypeKind falseKind = falseTy ? LLVMGetTypeKind(falseTy) : LLVMVoidTypeKind;
        if (trueKind == LLVMPointerTypeKind && falseKind == LLVMPointerTypeKind) {
            mergedType = trueTy;
        } else if (trueKind == LLVMPointerTypeKind &&
                   falseKind == LLVMIntegerTypeKind &&
                   LLVMIsAConstantInt(phiFalseValue) &&
                   LLVMConstIntGetSExtValue(phiFalseValue) == 0) {
            mergedType = trueTy;
        } else if (falseKind == LLVMPointerTypeKind &&
                   trueKind == LLVMIntegerTypeKind &&
                   LLVMIsAConstantInt(phiTrueValue) &&
                   LLVMConstIntGetSExtValue(phiTrueValue) == 0) {
            mergedType = falseTy;
        }
        if (!mergedType) {
            mergedType = cg_merge_types_for_phi(ctx, trueParsed, falseParsed, phiTrueValue, phiFalseValue);
        }
    } else if (phiTrueValue) {
        mergedType = LLVMTypeOf(phiTrueValue);
    } else {
        mergedType = LLVMTypeOf(phiFalseValue);
    }
    if (!mergedType) {
        mergedType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    if (trueFallsThrough) {
        LLVMPositionBuilderAtEnd(ctx->builder, trueEndBB);
        if (phiTrueValue && LLVMTypeOf(phiTrueValue) != mergedType) {
            phiTrueValue = cg_cast_value(ctx, phiTrueValue, mergedType, trueParsed, falseParsed, "ternary.true.cast");
        }
        trueEndBB = LLVMGetInsertBlock(ctx->builder);
        if (!LLVMGetBasicBlockTerminator(trueEndBB)) {
            LLVMBuildBr(ctx->builder, mergeBB);
        }
    }
    if (falseFallsThrough) {
        LLVMPositionBuilderAtEnd(ctx->builder, falseEndBB);
        if (phiFalseValue && LLVMTypeOf(phiFalseValue) != mergedType) {
            phiFalseValue = cg_cast_value(ctx, phiFalseValue, mergedType, falseParsed, trueParsed, "ternary.false.cast");
        }
        falseEndBB = LLVMGetInsertBlock(ctx->builder);
        if (!LLVMGetBasicBlockTerminator(falseEndBB)) {
            LLVMBuildBr(ctx->builder, mergeBB);
        }
    }

    LLVMPositionBuilderAtEnd(ctx->builder, mergeBB);
    LLVMValueRef phi = LLVMBuildPhi(ctx->builder, mergedType, "ternaryResult");
    LLVMValueRef incomingVals[2];
    LLVMBasicBlockRef incomingBlocks[2];
    int count = 0;
    if (phiTrueValue && trueFallsThrough) {
        incomingVals[count] = phiTrueValue;
        incomingBlocks[count] = trueEndBB;
        count++;
    }
    if (phiFalseValue && falseFallsThrough) {
        incomingVals[count] = phiFalseValue;
        incomingBlocks[count] = falseEndBB;
        count++;
    }
    if (count == 0) {
        return NULL;
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
