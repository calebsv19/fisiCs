// SPDX-License-Identifier: Apache-2.0

#include "analyze_stmt.h"
#include "analyze_core.h"
#include "analyze_decls.h"
#include "analyze_expr.h"
#include "const_eval.h"
#include "syntax_errors.h"
#include "type_checker.h"
#include "symbol_table.h"
#include "Parser/Helpers/parsed_type.h"
#include "Lexer/tokens.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    ASTNode* block;
    ASTNode* parent;
    size_t* initIndices;
    size_t initCount;
    size_t initCapacity;
} BlockInitInfo;

typedef struct {
    const char* name;
    ASTNode* block;
    size_t index;
    int line;
} LabelInfo;

typedef struct {
    const char* name;
    ASTNode* block;
    size_t index;
    int line;
} GotoInfo;

typedef struct {
    long long* values;
    SourceRange* locations;
    size_t count;
    size_t capacity;
    bool hasDefault;
    SourceRange defaultLoc;
} SwitchFrame;

#define SWITCH_STACK_MAX 32

typedef struct {
    SwitchFrame frames[SWITCH_STACK_MAX];
    int depth;
} SwitchStack;

static void blockInitInfoFree(BlockInitInfo* info) {
    if (!info) return;
    free(info->initIndices);
    info->initIndices = NULL;
    info->initCount = 0;
    info->initCapacity = 0;
}

static BlockInitInfo* blockInitInfoAdd(BlockInitInfo** list,
                                       size_t* count,
                                       size_t* capacity,
                                       ASTNode* block,
                                       ASTNode* parent) {
    if (!list || !count || !capacity) return NULL;
    if (*count >= *capacity) {
        size_t newCap = *capacity ? *capacity * 2 : 8;
        BlockInitInfo* resized = (BlockInitInfo*)realloc(*list, newCap * sizeof(BlockInitInfo));
        if (!resized) return NULL;
        *list = resized;
        *capacity = newCap;
    }
    BlockInitInfo* info = &(*list)[(*count)++];
    info->block = block;
    info->parent = parent;
    info->initIndices = NULL;
    info->initCount = 0;
    info->initCapacity = 0;
    return info;
}

static void blockInitInfoAddIndex(BlockInitInfo* info, size_t index) {
    if (!info) return;
    if (info->initCount >= info->initCapacity) {
        size_t newCap = info->initCapacity ? info->initCapacity * 2 : 8;
        size_t* resized = (size_t*)realloc(info->initIndices, newCap * sizeof(size_t));
        if (!resized) return;
        info->initIndices = resized;
        info->initCapacity = newCap;
    }
    info->initIndices[info->initCount++] = index;
}

static BlockInitInfo* blockInitInfoFind(BlockInitInfo* list, size_t count, ASTNode* block) {
    for (size_t i = 0; i < count; ++i) {
        if (list[i].block == block) {
            return &list[i];
        }
    }
    return NULL;
}

static bool varDeclHasInitOrVLA(ASTNode* node) {
    if (!node || node->type != AST_VARIABLE_DECLARATION) return false;
    for (size_t i = 0; i < node->varDecl.varCount; ++i) {
        const ParsedType* t = astVarDeclTypeAt(node, i);
        if (t) {
            if (parsedTypeHasVLA(t)) {
                return true;
            }
            for (size_t d = 0; d < t->derivationCount; ++d) {
                const TypeDerivation* deriv = parsedTypeGetDerivation(t, d);
                if (!deriv || deriv->kind != TYPE_DERIVATION_ARRAY) {
                    continue;
                }
                if (deriv->as.array.isVLA) {
                    return true;
                }
                ASTNode* sizeExpr = deriv->as.array.sizeExpr;
                if (!sizeExpr) {
                    continue;
                }
                long long ignored = 0;
                if (!constEvalInteger(sizeExpr, NULL, &ignored, true)) {
                    return true;
                }
            }
        }
        if (node->varDecl.initializers && node->varDecl.initializers[i]) {
            return true;
        }
    }
    return false;
}

