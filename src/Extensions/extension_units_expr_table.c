// SPDX-License-Identifier: Apache-2.0

#include "Extensions/extension_units_expr_table.h"

#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE* effective_out(FILE* out) {
    return out ? out : stdout;
}

static bool units_expr_result_reserve(FisicsExtensionState* state, size_t extra) {
    if (!state) return false;
    size_t need = state->unitsExprResultCount + extra;
    if (need <= state->unitsExprResultCapacity) return true;
    size_t newCap = state->unitsExprResultCapacity ? state->unitsExprResultCapacity * 2 : 8;
    while (newCap < need) newCap *= 2;
    FisicsUnitsExprResult* grown =
        (FisicsUnitsExprResult*)realloc(state->unitsExprResults,
                                        newCap * sizeof(FisicsUnitsExprResult));
    if (!grown) return false;
    state->unitsExprResults = grown;
    state->unitsExprResultCapacity = newCap;
    return true;
}

static const char* ast_type_name(ASTNodeType type) {
    switch (type) {
        case AST_NUMBER_LITERAL: return "number-literal";
        case AST_CHAR_LITERAL: return "char-literal";
        case AST_IDENTIFIER: return "identifier";
        case AST_CAST_EXPRESSION: return "cast-expression";
        case AST_COMMA_EXPRESSION: return "comma-expression";
        case AST_ASSIGNMENT: return "assignment";
        case AST_BINARY_EXPRESSION: return "binary-expression";
        case AST_UNARY_EXPRESSION: return "unary-expression";
        case AST_TERNARY_EXPRESSION: return "ternary-expression";
        case AST_FUNCTION_CALL: return "function-call";
        default: return "expr";
    }
}

static void describe_expr_node(const ASTNode* node, char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) return;
    buffer[0] = '\0';
    if (!node) {
        snprintf(buffer, bufferSize, "expr");
        return;
    }

    switch (node->type) {
        case AST_IDENTIFIER:
            if (node->valueNode.value && node->valueNode.value[0] != '\0') {
                snprintf(buffer, bufferSize, "identifier(%s)", node->valueNode.value);
                return;
            }
            break;
        case AST_BINARY_EXPRESSION:
            if (node->expr.op && node->expr.op[0] != '\0') {
                snprintf(buffer, bufferSize, "binary-expression(%s)", node->expr.op);
                return;
            }
            break;
        case AST_ASSIGNMENT:
            if (node->assignment.op && node->assignment.op[0] != '\0') {
                snprintf(buffer, bufferSize, "assignment(%s)", node->assignment.op);
                return;
            }
            break;
        case AST_UNARY_EXPRESSION:
            if (node->expr.op && node->expr.op[0] != '\0') {
                snprintf(buffer, bufferSize, "unary-expression(%s)", node->expr.op);
                return;
            }
            break;
        case AST_TERNARY_EXPRESSION:
            snprintf(buffer, bufferSize, "ternary-expression");
            return;
        case AST_FUNCTION_CALL:
            if (node->functionCall.callee &&
                node->functionCall.callee->type == AST_IDENTIFIER &&
                node->functionCall.callee->valueNode.value &&
                node->functionCall.callee->valueNode.value[0] != '\0') {
                snprintf(buffer,
                         bufferSize,
                         "function-call(%s)",
                         node->functionCall.callee->valueNode.value);
                return;
            }
            break;
        default:
            break;
    }

    snprintf(buffer, bufferSize, "%s", ast_type_name(node->type));
}

bool fisics_extension_set_units_expr_result(CompilerContext* ctx,
                                            const ASTNode* node,
                                            FisicsDim8 dim,
                                            bool resolved) {
    return fisics_extension_set_units_expr_result_with_unit(ctx, node, dim, resolved, NULL, false);
}

