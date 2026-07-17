// SPDX-License-Identifier: Apache-2.0

#include "analyze_stmt.h"
#include "analyze_core.h"
#include "analyze_decls.h"
#include "analyze_expr.h"
#include "const_eval.h"
#include "syntax_errors.h"
#include "type_checker.h"
#include "symbol_table.h"
#include "Lexer/tokens.h"
#include "Utils/profiler.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint64_t* values;
    SourceRange* locations;
    size_t count;
    size_t capacity;
    bool hasDefault;
    SourceRange defaultLoc;
    unsigned switchBits;
    bool switchIsUnsigned;
    bool hasSwitchType;
    unsigned originalSwitchBits;
    bool originalSwitchIsUnsigned;
    bool hasOriginalSwitchType;
    Scope* scope;
} SwitchFrame;

#define SWITCH_STACK_MAX 32

typedef struct {
    SwitchFrame frames[SWITCH_STACK_MAX];
    int depth;
} SwitchStack;

static SourceRange expressionCoveringRange(ASTNode* expr) {
    if (!expr) return (SourceRange){0};
    SourceRange range = expr->location;
    ASTNode* children[3] = {0};
    size_t childCount = 0;
    if (expr->type == AST_BINARY_EXPRESSION) {
        children[childCount++] = expr->expr.left;
        children[childCount++] = expr->expr.right;
    } else if (expr->type == AST_TERNARY_EXPRESSION) {
        children[childCount++] = expr->ternaryExpr.condition;
        children[childCount++] = expr->ternaryExpr.trueExpr;
        children[childCount++] = expr->ternaryExpr.falseExpr;
    } else if (expr->type == AST_UNARY_EXPRESSION ||
               expr->type == AST_CAST_EXPRESSION) {
        children[childCount++] = expr->type == AST_CAST_EXPRESSION
                                     ? expr->castExpr.expression
                                     : expr->expr.left;
    }

    for (size_t i = 0; i < childCount; ++i) {
        SourceRange child = expressionCoveringRange(children[i]);
        if (!child.start.file) continue;
        if (!range.start.file) {
            range = child;
            continue;
        }
        if (strcmp(range.start.file, child.start.file) != 0 ||
            range.start.line != child.start.line ||
            range.end.line != child.end.line) {
            continue;
        }
        if (child.start.column < range.start.column) range.start = child.start;
        if (child.end.column > range.end.column) range.end = child.end;
    }
    return range;
}

static bool expressionContainsVlaSizeof(ASTNode* expr, Scope* scope) {
    if (!expr || !scope) return false;
    if (expr->type == AST_SIZEOF && expr->expr.left) {
        ASTNode* operand = expr->expr.left;
        if (operand->type == AST_PARSED_TYPE) {
            return parsedTypeHasVLA(&operand->parsedTypeNode.parsed);
        }
        if (operand->type == AST_IDENTIFIER && operand->valueNode.value) {
            Symbol* sym = resolveInScopeChain(scope, operand->valueNode.value);
            return sym && parsedTypeHasVLA(&sym->type);
        }
        TypeInfo info = analyzeExpression(operand, scope);
        return info.isVLA ||
               (info.originalType && parsedTypeHasVLA(info.originalType));
    }
    if (expr->type == AST_BINARY_EXPRESSION) {
        return expressionContainsVlaSizeof(expr->expr.left, scope) ||
               expressionContainsVlaSizeof(expr->expr.right, scope);
    }
    if (expr->type == AST_TERNARY_EXPRESSION) {
        return expressionContainsVlaSizeof(expr->ternaryExpr.condition, scope) ||
               expressionContainsVlaSizeof(expr->ternaryExpr.trueExpr, scope) ||
               expressionContainsVlaSizeof(expr->ternaryExpr.falseExpr, scope);
    }
    if (expr->type == AST_UNARY_EXPRESSION) {
        return expressionContainsVlaSizeof(expr->expr.left, scope);
    }
    if (expr->type == AST_CAST_EXPRESSION) {
        return expressionContainsVlaSizeof(expr->castExpr.expression, scope);
    }
    return false;
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
    frame->switchBits = 0;
    frame->switchIsUnsigned = false;
    frame->hasSwitchType = false;
    frame->originalSwitchBits = 0;
    frame->originalSwitchIsUnsigned = false;
    frame->hasOriginalSwitchType = false;
    frame->scope = NULL;
}

