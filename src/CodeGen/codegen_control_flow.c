// SPDX-License-Identifier: Apache-2.0

#include "codegen_private.h"

#include <stdio.h>
#include <stdlib.h>

static bool cg_builder_block_terminated(const CodegenContext* ctx) {
    if (!ctx || !ctx->builder) {
        return false;
    }
    LLVMBasicBlockRef block = LLVMGetInsertBlock(ctx->builder);
    if (!block) {
        return false;
    }
    return LLVMGetBasicBlockTerminator(block) != NULL;
}

static bool cg_statement_can_reopen_block(const ASTNode* stmt) {
    return stmt && stmt->type == AST_LABEL_DECLARATION;
}

static bool cg_should_emit_statement_in_current_block(const CodegenContext* ctx,
                                                      const ASTNode* stmt) {
    if (!stmt) {
        return false;
    }
    if (!cg_builder_block_terminated(ctx)) {
        return true;
    }
    return cg_statement_can_reopen_block(stmt);
}

LLVMValueRef codegenIfStatement(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_IF_STATEMENT) {
        fprintf(stderr, "Error: Invalid node type for codegenIfStatement\n");
        return NULL;
    }

    LLVMValueRef cond = codegenNode(ctx, node->ifStmt.condition);
    if (!cond) {
        fprintf(stderr, "Error: Failed to generate condition for if statement\n");
        return NULL;
    }

    cond = cg_build_truthy(ctx, cond, NULL, "ifcond");
    if (!cond) {
        fprintf(stderr, "Error: Failed to convert if condition to boolean\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef thenBB = LLVMAppendBasicBlock(func, "then");
    LLVMBasicBlockRef elseBB = node->ifStmt.elseBranch ? LLVMAppendBasicBlock(func, "else") : NULL;
    LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(func, "ifcont");

    LLVMBuildCondBr(ctx->builder, cond, thenBB, elseBB ? elseBB : mergeBB);

    LLVMPositionBuilderAtEnd(ctx->builder, thenBB);
    codegenNode(ctx, node->ifStmt.thenBranch);
    if (!LLVMGetInsertBlock(ctx->builder) || !LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->builder))) {
        LLVMBuildBr(ctx->builder, mergeBB);
    }

    if (elseBB) {
        LLVMPositionBuilderAtEnd(ctx->builder, elseBB);
        codegenNode(ctx, node->ifStmt.elseBranch);
        if (!LLVMGetInsertBlock(ctx->builder) || !LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->builder))) {
            LLVMBuildBr(ctx->builder, mergeBB);
        }
    }

    LLVMPositionBuilderAtEnd(ctx->builder, mergeBB);
    return NULL;
}