bool fisics_extension_set_units_expr_result_with_unit(CompilerContext* ctx,
                                                      const ASTNode* node,
                                                      FisicsDim8 dim,
                                                      bool resolved,
                                                      const FisicsUnitDef* unitDef,
                                                      bool unitResolved) {
    if (!ctx || !node) return false;
    if (!fisics_extension_state_ensure(ctx)) return false;
    FisicsExtensionState* state = ctx->extensionState;
    for (size_t i = 0; i < state->unitsExprResultCount; ++i) {
        FisicsUnitsExprResult* result = &state->unitsExprResults[i];
        if (result->node == node) {
            result->dim = dim;
            result->resolved = resolved;
            result->unitDef = unitDef;
            result->unitResolved = unitResolved && unitDef != NULL;
            return true;
        }
    }
    if (!units_expr_result_reserve(state, 1)) return false;
    FisicsUnitsExprResult* result = &state->unitsExprResults[state->unitsExprResultCount++];
    memset(result, 0, sizeof(*result));
    result->node = (ASTNode*)node;
    result->dim = dim;
    result->resolved = resolved;
    result->unitDef = unitDef;
    result->unitResolved = unitResolved && unitDef != NULL;
    return true;
}

void fisics_extension_clear_units_expr_results(CompilerContext* ctx) {
    if (!ctx || !ctx->extensionState) return;
    ctx->extensionState->unitsExprResultCount = 0;
}

const FisicsUnitsExprResult* fisics_extension_lookup_units_expr_result(
    const CompilerContext* ctx,
    const ASTNode* node) {
    if (!ctx || !ctx->extensionState || !node) return NULL;
    for (size_t i = 0; i < ctx->extensionState->unitsExprResultCount; ++i) {
        const FisicsUnitsExprResult* result = &ctx->extensionState->unitsExprResults[i];
        if (result->node == node) {
            return result;
        }
    }
    return NULL;
}

const FisicsUnitsExprResult* fisics_extension_units_expr_result_at(const CompilerContext* ctx,
                                                                   size_t index) {
    if (!ctx || !ctx->extensionState || index >= ctx->extensionState->unitsExprResultCount) {
        return NULL;
    }
    return &ctx->extensionState->unitsExprResults[index];
}

void fisics_extension_for_each_units_expr_result(const CompilerContext* ctx,
                                                 FisicsUnitsExprResultCallback callback,
                                                 void* userData) {
    if (!ctx || !ctx->extensionState || !callback) return;
    for (size_t i = 0; i < ctx->extensionState->unitsExprResultCount; ++i) {
        callback(&ctx->extensionState->unitsExprResults[i], userData);
    }
}

size_t fisics_extension_units_expr_result_count(const CompilerContext* ctx) {
    return (ctx && ctx->extensionState) ? ctx->extensionState->unitsExprResultCount : 0;
}

void fisics_extension_dump_units_expr_results(const CompilerContext* ctx, FILE* out) {
    FILE* stream = effective_out(out);
    size_t count = fisics_extension_units_expr_result_count(ctx);
    if (count == 0) {
        fprintf(stream, "Extension Units Expr Results: 0\n");
        return;
    }

    fprintf(stream, "Extension Units Expr Results: %zu\n", count);
    for (size_t i = 0; i < count; ++i) {
        const FisicsUnitsExprResult* result = fisics_extension_units_expr_result_at(ctx, i);
        if (!result || !result->node) continue;
        unsigned line = result->node->location.start.line
                            ? result->node->location.start.line
                            : (unsigned)result->node->line;
        char* dimText = fisics_dim_to_string(result->dim);
        char exprLabel[128];
        describe_expr_node(result->node, exprLabel, sizeof(exprLabel));
        fprintf(stream,
                "  - line %u: %s [dim=%s, %s",
                line,
                exprLabel,
                dimText ? dimText : "<oom>",
                result->resolved ? "resolved" : "pending");
        if (result->unitResolved && result->unitDef) {
            fprintf(stream, ", unit=%s", result->unitDef->name);
        }
        fprintf(stream, "]\n");
        free(dimText);
    }
}