static SwitchFrame* pushSwitchFrame(SwitchStack* stack, Scope* scope) {
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
    frame->switchBits = 0;
    frame->switchIsUnsigned = false;
    frame->hasSwitchType = false;
    frame->originalSwitchBits = 0;
    frame->originalSwitchIsUnsigned = false;
    frame->hasOriginalSwitchType = false;
    frame->scope = scope;
    return frame;
}

static const char* variablyModifiedNameBetweenScopes(Scope* scope, Scope* stopScope) {
    for (Scope* current = scope; current && current != stopScope; current = current->parent) {
        for (size_t bucket = 0; bucket < SYMBOL_TABLE_SIZE; ++bucket) {
            for (Symbol* sym = current->table.buckets[bucket]; sym; sym = sym->next) {
                if ((sym->kind == SYMBOL_VARIABLE || sym->kind == SYMBOL_TYPEDEF) &&
                    (sym->type.isVLA || parsedTypeHasVLA(&sym->type))) {
                    return sym->name;
                }
            }
        }
    }
    return NULL;
}

static void popSwitchFrame(SwitchStack* stack) {
    if (!stack || stack->depth <= 0) return;
    stack->depth--;
    switchFrameFree(&stack->frames[stack->depth]);
}

static uint64_t normalizeSwitchCaseValue(ConstEvalResult value,
                                         unsigned switchBits,
                                         bool switchIsUnsigned) {
    unsigned bits = switchBits ? switchBits : (value.bitWidth ? value.bitWidth : 64);
    if (bits >= 64) {
        return (uint64_t)value.value;
    }
    uint64_t mask = (1ULL << bits) - 1ULL;
    uint64_t normalized = ((uint64_t)value.value) & mask;
    if (!switchIsUnsigned && bits > 0) {
        uint64_t sign = 1ULL << (bits - 1);
        if (normalized & sign) {
            normalized |= ~mask;
        }
    }
    return normalized;
}

static long long convertSwitchCaseValue(ConstEvalResult value,
                                        unsigned switchBits,
                                        bool switchIsUnsigned) {
    return (long long)normalizeSwitchCaseValue(value, switchBits, switchIsUnsigned);
}

static void formatSwitchCaseValue(char* buffer,
                                  size_t bufferSize,
                                  uint64_t value,
                                  bool isUnsigned) {
    if (!buffer || bufferSize == 0) return;
    if (isUnsigned) {
        snprintf(buffer, bufferSize, "%llu", (unsigned long long)value);
    } else {
        snprintf(buffer, bufferSize, "%lld", (long long)value);
    }
}

static bool switchFrameRecordValue(SwitchFrame* frame, uint64_t value, SourceRange loc) {
    if (!frame) return false;
    for (size_t i = 0; i < frame->count; ++i) {
        if (frame->values[i] == value) {
            return false;
        }
    }
    if (frame->count == frame->capacity) {
        size_t newCap = frame->capacity == 0 ? 8 : frame->capacity * 2;
        uint64_t* newVals = realloc(frame->values, newCap * sizeof(uint64_t));
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
        case AST_FUNCTION_DECLARATION:
        case AST_STRUCT_DEFINITION:
        case AST_UNION_DEFINITION:
        case AST_ENUM_DEFINITION:
        case AST_TYPEDEF:
        case AST_STATIC_ASSERT:
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

static TypeInfo switchOriginalConditionType(ASTNode* expr, Scope* scope) {
    if (!expr) {
        return makeInvalidType();
    }
    if (expr->type == AST_IDENTIFIER && expr->valueNode.value && scope) {
        Symbol* sym = resolveInScopeChain(scope, expr->valueNode.value);
        if (sym) {
            TypeInfo info = typeInfoFromSymbolCached(sym, scope);
            if (info.category != TYPEINFO_INVALID) {
                return info;
            }
        }
    }
    return analyzeExpression(expr, scope);
}

static void analyzeControlCondition(ASTNode* expr,
                                    ASTNode* ownerStmt,
                                    Scope* scope,
                                    bool requireInteger,
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
            SourceRange loc = expr->location;
            SourceRange callSite = expr->macroCallSite;
            SourceRange macroDef = expr->macroDefinition;
            if (ownerStmt) {
                if (!loc.start.file) {
                    loc = ownerStmt->location;
                }
                if (!callSite.start.file) {
                    callSite = ownerStmt->macroCallSite;
                }
                if (!macroDef.start.file) {
                    macroDef = ownerStmt->macroDefinition;
                }
            }
            loc.start.column = 0;
            loc.end.column = 0;
            snprintf(buffer, sizeof(buffer), "%s controlling expression must be integer", stmtName);
            addErrorWithRanges(loc, callSite, macroDef, buffer, NULL);
        }
        return;
    }
    if (!typeInfoIsScalar(&cond)) {
        char buffer[128];
        SourceRange loc = expr->location;
        SourceRange callSite = expr->macroCallSite;
        SourceRange macroDef = expr->macroDefinition;
        if (ownerStmt) {
            if (!loc.start.file) {
                loc = ownerStmt->location;
            }
            if (!callSite.start.file) {
                callSite = ownerStmt->macroCallSite;
            }
            if (!macroDef.start.file) {
                macroDef = ownerStmt->macroDefinition;
            }
        }
        loc.start.column = 0;
        loc.end.column = 0;
        snprintf(buffer, sizeof(buffer), "%s controlling expression must be scalar", stmtName);
        addErrorWithRanges(loc, callSite, macroDef, buffer, NULL);
    }
}

