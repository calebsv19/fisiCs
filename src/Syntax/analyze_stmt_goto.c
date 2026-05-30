// SPDX-License-Identifier: Apache-2.0

#include "analyze_stmt.h"

#include "const_eval.h"
#include "syntax_errors.h"
#include "Parser/Helpers/parsed_type.h"

#include <stdio.h>
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
    SourceRange location;
    SourceRange macroCallSite;
    SourceRange macroDefinition;
} LabelInfo;

typedef struct {
    const char* name;
    ASTNode* block;
    size_t index;
    SourceRange location;
    SourceRange macroCallSite;
    SourceRange macroDefinition;
} GotoInfo;

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
                         SourceRange location,
                         SourceRange macroCallSite,
                         SourceRange macroDefinition) {
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
        .location = location,
        .macroCallSite = macroCallSite,
        .macroDefinition = macroDefinition
    };
    (*count)++;
}

static void gotoInfoAdd(GotoInfo** list,
                        size_t* count,
                        size_t* capacity,
                        const char* name,
                        ASTNode* block,
                        size_t index,
                        SourceRange location,
                        SourceRange macroCallSite,
                        SourceRange macroDefinition) {
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
        .location = location,
        .macroCallSite = macroCallSite,
        .macroDefinition = macroDefinition
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
            labelInfoAdd(labels,
                         labelCount,
                         labelCapacity,
                         stmt->label.labelName,
                         block,
                         i,
                         stmt->location,
                         stmt->macroCallSite,
                         stmt->macroDefinition);
        } else if (stmt->type == AST_GOTO_STATEMENT && stmt->gotoStmt.label) {
            gotoInfoAdd(gotos,
                        gotoCount,
                        gotoCapacity,
                        stmt->gotoStmt.label,
                        block,
                        i,
                        stmt->location,
                        stmt->macroCallSite,
                        stmt->macroDefinition);
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
            addErrorWithRanges(gt->location, gt->macroCallSite, gt->macroDefinition, buffer, NULL);
            continue;
        }

        BlockInitInfo* labelBlockInfo = blockInitInfoFind(blocks, blockCount, label->block);
        if (label->block == gt->block) {
            if (gt->index < label->index &&
                blockHasInitBetween(labelBlockInfo, gt->index, label->index)) {
                addErrorWithRanges(gt->location,
                                   gt->macroCallSite,
                                   gt->macroDefinition,
                                   "goto jumps into scope of initialized variable",
                                   gt->name);
            }
            continue;
        }

        if (blockIsDescendant(blocks, blockCount, label->block, gt->block) &&
            blockHasInitBefore(labelBlockInfo, label->index)) {
            addErrorWithRanges(gt->location,
                               gt->macroCallSite,
                               gt->macroDefinition,
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
