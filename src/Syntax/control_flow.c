#include "control_flow.h"
#include "syntax_errors.h"
#include "type_checker.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char** names;
    size_t count;
    size_t capacity;
} GotoTargetSet;

typedef struct {
    bool inLoop;
    bool inSwitch;
    bool reachable;
    bool suppressUnreachable;
    const GotoTargetSet* gotoTargets;
} FlowContext;

typedef struct {
    bool returns;
    bool stops;
} FlowResult;

static FlowResult makeFlow(bool returns, bool stops) {
    FlowResult flow;
    flow.returns = returns;
    flow.stops = stops;
    return flow;
}

static int safeLine(ASTNode* node) {
    if (!node || node->line <= 0) {
        return 0;
    }
    return node->line;
}

static void gotoTargetSetFree(GotoTargetSet* set) {
    if (!set) return;
    for (size_t i = 0; i < set->count; ++i) {
        free(set->names[i]);
    }
    free(set->names);
    set->names = NULL;
    set->count = 0;
    set->capacity = 0;
}

static void gotoTargetSetAdd(GotoTargetSet* set, const char* name) {
    if (!set || !name || !name[0]) return;
    for (size_t i = 0; i < set->count; ++i) {
        if (set->names[i] && strcmp(set->names[i], name) == 0) {
            return;
        }
    }
    if (set->count == set->capacity) {
        size_t newCap = (set->capacity == 0) ? 8 : set->capacity * 2;
        char** grown = (char**)realloc(set->names, newCap * sizeof(char*));
        if (!grown) return;
        set->names = grown;
        set->capacity = newCap;
    }
    char* copy = strdup(name);
    if (!copy) return;
    set->names[set->count++] = copy;
}

static bool gotoTargetSetContains(const GotoTargetSet* set, const char* name) {
    if (!set || !name || !name[0]) return false;
    for (size_t i = 0; i < set->count; ++i) {
        if (set->names[i] && strcmp(set->names[i], name) == 0) {
            return true;
        }
    }
    return false;
}

static void collectGotoTargets(ASTNode* node, GotoTargetSet* set) {
    if (!node || !set) return;
    switch (node->type) {
        case AST_BLOCK:
        case AST_PROGRAM:
            for (size_t i = 0; i < node->block.statementCount; ++i) {
                collectGotoTargets(node->block.statements[i], set);
            }
            break;
        case AST_IF_STATEMENT:
            collectGotoTargets(node->ifStmt.thenBranch, set);
            collectGotoTargets(node->ifStmt.elseBranch, set);
            break;
        case AST_WHILE_LOOP:
            collectGotoTargets(node->whileLoop.condition, set);
            collectGotoTargets(node->whileLoop.body, set);
            break;
        case AST_FOR_LOOP:
            collectGotoTargets(node->forLoop.initializer, set);
            collectGotoTargets(node->forLoop.condition, set);
            collectGotoTargets(node->forLoop.increment, set);
            collectGotoTargets(node->forLoop.body, set);
            break;
        case AST_SWITCH:
            collectGotoTargets(node->switchStmt.condition, set);
            for (size_t i = 0; i < node->switchStmt.caseListSize; ++i) {
                collectGotoTargets(node->switchStmt.caseList[i], set);
            }
            break;
        case AST_CASE:
            for (size_t i = 0; i < node->caseStmt.caseBodySize; ++i) {
                collectGotoTargets(node->caseStmt.caseBody[i], set);
            }
            collectGotoTargets(node->caseStmt.nextCase, set);
            break;
        case AST_LABEL_DECLARATION:
            collectGotoTargets(node->label.statement, set);
            break;
        case AST_GOTO_STATEMENT:
            gotoTargetSetAdd(set, node->gotoStmt.label);
            break;
        default:
            break;
    }
}

static FlowResult checkStatement(ASTNode* node, Scope* scope, FlowContext ctx);
static FlowResult checkStatementList(ASTNode** list, size_t count, Scope* scope, FlowContext ctx);

static void reportUnreachable(ASTNode* node) {
    addWarning(safeLine(node), 0, "Unreachable statement", NULL);
}

static FlowResult checkBlock(ASTNode* block, Scope* scope, FlowContext ctx) {
    if (!block) return makeFlow(false, false);
    return checkStatementList(block->block.statements, block->block.statementCount, scope, ctx);
}