static void analyzeStatementInternal(ASTNode* node,
                                     Scope* scope,
                                     SwitchStack* switchStack,
                                     LabelTracker* labels,
                                     int loopDepth) {
    switch (node->type) {
        case AST_IF_STATEMENT:
            analyzeControlCondition(node->ifStmt.condition, node, scope, false, "if");
            analyzeStatementInternal(node->ifStmt.thenBranch, scope, switchStack, labels, loopDepth);
            if (node->ifStmt.elseBranch) {
                analyzeStatementInternal(node->ifStmt.elseBranch, scope, switchStack, labels, loopDepth);
            }
            break;

        case AST_FOR_LOOP: {
            Scope* inner = createScope(scope);
            analyze(node->forLoop.initializer, inner);
            analyzeControlCondition(node->forLoop.condition, node, inner, false, "for");
            analyze(node->forLoop.increment, inner);
            analyzeStatementInternal(node->forLoop.body, inner, switchStack, labels, loopDepth + 1);
            destroyScope(inner);
            break;
        }

        case AST_WHILE_LOOP:
            analyzeControlCondition(node->whileLoop.condition,
                                    node,
                                    scope,
                                    false,
                                    node->whileLoop.isDoWhile ? "do-while" : "while");
            analyzeStatementInternal(node->whileLoop.body, scope, switchStack, labels, loopDepth + 1);
            break;

        case AST_RETURN:
            if (node->returnStmt.returnValue) {
                TypeInfo retVal = analyzeExpression(node->returnStmt.returnValue, scope);
                retVal = decayToRValue(retVal);
                if (scope && scope->hasReturnType) {
                    if (scope->returnType.category == TYPEINFO_VOID) {
                        addErrorWithRanges(node->location,
                                           node->macroCallSite,
                                           node->macroDefinition,
                                           "Void function should not return a value",
                                           NULL);
                    } else if (retVal.category != TYPEINFO_INVALID) {
                        AssignmentCheckResult res = canAssignTypesInScope(&scope->returnType, &retVal, scope);
                        if (res == ASSIGN_INCOMPATIBLE &&
                            typeInfoIsPointerLike(&scope->returnType) &&
                            typeInfoIsInteger(&retVal)) {
                            long long zero = 1;
                            if (constEvalInteger(node->returnStmt.returnValue, scope, &zero, true) && zero == 0) {
                                res = ASSIGN_OK;
                            }
                        }
                        if (res == ASSIGN_INCOMPATIBLE) {
                            bool allowFunctionDecay = isFunctionDesignatorExpr(node->returnStmt.returnValue, scope) &&
                                                      (typeInfoIsPointerLike(&scope->returnType) ||
                                                       scope->returnType.category == TYPEINFO_FUNCTION ||
                                                       scope->returnType.isFunction ||
                                                       scope->returnType.pointerDepth > 0);
                            if (!allowFunctionDecay) {
                                addErrorWithRanges(node->location,
                                                   node->macroCallSite,
                                                   node->macroDefinition,
                                                   "Incompatible return type",
                                                   NULL);
                            }
                        } else if (res == ASSIGN_QUALIFIER_LOSS) {
                            addErrorWithRanges(node->location,
                                               node->macroCallSite,
                                               node->macroDefinition,
                                               "Return discards qualifiers from pointer target",
                                               NULL);
                        }
                    }
                }
            } else {
                if (scope && scope->hasReturnType && scope->returnType.category != TYPEINFO_VOID) {
                    addErrorWithRanges(node->location,
                                       node->macroCallSite,
                                       node->macroDefinition,
                                       "Non-void function must return a value",
                                       NULL);
                }
            }
            break;

        case AST_BREAK:
            if (loopDepth == 0 && (!switchStack || switchStack->depth == 0)) {
                addErrorWithRanges(node->location,
                                   node->macroCallSite,
                                   node->macroDefinition,
                                   "Break statement not within loop or switch",
                                   NULL);
            }
            break;
        case AST_CONTINUE:
            if (loopDepth == 0) {
                addErrorWithRanges(node->location,
                                   node->macroCallSite,
                                   node->macroDefinition,
                                   "Continue statement not within a loop",
                                   NULL);
            }
            break;

        case AST_SWITCH: {
            analyzeControlCondition(node->switchStmt.condition, node, scope, true, "switch");
            SwitchFrame* frame = pushSwitchFrame(switchStack, scope);
            if (frame && node->switchStmt.condition) {
                TypeInfo originalSwitchType = switchOriginalConditionType(node->switchStmt.condition, scope);
                if (typeInfoIsInteger(&originalSwitchType)) {
                    frame->originalSwitchBits = originalSwitchType.bitWidth ? originalSwitchType.bitWidth : 64;
                    frame->originalSwitchIsUnsigned = !originalSwitchType.isSigned;
                    frame->hasOriginalSwitchType = true;
                }
                TypeInfo switchType = analyzeExpression(node->switchStmt.condition, scope);
                switchType = decayToRValue(switchType);
                if (typeInfoIsInteger(&switchType)) {
                    switchType = integerPromote(switchType);
                    frame->switchBits = switchType.bitWidth ? switchType.bitWidth : 64;
                    frame->switchIsUnsigned = !switchType.isSigned;
                    frame->hasSwitchType = true;
                }
            }
            for (size_t i = 0; i < node->switchStmt.caseListSize; i++) {
                analyzeStatementInternal(node->switchStmt.caseList[i], scope, switchStack, labels, loopDepth);
            }
            popSwitchFrame(switchStack);
            (void)frame;
            break;
        }

        case AST_CASE:
            if (switchStack && switchStack->depth > 0) {
                SwitchFrame* frame = &switchStack->frames[switchStack->depth - 1];
                const char* bypassed = variablyModifiedNameBetweenScopes(scope, frame->scope);
                if (bypassed) {
                    char hint[256];
                    if (node->caseStmt.caseValue) {
                        ConstEvalResult caseValue =
                            constEvalIntegerResult(node->caseStmt.caseValue, scope, true);
                        if (caseValue.isConst) {
                            snprintf(hint,
                                     sizeof(hint),
                                     "case %lld bypasses %s",
                                     caseValue.value,
                                     bypassed);
                        } else {
                            snprintf(hint, sizeof(hint), "case label bypasses %s", bypassed);
                        }
                    } else {
                        snprintf(hint, sizeof(hint), "default label bypasses %s", bypassed);
                    }
                    addErrorWithRanges(node->location,
                                       node->macroCallSite,
                                       node->macroDefinition,
                                       "switch dispatch jumps into scope of variably modified declaration",
                                       hint);
                }
            }
            if (node->caseStmt.caseBodySize > 0 &&
                node->caseStmt.caseBody &&
                node->caseStmt.caseBody[0] &&
                isDeclarationStatementType(node->caseStmt.caseBody[0]->type)) {
                addErrorWithRanges(node->location,
                                   node->macroCallSite,
                                   node->macroDefinition,
                                   "label before declaration is not allowed in C99; wrap declaration in a block",
                                   NULL);
            }
            if (node->caseStmt.caseValue) {
                node->caseStmt.hasAnalyzedConstValue = false;
                analyze(node->caseStmt.caseValue, scope);
                ConstEvalResult res = constEvalIntegerResult(node->caseStmt.caseValue, scope, true);
                if (!res.isConst) {
                    SourceRange caseRange =
                        expressionContainsVlaSizeof(node->caseStmt.caseValue, scope)
                            ? expressionCoveringRange(node->caseStmt.caseValue)
                            : node->caseStmt.caseValue->location;
                    addErrorWithRanges(caseRange,
                                       node->caseStmt.caseValue->macroCallSite,
                                       node->caseStmt.caseValue->macroDefinition,
                                       "Case label is not an integer constant expression",
                                       NULL);
                } else if (switchStack && switchStack->depth > 0) {
                    node->caseStmt.analyzedConstValue = res.value;
                    node->caseStmt.hasAnalyzedConstValue = true;
                    SwitchFrame* frame = &switchStack->frames[switchStack->depth - 1];
                    if (frame->hasOriginalSwitchType) {
                        long long converted =
                            convertSwitchCaseValue(res,
                                                   frame->originalSwitchBits,
                                                   frame->originalSwitchIsUnsigned);
                        if (converted != res.value) {
                            char originalValue[64];
                            char convertedValue[64];
                            char warningBuffer[160];
                            formatSwitchCaseValue(originalValue,
                                                  sizeof(originalValue),
                                                  (uint64_t)res.value,
                                                  res.isUnsigned);
                            formatSwitchCaseValue(convertedValue,
                                                  sizeof(convertedValue),
                                                  (uint64_t)converted,
                                                  frame->originalSwitchIsUnsigned);
                            snprintf(warningBuffer,
                                     sizeof(warningBuffer),
                                     "overflow converting case value to switch condition type (%s to %s)",
                                     originalValue,
                                     convertedValue);
                            addWarningWithRanges(node->caseStmt.caseValue->location,
                                                 node->caseStmt.caseValue->macroCallSite,
                                                 node->caseStmt.caseValue->macroDefinition,
                                                 warningBuffer,
                                                 NULL);
                        }
                    }
                    uint64_t normalized =
                        normalizeSwitchCaseValue(res,
                                                 frame->hasSwitchType ? frame->switchBits : 0,
                                                 frame->hasSwitchType ? frame->switchIsUnsigned : res.isUnsigned);
                    if (!switchFrameRecordValue(frame, normalized, node->caseStmt.caseValue->location)) {
                        SourceRange prev = {0};
                        // find previous location to report
                        for (size_t i = 0; i < frame->count; ++i) {
                            if (frame->values[i] == normalized) {
                                prev = frame->locations[i];
                                break;
                            }
                        }
                        char buffer[128];
                        char valueBuffer[64];
                        formatSwitchCaseValue(valueBuffer,
                                              sizeof(valueBuffer),
                                              normalized,
                                              frame->hasSwitchType ? frame->switchIsUnsigned : res.isUnsigned);
                        snprintf(buffer,
                                 sizeof(buffer),
                                 "Duplicate case label with value %s",
                                 valueBuffer);
                        addErrorWithRanges(node->caseStmt.caseValue->location,
                                           node->caseStmt.caseValue->macroCallSite,
                                           node->caseStmt.caseValue->macroDefinition,
                                           buffer,
                                           NULL);
                        if (prev.start.line > 0) {
                            addWarningWithRanges(prev,
                                                 (SourceRange){0},
                                                 (SourceRange){0},
                                                 "Previous case label with same value here",
                                                 NULL);
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
                            SourceRange previousDefault = frame->defaultLoc;
                            previousDefault.start.column = 0;
                            previousDefault.end.column = 0;
                            addWarningWithRanges(previousDefault,
                                                 (SourceRange){0},
                                                 (SourceRange){0},
                                                 "Previous default label is here",
                                                 NULL);
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
                    addWarningWithRanges(node->location,
                                         node->macroCallSite,
                                         node->macroDefinition,
                                         buffer,
                                         NULL);
                    if (prev.start.line > 0) {
                        addWarningWithRanges(prev, (SourceRange){0}, (SourceRange){0}, "Previous label is here", NULL);
                    }
                }
            }
            if (node->label.statement) {
                if (isDeclarationStatementType(node->label.statement->type)) {
                    addErrorWithRanges(node->location,
                                       node->macroCallSite,
                                       node->macroDefinition,
                                       "label before declaration is not allowed in C99; wrap declaration in a block",
                                       NULL);
                }
                analyzeStatementInternal(node->label.statement, scope, switchStack, labels, loopDepth);
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
        case AST_FUNCTION_DECLARATION:
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
    ProfilerScope stmtScope = profiler_begin("semantic_analyze_statement");
    profiler_record_value("semantic_count_analyze_statement", 1);
    SwitchStack stack = {0};
    LabelTracker labels = {0};
    analyzeStatementInternal(node, scope, &stack, &labels, 0);
    free(labels.names);
    free(labels.locations);
    profiler_end(stmtScope);
}

void analyzeFunctionBody(ASTNode* node, Scope* scope) {
    if (!node || node->type != AST_BLOCK) {
        analyzeStatement(node, scope);
        return;
    }

    ProfilerScope stmtScope = profiler_begin("semantic_analyze_function_body");
    profiler_record_value("semantic_count_analyze_statement", 1);
    SwitchStack stack = {0};
    LabelTracker labels = {0};
    for (size_t i = 0; i < node->block.statementCount; ++i) {
        analyzeStatementInternal(node->block.statements[i],
                                 scope,
                                 &stack,
                                 &labels,
                                 0);
    }
    free(labels.names);
    free(labels.locations);
    profiler_end(stmtScope);
}