static void labelInfoAdd(LabelInfo** list,
                         size_t* count,
                         size_t* capacity,
                         const char* name,
                         ASTNode* block,
                         size_t index,
                         int line) {
    if (!list || !count || !capacity || !name) return;
    if (*count >= *capacity) {
        size_t newCap = *capacity ? *capacity * 2 : 8;
        LabelInfo* resized = (LabelInfo*)realloc(*list, newCap * sizeof(LabelInfo));
        if (!resized) return;
        *list = resized;
        *capacity = newCap;
    }
    (*list)[*count] = (LabelInfo){
        .name = name,
        .block = block,
        .index = index,
        .line = line
    };
    (*count)++;
}

static void gotoInfoAdd(GotoInfo** list,
                        size_t* count,
                        size_t* capacity,
                        const char* name,
                        ASTNode* block,
                        size_t index,
                        int line) {
    if (!list || !count || !capacity || !name) return;
    if (*count >= *capacity) {
        size_t newCap = *capacity ? *capacity * 2 : 8;
        GotoInfo* resized = (GotoInfo*)realloc(*list, newCap * sizeof(GotoInfo));
        if (!resized) return;
        *list = resized;
        *capacity = newCap;
    }
    (*list)[*count] = (GotoInfo){
        .name = name,
        .block = block,
        .index = index,
        .line = line
    };
    (*count)++;
}

static void collectGotoInfoFromStatement(ASTNode* stmt,
                                         ASTNode* currentBlock,
                                         size_t stmtIndex,
                                         BlockInitInfo** blocks,
                                         size_t* blockCount,
                                         size_t* blockCapacity,
                                         LabelInfo** labels,
                                         size_t* labelCount,
                                         size_t* labelCapacity,
                                         GotoInfo** gotos,
                                         size_t* gotoCount,
                                         size_t* gotoCapacity);

static void collectGotoInfoFromBlock(ASTNode* block,
                                     ASTNode* parentBlock,
                                     BlockInitInfo** blocks,
                                     size_t* blockCount,
                                     size_t* blockCapacity,
                                     LabelInfo** labels,
                                     size_t* labelCount,
                                     size_t* labelCapacity,
                                     GotoInfo** gotos,
                                     size_t* gotoCount,
                                     size_t* gotoCapacity) {
    if (!block || block->type != AST_BLOCK) return;
    BlockInitInfo* blockInfo = blockInitInfoAdd(blocks, blockCount, blockCapacity, block, parentBlock);
    if (!blockInfo) return;
    size_t blockIndex = (*blockCount) - 1;
    for (size_t i = 0; i < block->block.statementCount; ++i) {
        ASTNode* stmt = block->block.statements[i];
        if (!stmt) continue;
        if (stmt->type == AST_VARIABLE_DECLARATION && varDeclHasInitOrVLA(stmt)) {
            blockInitInfoAddIndex(&(*blocks)[blockIndex], i);
        }
        if (stmt->type == AST_LABEL_DECLARATION && stmt->label.labelName) {
            labelInfoAdd(labels, labelCount, labelCapacity, stmt->label.labelName, block, i, stmt->line);
        } else if (stmt->type == AST_GOTO_STATEMENT && stmt->gotoStmt.label) {
            gotoInfoAdd(gotos, gotoCount, gotoCapacity, stmt->gotoStmt.label, block, i, stmt->line);
        }
        collectGotoInfoFromStatement(stmt,
                                     block,
                                     i,
                                     blocks,
                                     blockCount,
                                     blockCapacity,
                                     labels,
                                     labelCount,
                                     labelCapacity,
                                     gotos,
                                     gotoCount,
                                     gotoCapacity);
    }
}