static FlowResult checkCaseBody(ASTNode* caseNode, Scope* scope, FlowContext ctx) {
    FlowResult result = makeFlow(false, false);
    if (!caseNode) return result;

    FlowResult bodyFlow = checkStatementList(caseNode->caseStmt.caseBody,
                                             caseNode->caseStmt.caseBodySize,
                                             scope,
                                             ctx);
    return bodyFlow;
}

static FlowResult checkSwitch(ASTNode* node, Scope* scope, FlowContext ctx) {
    FlowResult result = makeFlow(false, false);
    if (!node) return result;

    FlowContext caseCtx = ctx;
    caseCtx.inSwitch = true;

    size_t caseCount = node->switchStmt.caseListSize;
    FlowResult* caseFlows = NULL;
    bool* isDefault = NULL;
    FlowResult* chainFlows = NULL;
    if (caseCount > 0) {
        caseFlows = (FlowResult*)calloc(caseCount, sizeof(FlowResult));
        isDefault = (bool*)calloc(caseCount, sizeof(bool));
        chainFlows = (FlowResult*)calloc(caseCount, sizeof(FlowResult));
        if (!caseFlows || !isDefault || !chainFlows) {
            free(caseFlows);
            free(isDefault);
            free(chainFlows);
            return result;
        }
    }

    bool sawDefault = false;
    for (size_t i = 0; i < caseCount; ++i) {
        ASTNode* caseNode = node->switchStmt.caseList[i];
        if (!caseNode) {
            caseFlows[i] = makeFlow(false, false);
            continue;
        }

        bool caseReachable = ctx.reachable;
        if (!caseReachable && !caseCtx.suppressUnreachable) {
            reportUnreachable(caseNode);
        }

        FlowContext thisCtx = caseCtx;
        thisCtx.reachable = caseReachable;
        thisCtx.suppressUnreachable = !caseReachable;

        FlowResult caseFlow = checkCaseBody(caseNode, scope, thisCtx);
        caseFlows[i] = caseFlow;

        if (!caseNode->caseStmt.caseValue) {
            sawDefault = true;
            if (isDefault) isDefault[i] = true;
        }

        bool hasStatements = caseNode->caseStmt.caseBodySize > 0;
        bool fallsThrough = caseReachable && hasStatements && !caseFlow.stops && (i + 1 < caseCount);
        if (fallsThrough) {
            addWarning(safeLine(caseNode), 0, "Switch case may fall through", NULL);
        }

        if (caseReachable && caseFlow.stops) {
            /* stopping a case does not affect reachability of other cases */
        }
    }

    if (caseCount > 0 && chainFlows && caseFlows) {
        FlowResult nextChain = makeFlow(false, false);
        for (size_t idx = caseCount; idx-- > 0;) {
            FlowResult flow = caseFlows[idx];
            if (!flow.stops) {
                flow = nextChain;
            }
            chainFlows[idx] = flow;
            nextChain = flow;
        }
    }

    bool allReturn = true;
    bool anyReachable = ctx.reachable && caseCount > 0;
    if (!anyReachable) {
        allReturn = false;
    } else {
        for (size_t i = 0; i < caseCount; ++i) {
            if (!chainFlows[i].returns) {
                allReturn = false;
                break;
            }
        }
    }

    result.stops = sawDefault && ctx.reachable && anyReachable && allReturn;
    result.returns = sawDefault && ctx.reachable && anyReachable && allReturn;
    free(caseFlows);
    free(isDefault);
    free(chainFlows);
    return result;
}

static FlowResult checkIf(ASTNode* node, Scope* scope, FlowContext ctx) {
    FlowResult thenFlow = checkStatement(node->ifStmt.thenBranch, scope, ctx);
    FlowResult elseFlow = makeFlow(false, false);
    if (node->ifStmt.elseBranch) {
        elseFlow = checkStatement(node->ifStmt.elseBranch, scope, ctx);
    }
    bool returns = thenFlow.returns && elseFlow.returns;
    bool stops = thenFlow.stops && elseFlow.stops;
    return makeFlow(returns, stops);
}

