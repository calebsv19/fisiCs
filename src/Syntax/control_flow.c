#include "control_flow.h"
#include "syntax_errors.h"
#include "type_checker.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    bool inLoop;
    bool inSwitch;
    bool reachable;
    bool suppressUnreachable;
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

    bool sawDefault = false;
    bool allReturn = true;
    bool allStop = true;
    bool anyReachable = false;
    for (size_t i = 0; i < node->switchStmt.caseListSize; ++i) {
        ASTNode* caseNode = node->switchStmt.caseList[i];
        if (!caseNode) continue;

        bool caseReachable = ctx.reachable;
        if (!caseReachable && !caseCtx.suppressUnreachable) {
            reportUnreachable(caseNode);
        }

        FlowContext thisCtx = caseCtx;
        thisCtx.reachable = caseReachable;
        thisCtx.suppressUnreachable = !caseReachable;

        FlowResult caseFlow = checkCaseBody(caseNode, scope, thisCtx);

        if (!caseNode->caseStmt.caseValue) {
            sawDefault = true;
        }

        if (caseReachable) {
            anyReachable = true;
            allReturn = allReturn && caseFlow.returns;
            allStop = allStop && caseFlow.stops;
        }

        bool fallsThrough = caseReachable && !caseFlow.stops && (i + 1 < node->switchStmt.caseListSize);
        if (fallsThrough) {
            addWarning(safeLine(caseNode), 0, "Switch case may fall through", NULL);
        }

        if (caseReachable && caseFlow.stops) {
            /* stopping a case does not affect reachability of other cases */
        }
    }

    result.stops = sawDefault && ctx.reachable && anyReachable && allStop;
    result.returns = sawDefault && ctx.reachable && anyReachable && allReturn;
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

    for (size_t i = 0; i < count; ++i) {
        ASTNode* stmt = list ? list[i] : NULL;
        FlowContext stmtCtx = ctx;
        stmtCtx.reachable = reachable;
        stmtCtx.suppressUnreachable = !reachable;

        if (stmt && stmt->line > 0 && !reachable && !ctx.suppressUnreachable) {
            reportUnreachable(stmt);
        }

        FlowResult flow = checkStatement(stmt, scope, stmtCtx);

        if (reachable && flow.stops) {
            reachable = false;
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
            (void)checkStatement(node->forLoop.body, scope, loopCtx);
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
            FlowContext ctx = { .inLoop = false, .inSwitch = false, .reachable = true, .suppressUnreachable = false };
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