static void collectGotoInfoFromStatement(ASTNode* stmt,
                                         ASTNode* currentBlock,
                                         size_t stmtIndex,
                                         BlockInitInfo** blocks,
                                         size_t* blockCount,
                                         size_t* blockCapacity,
                                         LabelInfo** labels,
                                         size_t* labelCount,
                                         size_t* labelCapacity,
                                         GotoInfo** gotos,
                                         size_t* gotoCount,
                                         size_t* gotoCapacity) {
    if (!stmt) return;
    (void)currentBlock;
    (void)stmtIndex;
    switch (stmt->type) {
        case AST_BLOCK:
            collectGotoInfoFromBlock(stmt, currentBlock, blocks, blockCount, blockCapacity,
                                     labels, labelCount, labelCapacity,
                                     gotos, gotoCount, gotoCapacity);
            break;
        case AST_IF_STATEMENT:
            collectGotoInfoFromStatement(stmt->ifStmt.thenBranch, currentBlock, stmtIndex,
                                         blocks, blockCount, blockCapacity,
                                         labels, labelCount, labelCapacity,
                                         gotos, gotoCount, gotoCapacity);
            if (stmt->ifStmt.elseBranch) {
                collectGotoInfoFromStatement(stmt->ifStmt.elseBranch, currentBlock, stmtIndex,
                                             blocks, blockCount, blockCapacity,
                                             labels, labelCount, labelCapacity,
                                             gotos, gotoCount, gotoCapacity);
            }
            break;
        case AST_FOR_LOOP:
            collectGotoInfoFromStatement(stmt->forLoop.body, currentBlock, stmtIndex,
                                         blocks, blockCount, blockCapacity,
                                         labels, labelCount, labelCapacity,
                                         gotos, gotoCount, gotoCapacity);
            break;
        case AST_WHILE_LOOP:
            collectGotoInfoFromStatement(stmt->whileLoop.body, currentBlock, stmtIndex,
                                         blocks, blockCount, blockCapacity,
                                         labels, labelCount, labelCapacity,
                                         gotos, gotoCount, gotoCapacity);
            break;
        case AST_SWITCH:
            for (size_t i = 0; i < stmt->switchStmt.caseListSize; ++i) {
                collectGotoInfoFromStatement(stmt->switchStmt.caseList[i], currentBlock, stmtIndex,
                                             blocks, blockCount, blockCapacity,
                                             labels, labelCount, labelCapacity,
                                             gotos, gotoCount, gotoCapacity);
            }
            break;
        case AST_CASE:
            for (size_t i = 0; i < stmt->caseStmt.caseBodySize; ++i) {
                collectGotoInfoFromStatement(stmt->caseStmt.caseBody[i], currentBlock, stmtIndex,
                                             blocks, blockCount, blockCapacity,
                                             labels, labelCount, labelCapacity,
                                             gotos, gotoCount, gotoCapacity);
            }
            if (stmt->caseStmt.nextCase) {
                collectGotoInfoFromStatement(stmt->caseStmt.nextCase, currentBlock, stmtIndex,
                                             blocks, blockCount, blockCapacity,
                                             labels, labelCount, labelCapacity,
                                             gotos, gotoCount, gotoCapacity);
            }
            break;
        case AST_LABEL_DECLARATION:
            if (stmt->label.statement) {
                collectGotoInfoFromStatement(stmt->label.statement, currentBlock, stmtIndex,
                                             blocks, blockCount, blockCapacity,
                                             labels, labelCount, labelCapacity,
                                             gotos, gotoCount, gotoCapacity);
            }
            break;
        default:
            break;
    }
}

static bool blockHasInitBetween(const BlockInitInfo* info, size_t start, size_t end) {
    if (!info || info->initCount == 0) return false;
    for (size_t i = 0; i < info->initCount; ++i) {
        size_t idx = info->initIndices[i];
        if (idx > start && idx <= end) {
            return true;
        }
    }
    return false;
}

static bool blockHasInitBefore(const BlockInitInfo* info, size_t end) {
    if (!info || info->initCount == 0) return false;
    for (size_t i = 0; i < info->initCount; ++i) {
        if (info->initIndices[i] < end) {
            return true;
        }
    }
    return false;
}

static bool blockIsDescendant(BlockInitInfo* list, size_t count, ASTNode* block, ASTNode* ancestor) {
    ASTNode* current = block;
    while (current) {
        if (current == ancestor) return true;
        BlockInitInfo* info = blockInitInfoFind(list, count, current);
        current = info ? info->parent : NULL;
    }
    return false;
}