static FlowResult checkStatementList(ASTNode** list, size_t count, Scope* scope, FlowContext ctx) {
    bool reachable = ctx.reachable;
    bool sawReturn = false;
    bool reportedDeadRegion = false;

    for (size_t i = 0; i < count; ++i) {
        ASTNode* stmt = list ? list[i] : NULL;

        if (!reachable && stmt && stmt->type == AST_LABEL_DECLARATION &&
            stmt->label.labelName &&
            gotoTargetSetContains(ctx.gotoTargets, stmt->label.labelName)) {
            reachable = true;
            reportedDeadRegion = false;
        }

        FlowContext stmtCtx = ctx;
        stmtCtx.reachable = reachable;
        stmtCtx.suppressUnreachable = !reachable;

        if (stmt && stmt->line > 0 && !reachable && !ctx.suppressUnreachable && !reportedDeadRegion) {
            reportUnreachable(stmt);
            reportedDeadRegion = true;
        }

        FlowResult flow = checkStatement(stmt, scope, stmtCtx);

        if (reachable && flow.stops) {
            reachable = false;
            reportedDeadRegion = false;
            if (flow.returns) {
                sawReturn = true;
            }
        }
    }

    FlowResult result;
    result.stops = !reachable;
    result.returns = result.stops && sawReturn;
    return result;
}

static FlowResult checkStatement(ASTNode* node, Scope* scope, FlowContext ctx) {
    if (!node) return makeFlow(false, false);

    switch (node->type) {
        case AST_BLOCK:
            return checkBlock(node, scope, ctx);

        case AST_IF_STATEMENT:
            return checkIf(node, scope, ctx);

        case AST_WHILE_LOOP: {
            FlowContext loopCtx = ctx;
            loopCtx.inLoop = true;
            (void)checkStatement(node->whileLoop.condition, scope, ctx);
            (void)checkStatement(node->whileLoop.body, scope, loopCtx);
            return makeFlow(false, false);
        }

        case AST_FOR_LOOP: {
            FlowContext loopCtx = ctx;
            loopCtx.inLoop = true;
            (void)checkStatement(node->forLoop.initializer, scope, ctx);
            (void)checkStatement(node->forLoop.condition, scope, ctx);
            (void)checkStatement(node->forLoop.increment, scope, ctx);
            FlowResult bodyFlow = checkStatement(node->forLoop.body, scope, loopCtx);
            if (!node->forLoop.condition && bodyFlow.returns) {
                // `for (;;)` with a body that returns on every path does not fall through.
                return makeFlow(true, true);
            }
            return makeFlow(false, false);
        }

        case AST_SWITCH:
            return checkSwitch(node, scope, ctx);

        case AST_CASE:
            return checkCaseBody(node, scope, ctx);

        case AST_RETURN:
            return makeFlow(true, true);

        case AST_BREAK:
        case AST_CONTINUE:
        case AST_GOTO_STATEMENT:
            return makeFlow(false, true);

        case AST_LABEL_DECLARATION:
            return checkStatement(node->label.statement, scope, ctx);

        default:
            return makeFlow(false, false);
    }
}

static void traverse(ASTNode* node, Scope* scope) {
    if (!node) return;
    switch (node->type) {
        case AST_PROGRAM:
            for (size_t i = 0; i < node->block.statementCount; ++i) {
                traverse(node->block.statements[i], scope);
            }
            break;
        case AST_FUNCTION_DEFINITION: {
            GotoTargetSet gotoTargets = {0};
            collectGotoTargets(node->functionDef.body, &gotoTargets);

            FlowContext ctx = {
                .inLoop = false,
                .inSwitch = false,
                .reachable = true,
                .suppressUnreachable = false,
                .gotoTargets = &gotoTargets
            };
            FlowResult flow = checkStatement(node->functionDef.body, scope, ctx);

            TypeInfo retInfo = typeInfoFromParsedType(&node->functionDef.returnType, scope);
            if (retInfo.category != TYPEINFO_VOID && !flow.returns) {
                const char* name = node->functionDef.funcName && node->functionDef.funcName->valueNode.value
                    ? node->functionDef.funcName->valueNode.value
                    : "<anonymous>";
                char buffer[160];
                snprintf(buffer, sizeof(buffer), "Control reaches end of non-void function '%s'", name);
                addError(safeLine(node), 0, buffer, NULL);
            }
            gotoTargetSetFree(&gotoTargets);
            break;
        }
        default:
            break;
    }
}

void analyzeControlFlow(ASTNode* root, Scope* scope) {
    if (!root || !scope) return;
    traverse(root, scope);
}
