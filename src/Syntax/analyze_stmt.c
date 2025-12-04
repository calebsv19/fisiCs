#include "analyze_stmt.h"
#include "analyze_core.h"
#include "analyze_decls.h"
#include "analyze_expr.h"
#include "const_eval.h"
#include "syntax_errors.h"
#include "type_checker.h"
#include "Lexer/tokens.h"
#include <stdlib.h>

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

static void analyzeStatementInternal(ASTNode* node, Scope* scope, SwitchStack* switchStack) {
    switch (node->type) {
        case AST_IF_STATEMENT:
            analyze(node->ifStmt.condition, scope);
            analyzeStatementInternal(node->ifStmt.thenBranch, scope, switchStack);
            if (node->ifStmt.elseBranch) {
                analyzeStatementInternal(node->ifStmt.elseBranch, scope, switchStack);
            }
            break;

        case AST_FOR_LOOP: {
            Scope* inner = createScope(scope);
            analyze(node->forLoop.initializer, inner);
            analyze(node->forLoop.condition, inner);
            analyze(node->forLoop.increment, inner);
            analyzeStatementInternal(node->forLoop.body, inner, switchStack);
            destroyScope(inner);
            break;
        }

        case AST_WHILE_LOOP:
            analyze(node->whileLoop.condition, scope);
            analyzeStatementInternal(node->whileLoop.body, scope, switchStack);
            break;

        case AST_RETURN:
            if (node->returnStmt.returnValue) {
                TypeInfo retVal = analyzeExpression(node->returnStmt.returnValue, scope);
                retVal = decayToRValue(retVal);
                if (scope && scope->hasReturnType) {
                    AssignmentCheckResult res = canAssignTypes(&scope->returnType, &retVal);
                    if (res == ASSIGN_QUALIFIER_LOSS) {
                        addError(node->line, 0, "Return discards qualifiers from pointer target", NULL);
                    }
                }
            } else {
                if (scope && scope->hasReturnType && scope->returnType.category != TYPEINFO_VOID) {
                    addError(node->line, 0, "Non-void function must return a value", NULL);
                }
            }
            break;

        case AST_BREAK:
        case AST_CONTINUE:
            // Could track enclosing loop for validation
            break;

        case AST_SWITCH: {
            analyze(node->switchStmt.condition, scope);
            SwitchFrame* frame = pushSwitchFrame(switchStack);
            for (size_t i = 0; i < node->switchStmt.caseListSize; i++) {
                analyzeStatementInternal(node->switchStmt.caseList[i], scope, switchStack);
            }
            popSwitchFrame(switchStack);
            (void)frame;
            break;
        }

        case AST_CASE:
            if (node->caseStmt.caseValue) {
                analyze(node->caseStmt.caseValue, scope);
                ConstEvalResult res = constEval(node->caseStmt.caseValue, scope, true);
                if (!res.isConst) {
                    addError(node->caseStmt.caseValue->line, 0, "Case label is not a constant expression", NULL);
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
                analyzeStatementInternal(node->caseStmt.caseBody[i], scope, switchStack);
            }
            if (node->caseStmt.nextCase) {
                analyzeStatementInternal(node->caseStmt.nextCase, scope, switchStack);
            }
            break;

        case AST_LABEL_DECLARATION:
            if (node->label.statement) {
                analyze(node->label.statement, scope);
            }
            break;

        case AST_GOTO_STATEMENT:
            // Goto/label validation not implemented yet
            break;

        case AST_BLOCK:
            for (size_t i = 0; i < node->block.statementCount; ++i) {
                analyzeStatementInternal(node->block.statements[i], scope, switchStack);
            }
            break;

        case AST_VARIABLE_DECLARATION:
        case AST_STRUCT_DEFINITION:
        case AST_UNION_DEFINITION:
        case AST_ENUM_DEFINITION:
        case AST_TYPEDEF:
            analyzeDeclaration(node, scope);
            break;

        case AST_ASSIGNMENT:
        case AST_BINARY_EXPRESSION:
        case AST_UNARY_EXPRESSION:
        case AST_FUNCTION_CALL:
        case AST_COMPOUND_LITERAL:
        case AST_CAST_EXPRESSION:
            (void)analyzeExpression(node, scope);
            break;

        default:
            addError(node ? node->line : 0, 0, "Unhandled statement node", "No analysis implemented for this statement type");
            break;
    }
}

void analyzeStatement(ASTNode* node, Scope* scope) {
    SwitchStack stack = {0};
    analyzeStatementInternal(node, scope, &stack);
}