void validateGotoScopes(ASTNode* node) {
    if (!node || node->type != AST_BLOCK) return;

    BlockInitInfo* blocks = NULL;
    size_t blockCount = 0, blockCapacity = 0;
    LabelInfo* labels = NULL;
    size_t labelCount = 0, labelCapacity = 0;
    GotoInfo* gotos = NULL;
    size_t gotoCount = 0, gotoCapacity = 0;

    collectGotoInfoFromBlock(node,
                             NULL,
                             &blocks, &blockCount, &blockCapacity,
                             &labels, &labelCount, &labelCapacity,
                             &gotos, &gotoCount, &gotoCapacity);

    for (size_t i = 0; i < gotoCount; ++i) {
        GotoInfo* gt = &gotos[i];
        LabelInfo* label = NULL;
        for (size_t j = 0; j < labelCount; ++j) {
            if (labels[j].name && strcmp(labels[j].name, gt->name) == 0) {
                label = &labels[j];
                break;
            }
        }
        if (!label) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "goto to undefined label '%s'", gt->name ? gt->name : "");
            addError(gt->line, 0, buffer, NULL);
            continue;
        }

        BlockInitInfo* labelBlockInfo = blockInitInfoFind(blocks, blockCount, label->block);
        if (label->block == gt->block) {
            if (gt->index < label->index &&
                blockHasInitBetween(labelBlockInfo, gt->index, label->index)) {
                addError(gt->line, 0,
                         "goto jumps into scope of initialized variable",
                         gt->name);
            }
            continue;
        }

        if (blockIsDescendant(blocks, blockCount, label->block, gt->block) &&
            blockHasInitBefore(labelBlockInfo, label->index)) {
            addError(gt->line, 0,
                     "goto jumps into scope of initialized variable",
                     gt->name);
        }
    }

    for (size_t i = 0; i < blockCount; ++i) {
        blockInitInfoFree(&blocks[i]);
    }
    free(blocks);
    free(labels);
    free(gotos);
}

static void switchFrameFree(SwitchFrame* frame) {
    if (!frame) return;
    free(frame->values);
    free(frame->locations);
    frame->values = NULL;
    frame->locations = NULL;
    frame->count = 0;
    frame->capacity = 0;
    frame->hasDefault = false;
    frame->defaultLoc = (SourceRange){0};
}

static SwitchFrame* pushSwitchFrame(SwitchStack* stack) {
    if (!stack || stack->depth >= SWITCH_STACK_MAX) {
        return NULL;
    }
    SwitchFrame* frame = &stack->frames[stack->depth++];
    frame->values = NULL;
    frame->locations = NULL;
    frame->count = 0;
    frame->capacity = 0;
    frame->hasDefault = false;
    frame->defaultLoc = (SourceRange){0};
    return frame;
}

static void popSwitchFrame(SwitchStack* stack) {
    if (!stack || stack->depth <= 0) return;
    stack->depth--;
    switchFrameFree(&stack->frames[stack->depth]);
}

static bool switchFrameRecordValue(SwitchFrame* frame, long long value, SourceRange loc) {
    if (!frame) return false;
    for (size_t i = 0; i < frame->count; ++i) {
        if (frame->values[i] == value) {
            return false;
        }
    }
    if (frame->count == frame->capacity) {
        size_t newCap = frame->capacity == 0 ? 8 : frame->capacity * 2;
        long long* newVals = realloc(frame->values, newCap * sizeof(long long));
        SourceRange* newLocs = realloc(frame->locations, newCap * sizeof(SourceRange));
        if (!newVals || !newLocs) {
            free(newVals);
            free(newLocs);
            return false;
        }
        frame->values = newVals;
        frame->locations = newLocs;
        frame->capacity = newCap;
    }
    frame->values[frame->count] = value;
    frame->locations[frame->count] = loc;
    frame->count++;
    return true;
}

typedef struct {
    const char** names;
    SourceRange* locations;
    size_t count;
    size_t capacity;
} LabelTracker;