LLVMValueRef codegenWhileLoop(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_WHILE_LOOP) {
        fprintf(stderr, "Error: Invalid node type for codegenWhileLoop\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(func, "loopcond");
    LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(func, "loopbody");
    LLVMBasicBlockRef afterBB = LLVMAppendBasicBlock(func, "loopend");

    if (!node->whileLoop.isDoWhile) {
        LLVMBasicBlockRef current = LLVMGetInsertBlock(ctx->builder);
        if (current && !LLVMGetBasicBlockTerminator(current)) {
            LLVMBuildBr(ctx->builder, condBB);
        }

        LLVMPositionBuilderAtEnd(ctx->builder, condBB);
        LLVMValueRef cond = codegenNode(ctx, node->whileLoop.condition);
        cond = cg_build_truthy(ctx, cond, NULL, "loopcond");
        if (!cond) {
            fprintf(stderr, "Error: Failed to convert loop condition to boolean\n");
            return NULL;
        }
        LLVMBuildCondBr(ctx->builder, cond, bodyBB, afterBB);

        LLVMPositionBuilderAtEnd(ctx->builder, bodyBB);
        cg_loop_push(ctx, afterBB, condBB);
        codegenNode(ctx, node->whileLoop.body);
        cg_loop_pop(ctx);
        LLVMBasicBlockRef bodyEnd = LLVMGetInsertBlock(ctx->builder);
        if (bodyEnd && !LLVMGetBasicBlockTerminator(bodyEnd)) {
            LLVMBuildBr(ctx->builder, condBB);
        }
    } else {
        LLVMBasicBlockRef current = LLVMGetInsertBlock(ctx->builder);
        if (current && !LLVMGetBasicBlockTerminator(current)) {
            LLVMBuildBr(ctx->builder, bodyBB);
        }

        LLVMPositionBuilderAtEnd(ctx->builder, bodyBB);
        cg_loop_push(ctx, afterBB, condBB);
        codegenNode(ctx, node->whileLoop.body);
        cg_loop_pop(ctx);
        LLVMBasicBlockRef bodyEnd = LLVMGetInsertBlock(ctx->builder);
        if (bodyEnd && !LLVMGetBasicBlockTerminator(bodyEnd)) {
            LLVMBuildBr(ctx->builder, condBB);
        }

        LLVMPositionBuilderAtEnd(ctx->builder, condBB);
        LLVMValueRef cond = codegenNode(ctx, node->whileLoop.condition);
        cond = cg_build_truthy(ctx, cond, NULL, "loopcond");
        if (!cond) {
            fprintf(stderr, "Error: Failed to convert do-while condition to boolean\n");
            return NULL;
        }
        LLVMBuildCondBr(ctx->builder, cond, bodyBB, afterBB);
    }

    LLVMPositionBuilderAtEnd(ctx->builder, afterBB);
    return NULL;
}

LLVMValueRef codegenForLoop(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_FOR_LOOP) {
        fprintf(stderr, "Error: Invalid node type for codegenForLoop\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(func, "forcond");
    LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(func, "forbody");
    LLVMBasicBlockRef incBB = LLVMAppendBasicBlock(func, "forinc");
    LLVMBasicBlockRef afterBB = LLVMAppendBasicBlock(func, "forend");

    if (node->forLoop.initializer) {
        codegenNode(ctx, node->forLoop.initializer);
    }

    LLVMBasicBlockRef startBB = LLVMGetInsertBlock(ctx->builder);
    if (startBB && !LLVMGetBasicBlockTerminator(startBB)) {
        LLVMBuildBr(ctx->builder, condBB);
    }

    LLVMPositionBuilderAtEnd(ctx->builder, condBB);
    LLVMValueRef cond = node->forLoop.condition ? codegenNode(ctx, node->forLoop.condition)
                                                : LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 1, 0);
    cond = cg_build_truthy(ctx, cond, NULL, "forcond");
    if (!cond) {
        fprintf(stderr, "Error: Failed to convert for condition to boolean\n");
        return NULL;
    }
    LLVMBuildCondBr(ctx->builder, cond, bodyBB, afterBB);

    LLVMPositionBuilderAtEnd(ctx->builder, bodyBB);
    cg_loop_push(ctx, afterBB, incBB);
    codegenNode(ctx, node->forLoop.body);
    cg_loop_pop(ctx);
    LLVMBasicBlockRef bodyEnd = LLVMGetInsertBlock(ctx->builder);
    if (bodyEnd && !LLVMGetBasicBlockTerminator(bodyEnd)) {
        LLVMBuildBr(ctx->builder, incBB);
    }

    LLVMPositionBuilderAtEnd(ctx->builder, incBB);
    if (node->forLoop.increment) {
        codegenNode(ctx, node->forLoop.increment);
    }
    LLVMBasicBlockRef incEnd = LLVMGetInsertBlock(ctx->builder);
    if (incEnd && !LLVMGetBasicBlockTerminator(incEnd)) {
        LLVMBuildBr(ctx->builder, condBB);
    }

    LLVMPositionBuilderAtEnd(ctx->builder, afterBB);
    return NULL;
}

LLVMValueRef codegenBreak(CodegenContext* ctx, ASTNode* node) {
    (void)node;
    LoopTarget target = cg_loop_peek(ctx);
    if (!target.breakBB) {
        fprintf(stderr, "Error: 'break' used outside of loop\n");
        return NULL;
    }
    return LLVMBuildBr(ctx->builder, target.breakBB);
}

LLVMValueRef codegenContinue(CodegenContext* ctx, ASTNode* node) {
    (void)node;
    LoopTarget target = cg_loop_peek_for_continue(ctx);
    if (!target.continueBB) {
        fprintf(stderr, "Error: 'continue' used outside of loop\n");
        return NULL;
    }
    return LLVMBuildBr(ctx->builder, target.continueBB);
}

typedef struct {
    long long value;
    LLVMValueRef constVal;
    ASTNode* caseNode;
} CGCaseEntry;

static bool cg_const_int_from_value(LLVMValueRef v, long long* out) {
    if (!v || LLVMGetTypeKind(LLVMTypeOf(v)) != LLVMIntegerTypeKind) return false;
    if (!LLVMIsConstant(v)) return false;
    if (out) *out = (long long)LLVMConstIntGetSExtValue(v);
    return true;
}

LLVMValueRef codegenSwitch(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_SWITCH) {
        fprintf(stderr, "Error: Invalid node type for codegenSwitch\n");
        return NULL;
    }

    LLVMValueRef condition = codegenNode(ctx, node->switchStmt.condition);
    if (!condition) {
        fprintf(stderr, "Error: Failed to generate switch condition\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef switchEnd = LLVMAppendBasicBlock(func, "switch.end");
    LLVMBasicBlockRef defaultBB = LLVMAppendBasicBlock(func, "switch.default");
    LLVMTypeRef conditionType = LLVMTypeOf(condition);
    bool conditionIsInteger = LLVMGetTypeKind(conditionType) == LLVMIntegerTypeKind;
    if (!conditionIsInteger) {
        LLVMTypeRef intptrTy = cg_get_intptr_type(ctx);
        if (LLVMGetTypeKind(conditionType) == LLVMPointerTypeKind) {
            condition = LLVMBuildPtrToInt(ctx->builder, condition, intptrTy, "switch.ptrtoint");
        } else {
            condition = LLVMBuildBitCast(ctx->builder, condition, intptrTy, "switch.to.int");
        }
        conditionType = LLVMTypeOf(condition);
    }

    size_t caseCount = node->switchStmt.caseListSize;
    CGCaseEntry* entries = calloc(caseCount, sizeof(CGCaseEntry));
    size_t realCases = 0;
    ASTNode* defaultCase = NULL;
    for (size_t i = 0; i < caseCount; ++i) {
        ASTNode* caseNode = node->switchStmt.caseList[i];
        if (!caseNode || caseNode->type != AST_CASE) continue;
        if (!caseNode->caseStmt.caseValue) {
            defaultCase = caseNode;
            continue;
        }
        LLVMValueRef caseValue = codegenNode(ctx, caseNode->caseStmt.caseValue);
        if (!caseValue) continue;
        if (LLVMTypeOf(caseValue) != conditionType) {
            caseValue = cg_cast_value(ctx, caseValue, conditionType, NULL, NULL, "case.cast");
        }
        long long val = 0;
        if (!cg_const_int_from_value(caseValue, &val)) continue;
        entries[realCases].value = val;
        entries[realCases].constVal = caseValue;
        entries[realCases].caseNode = caseNode;
        realCases++;
    }

    long long minVal = 0, maxVal = 0;
    if (realCases > 0) {
        minVal = maxVal = entries[0].value;
        for (size_t i = 1; i < realCases; ++i) {
            if (entries[i].value < minVal) minVal = entries[i].value;
            if (entries[i].value > maxVal) maxVal = entries[i].value;
        }
    }

    bool dense = false;
    if (realCases > 0) {
        long long span = maxVal - minVal + 1;
        dense = conditionIsInteger && span > 0 && span <= (long long)(realCases * 2);
    }

    cg_loop_push(ctx, switchEnd, NULL);
    LLVMBasicBlockRef pendingFallthrough = NULL;

    if (dense) {
        LLVMValueRef switchInst = LLVMBuildSwitch(ctx->builder,
                                                  condition,
                                                  defaultBB,
                                                  (unsigned)realCases);
        for (size_t i = 0; i < realCases; ++i) {
            ASTNode* caseNode = entries[i].caseNode;
            LLVMBasicBlockRef caseBB = LLVMAppendBasicBlock(func, "switch.case");
            LLVMAddCase(switchInst, entries[i].constVal, caseBB);
            if (pendingFallthrough) {
                LLVMPositionBuilderAtEnd(ctx->builder, pendingFallthrough);
                LLVMBuildBr(ctx->builder, caseBB);
                pendingFallthrough = NULL;
            }
            LLVMPositionBuilderAtEnd(ctx->builder, caseBB);
            for (size_t stmtIdx = 0; stmtIdx < caseNode->caseStmt.caseBodySize; ++stmtIdx) {
                ASTNode* stmt = caseNode->caseStmt.caseBody[stmtIdx];
                if (!cg_should_emit_statement_in_current_block(ctx, stmt)) {
                    continue;
                }
                codegenNode(ctx, stmt);
            }
            LLVMBasicBlockRef currentBB = LLVMGetInsertBlock(ctx->builder);
            if (currentBB && !LLVMGetBasicBlockTerminator(currentBB)) {
                pendingFallthrough = currentBB;
            }
        }
    } else {
        LLVMBasicBlockRef nextTest = LLVMGetInsertBlock(ctx->builder);
        for (size_t i = 0; i < realCases; ++i) {
            ASTNode* caseNode = entries[i].caseNode;
            LLVMBasicBlockRef caseBB = LLVMAppendBasicBlock(func, "switch.case");
            LLVMBasicBlockRef contBB = LLVMAppendBasicBlock(func, "switch.next");
            if (pendingFallthrough) {
                LLVMPositionBuilderAtEnd(ctx->builder, pendingFallthrough);
                LLVMBuildBr(ctx->builder, caseBB);
                pendingFallthrough = NULL;
            }
            LLVMPositionBuilderAtEnd(ctx->builder, nextTest);
            LLVMValueRef cmp = LLVMBuildICmp(ctx->builder,
                                             LLVMIntEQ,
                                             condition,
                                             entries[i].constVal,
                                             "switch.cmp");
            LLVMBuildCondBr(ctx->builder, cmp, caseBB, contBB);

            LLVMPositionBuilderAtEnd(ctx->builder, caseBB);
            for (size_t stmtIdx = 0; stmtIdx < caseNode->caseStmt.caseBodySize; ++stmtIdx) {
                ASTNode* stmt = caseNode->caseStmt.caseBody[stmtIdx];
                if (!cg_should_emit_statement_in_current_block(ctx, stmt)) {
                    continue;
                }
                codegenNode(ctx, stmt);
            }
            LLVMBasicBlockRef currentBB = LLVMGetInsertBlock(ctx->builder);
            if (currentBB && !LLVMGetBasicBlockTerminator(currentBB)) {
                pendingFallthrough = currentBB;
            }
            nextTest = contBB;
        }
        LLVMPositionBuilderAtEnd(ctx->builder, nextTest);
        LLVMBuildBr(ctx->builder, defaultBB);
    }

    free(entries);

    if (pendingFallthrough) {
        LLVMPositionBuilderAtEnd(ctx->builder, pendingFallthrough);
        LLVMBuildBr(ctx->builder, defaultCase ? defaultBB : switchEnd);
        pendingFallthrough = NULL;
    }

    LLVMPositionBuilderAtEnd(ctx->builder, defaultBB);
    if (defaultCase) {
        for (size_t stmtIdx = 0; stmtIdx < defaultCase->caseStmt.caseBodySize; ++stmtIdx) {
            ASTNode* stmt = defaultCase->caseStmt.caseBody[stmtIdx];
            if (!cg_should_emit_statement_in_current_block(ctx, stmt)) {
                continue;
            }
            codegenNode(ctx, stmt);
        }
    }
    if (!LLVMGetBasicBlockTerminator(defaultBB)) {
        LLVMBuildBr(ctx->builder, switchEnd);
    }

    cg_loop_pop(ctx);

    LLVMPositionBuilderAtEnd(ctx->builder, switchEnd);
    return NULL;
}

LLVMValueRef codegenLabel(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_LABEL_DECLARATION) {
        fprintf(stderr, "Error: Invalid node for label codegen\n");
        return NULL;
    }
    const char* name = node->label.labelName;
    if (!name) {
        fprintf(stderr, "Error: label missing name\n");
        return NULL;
    }

    LabelBinding* binding = cg_ensure_label(ctx, name);
    if (!binding) {
        fprintf(stderr, "Error: Failed to allocate label binding\n");
        return NULL;
    }

    LLVMBasicBlockRef insertBlock = LLVMGetInsertBlock(ctx->builder);
    LLVMValueRef func = insertBlock ? LLVMGetBasicBlockParent(insertBlock) : NULL;
    LLVMBasicBlockRef block = cg_label_ensure_block(ctx, binding, func);
    if (!block) {
        fprintf(stderr, "Error: Failed to create label block\n");
        return NULL;
    }

    binding->defined = true;

    if (insertBlock && insertBlock != block && !LLVMGetBasicBlockTerminator(insertBlock)) {
        LLVMBuildBr(ctx->builder, block);
    }

    LLVMPositionBuilderAtEnd(ctx->builder, block);
    if (node->label.statement) {
        codegenNode(ctx, node->label.statement);
    }
    return NULL;
}

LLVMValueRef codegenGoto(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_GOTO_STATEMENT) {
        fprintf(stderr, "Error: Invalid node for goto codegen\n");
        return NULL;
    }
    const char* name = node->gotoStmt.label;
    if (!name) {
        fprintf(stderr, "Error: goto missing label name\n");
        return NULL;
    }

    LabelBinding* binding = cg_ensure_label(ctx, name);
    if (!binding) {
        fprintf(stderr, "Error: Failed to allocate goto label binding\n");
        return NULL;
    }

    LLVMBasicBlockRef insertBlock = LLVMGetInsertBlock(ctx->builder);
    LLVMValueRef func = insertBlock ? LLVMGetBasicBlockParent(insertBlock) : NULL;
    LLVMBasicBlockRef target = cg_label_ensure_block(ctx, binding, func);
    if (!target) {
        fprintf(stderr, "Error: Failed to materialize goto target block\n");
        return NULL;
    }

    LLVMBuildBr(ctx->builder, target);

    if (func) {
        LLVMBasicBlockRef sink = LLVMAppendBasicBlock(func, "goto.sink");
        LLVMPositionBuilderAtEnd(ctx->builder, sink);
        LLVMBuildUnreachable(ctx->builder);
        LLVMBasicBlockRef cont = LLVMAppendBasicBlock(func, "goto.cont");
        LLVMPositionBuilderAtEnd(ctx->builder, cont);
    }
    return NULL;
}