static bool labelTrackerRecord(LabelTracker* tracker,
                               const char* name,
                               SourceRange loc,
                               SourceRange* outPrev) {
    if (!tracker || !name) return true;
    for (size_t i = 0; i < tracker->count; ++i) {
        if (tracker->names[i] && strcmp(tracker->names[i], name) == 0) {
            if (outPrev) {
                *outPrev = tracker->locations[i];
            }
            return false;
        }
    }
    if (tracker->count == tracker->capacity) {
        size_t newCap = tracker->capacity == 0 ? 8 : tracker->capacity * 2;
        const char** newNames = realloc(tracker->names, newCap * sizeof(char*));
        SourceRange* newLocs = realloc(tracker->locations, newCap * sizeof(SourceRange));
        if (!newNames || !newLocs) {
            free(newNames);
            free(newLocs);
            return true;
        }
        tracker->names = newNames;
        tracker->locations = newLocs;
        tracker->capacity = newCap;
    }
    tracker->names[tracker->count] = name;
    tracker->locations[tracker->count] = loc;
    tracker->count++;
    return true;
}

static bool isDeclarationStatementType(ASTNodeType type) {
    switch (type) {
        case AST_VARIABLE_DECLARATION:
        case AST_STRUCT_DEFINITION:
        case AST_UNION_DEFINITION:
        case AST_ENUM_DEFINITION:
        case AST_TYPEDEF:
        case AST_STATIC_ASSERT:
        case AST_FUNCTION_DECLARATION:
            return true;
        default:
            return false;
    }
}

static bool isFunctionDesignatorExpr(ASTNode* expr, Scope* scope) {
    if (!expr || expr->type != AST_IDENTIFIER || !scope || !expr->valueNode.value) {
        return false;
    }
    Symbol* sym = resolveInScopeChain(scope, expr->valueNode.value);
    return sym && sym->kind == SYMBOL_FUNCTION;
}

static bool typeInfoIsScalar(const TypeInfo* info) {
    return typeInfoIsArithmetic(info) || typeInfoIsPointerLike(info);
}

static void analyzeControlCondition(ASTNode* expr,
                                    Scope* scope,
                                    bool requireInteger,
                                    int line,
                                    const char* stmtName) {
    if (!expr) return;
    TypeInfo cond = analyzeExpression(expr, scope);
    cond = decayToRValue(cond);
    if (cond.category == TYPEINFO_INVALID) {
        return;
    }
    if (requireInteger) {
        if (!typeInfoIsInteger(&cond)) {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "%s controlling expression must be integer", stmtName);
            addError(line, 0, buffer, NULL);
        }
        return;
    }
    if (!typeInfoIsScalar(&cond)) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%s controlling expression must be scalar", stmtName);
        addError(line, 0, buffer, NULL);
    }
}

static void analyzeStatementInternal(ASTNode* node,
                                     Scope* scope,
                                     SwitchStack* switchStack,
                                     LabelTracker* labels,
                                     int loopDepth) {
    switch (node->type) {
        case AST_IF_STATEMENT:
            analyzeControlCondition(node->ifStmt.condition, scope, false, node->line, "if");
            analyzeStatementInternal(node->ifStmt.thenBranch, scope, switchStack, labels, loopDepth);
            if (node->ifStmt.elseBranch) {
                analyzeStatementInternal(node->ifStmt.elseBranch, scope, switchStack, labels, loopDepth);
            }
            break;

        case AST_FOR_LOOP: {
            Scope* inner = createScope(scope);
            analyze(node->forLoop.initializer, inner);
            analyzeControlCondition(node->forLoop.condition, inner, false, node->line, "for");
            analyze(node->forLoop.increment, inner);
            analyzeStatementInternal(node->forLoop.body, inner, switchStack, labels, loopDepth + 1);
            destroyScope(inner);
            break;
        }

        case AST_WHILE_LOOP:
            analyzeControlCondition(node->whileLoop.condition,
                                    scope,
                                    false,
                                    node->line,
                                    node->whileLoop.isDoWhile ? "do-while" : "while");
            analyzeStatementInternal(node->whileLoop.body, scope, switchStack, labels, loopDepth + 1);
            break;

        case AST_RETURN:
            if (node->returnStmt.returnValue) {
                TypeInfo retVal = analyzeExpression(node->returnStmt.returnValue, scope);
                retVal = decayToRValue(retVal);
                if (scope && scope->hasReturnType) {
                    if (scope->returnType.category == TYPEINFO_VOID) {
                        addError(node->line, 0, "Void function should not return a value", NULL);
                    } else if (retVal.category != TYPEINFO_INVALID) {
                        AssignmentCheckResult res = canAssignTypes(&scope->returnType, &retVal);
                        if (res == ASSIGN_INCOMPATIBLE) {
                            bool allowFunctionDecay = isFunctionDesignatorExpr(node->returnStmt.returnValue, scope) &&
                                                      (typeInfoIsPointerLike(&scope->returnType) ||
                                                       scope->returnType.category == TYPEINFO_FUNCTION ||
                                                       scope->returnType.isFunction ||
                                                       scope->returnType.pointerDepth > 0);
                            if (!allowFunctionDecay) {
                                addError(node->line, 0, "Incompatible return type", NULL);
                            }
                        } else if (res == ASSIGN_QUALIFIER_LOSS) {
                            addError(node->line, 0, "Return discards qualifiers from pointer target", NULL);
                        }
                    }
                }
            } else {
                if (scope && scope->hasReturnType && scope->returnType.category != TYPEINFO_VOID) {
                    addError(node->line, 0, "Non-void function must return a value", NULL);
                }
            }
            break;

        case AST_BREAK:
            if (loopDepth == 0 && (!switchStack || switchStack->depth == 0)) {
                addError(node->line, 0, "Break statement not within loop or switch", NULL);
            }
            break;
        case AST_CONTINUE:
            if (loopDepth == 0) {
                addError(node->line, 0, "Continue statement not within a loop", NULL);
            }
            break;

        case AST_SWITCH: {
            analyzeControlCondition(node->switchStmt.condition, scope, true, node->line, "switch");
            SwitchFrame* frame = pushSwitchFrame(switchStack);
            for (size_t i = 0; i < node->switchStmt.caseListSize; i++) {
                analyzeStatementInternal(node->switchStmt.caseList[i], scope, switchStack, labels, loopDepth);
            }
            popSwitchFrame(switchStack);
            (void)frame;
            break;
        }

        case AST_CASE:
            if (node->caseStmt.caseBodySize > 0 &&
                node->caseStmt.caseBody &&
                node->caseStmt.caseBody[0] &&
                isDeclarationStatementType(node->caseStmt.caseBody[0]->type)) {
                addError(node->line,
                         0,
                         "label before declaration is not allowed in C99; wrap declaration in a block",
                         NULL);
            }
            if (node->caseStmt.caseValue) {
                analyze(node->caseStmt.caseValue, scope);
                ConstEvalResult res = constEval(node->caseStmt.caseValue, scope, true);
                if (!res.isConst) {
                    addError(node->caseStmt.caseValue->line, 0, "Case label is not an integer constant expression", NULL);
                } else if (switchStack && switchStack->depth > 0) {
                    SwitchFrame* frame = &switchStack->frames[switchStack->depth - 1];
                    if (!switchFrameRecordValue(frame, res.value, node->caseStmt.caseValue->location)) {
                        SourceRange prev = {0};
                        // find previous location to report
                        for (size_t i = 0; i < frame->count; ++i) {
                            if (frame->values[i] == res.value) {
                                prev = frame->locations[i];
                                break;
                            }
                        }
                        char buffer[128];
                        snprintf(buffer, sizeof(buffer), "Duplicate case label with value %lld", res.value);
                        addErrorWithRanges(node->caseStmt.caseValue->location,
                                           node->caseStmt.caseValue->macroCallSite,
                                           node->caseStmt.caseValue->macroDefinition,
                                           buffer,
                                           NULL);
                        if (prev.start.line > 0) {
                            addWarning(prev.start.line, 0, "Previous case label with same value here", NULL);
                        }
                    }
                }
            } else {
                if (switchStack && switchStack->depth > 0) {
                    SwitchFrame* frame = &switchStack->frames[switchStack->depth - 1];
                    if (frame->hasDefault) {
                        addErrorWithRanges(node->location,
                                           node->macroCallSite,
                                           node->macroDefinition,
                                           "Duplicate default label in switch",
                                           NULL);
                        if (frame->defaultLoc.start.line > 0) {
                            addWarning(frame->defaultLoc.start.line, 0, "Previous default label is here", NULL);
                        }
                    } else {
                        frame->hasDefault = true;
                        frame->defaultLoc = node->location;
                    }
                }
            }
            for (size_t i = 0; i < node->caseStmt.caseBodySize; i++) {
                analyzeStatementInternal(node->caseStmt.caseBody[i], scope, switchStack, labels, loopDepth);
            }
            if (node->caseStmt.nextCase) {
                analyzeStatementInternal(node->caseStmt.nextCase, scope, switchStack, labels, loopDepth);
            }
            break;

        case AST_LABEL_DECLARATION:
            if (node->label.labelName) {
                SourceRange prev = {0};
                if (!labelTrackerRecord(labels, node->label.labelName, node->location, &prev)) {
                    char buffer[256];
                    snprintf(buffer, sizeof(buffer), "Label '%s' redefined", node->label.labelName);
                    addWarning(node->line, 0, buffer, NULL);
                    if (prev.start.line > 0) {
                        addWarning(prev.start.line, 0, "Previous label is here", NULL);
                    }
                }
            }
            if (node->label.statement) {
                if (isDeclarationStatementType(node->label.statement->type)) {
                    addError(node->line,
                             0,
                             "label before declaration is not allowed in C99; wrap declaration in a block",
                             NULL);
                }
                analyze(node->label.statement, scope);
            }
            break;

        case AST_GOTO_STATEMENT:
            // Goto/label validation not implemented yet
            break;

        case AST_ASM:
            // Parsed/accepted; no semantic validation yet.
            break;

        case AST_BLOCK:
            if (scope) {
                Scope* inner = createScope(scope);
                for (size_t i = 0; i < node->block.statementCount; ++i) {
                    analyzeStatementInternal(node->block.statements[i], inner, switchStack, labels, loopDepth);
                }
                destroyScope(inner);
            } else {
                for (size_t i = 0; i < node->block.statementCount; ++i) {
                    analyzeStatementInternal(node->block.statements[i], scope, switchStack, labels, loopDepth);
                }
            }
            break;

        case AST_VARIABLE_DECLARATION:
        case AST_STRUCT_DEFINITION:
        case AST_UNION_DEFINITION:
        case AST_ENUM_DEFINITION:
        case AST_TYPEDEF:
        case AST_STATIC_ASSERT:
            analyzeDeclaration(node, scope);
            break;

        case AST_ASSIGNMENT:
        case AST_BINARY_EXPRESSION:
        case AST_UNARY_EXPRESSION:
        case AST_TERNARY_EXPRESSION:
        case AST_COMMA_EXPRESSION:
        case AST_FUNCTION_CALL:
        case AST_COMPOUND_LITERAL:
        case AST_CAST_EXPRESSION:
        case AST_STATEMENT_EXPRESSION:
        case AST_ARRAY_ACCESS:
        case AST_POINTER_ACCESS:
        case AST_POINTER_DEREFERENCE:
        case AST_DOT_ACCESS:
        case AST_IDENTIFIER:
        case AST_NUMBER_LITERAL:
        case AST_CHAR_LITERAL:
        case AST_STRING_LITERAL:
        case AST_SIZEOF:
        case AST_ALIGNOF:
            (void)analyzeExpression(node, scope);
            break;

        default:
            addError(node ? node->line : 0, 0, "Unhandled statement node", "No analysis implemented for this statement type");
            break;
    }
}

void analyzeStatement(ASTNode* node, Scope* scope) {
    SwitchStack stack = {0};
    LabelTracker labels = {0};
    analyzeStatementInternal(node, scope, &stack, &labels, 0);
    free(labels.names);
    free(labels.locations);
}
