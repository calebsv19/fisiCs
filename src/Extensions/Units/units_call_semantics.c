// SPDX-License-Identifier: Apache-2.0
#include "Extensions/Units/units_call_semantics.h"

#include "AST/ast_node.h"
#include "Extensions/extension_diagnostics.h"
#include "Extensions/extension_units_call_contracts.h"
#include "Extensions/extension_units_expr_table.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/extension_units_view.h"
#include "Extensions/Units/units_literal_semantics.h"
#include "Syntax/scope.h"
#include "Syntax/symbol_table.h"

static ASTNode* parameter_at(const Symbol* symbol, size_t index) {
    if (!symbol || symbol->kind != SYMBOL_FUNCTION || !symbol->definition) return NULL;
    ASTNode* def = symbol->definition;
    if (def->type == AST_FUNCTION_DEFINITION && index < def->functionDef.paramCount)
        return def->functionDef.parameters ? def->functionDef.parameters[index] : NULL;
    if (def->type == AST_FUNCTION_DECLARATION && index < def->functionDecl.paramCount)
        return def->functionDecl.parameters ? def->functionDecl.parameters[index] : NULL;
    return NULL;
}

void fisics_units_validate_call_arguments(struct CompilerContext* ctx,
                                          ASTNode* call, Scope* rootScope) {
    if (!ctx || !call || call->type != AST_FUNCTION_CALL) return;
    ASTNode* callee = call->functionCall.callee;
    Symbol* symbol = rootScope && callee && callee->type == AST_IDENTIFIER && callee->valueNode.value
                         ? resolveInScopeChain(rootScope, callee->valueNode.value) : NULL;
    /* Do not apply a global function's fallback declaration to a shadowing
     * local pointer. The per-call contracts already describe its actual type. */
    const ParsedType* boundType = fisics_extension_lookup_units_symbol_type_binding(ctx, callee);
    if (boundType && (!boundType->directlyDeclaresFunction ||
                      boundType->pointerDepth > 0 || boundType->isFunctionPointer)) symbol = NULL;
    for (size_t i = 0; i < call->functionCall.argumentCount; ++i) {
        FisicsDim8 expected = fisics_dim_zero();
        const FisicsUnitDef* expectedUnit = NULL;
        const FisicsUnitsCallArgContract* contract =
            fisics_extension_lookup_units_call_arg_contract(ctx, call, i);
        if (contract && contract->resolved) {
            expected = contract->dim;
            if (contract->unitResolved) expectedUnit = contract->unitDef;
        } else {
            const FisicsUnitsAnnotation* ann =
                fisics_extension_lookup_units_annotation(ctx, parameter_at(symbol, i));
            if (!ann || !ann->resolved || ann->dimDuplicateCount > 1) continue;
            expected = ann->dim;
            if (ann->unitResolved) expectedUnit = ann->unitDef;
        }
        ASTNode* argument = call->functionCall.arguments ? call->functionCall.arguments[i] : NULL;
        /* Numeric constants at an authored parameter boundary have the same
         * contextual interpretation as declaration initializers. Do not rewrite
         * the argument's expression metadata or extend this to identifiers. */
        if (fisics_units_is_literal_value(argument)) continue;
        const FisicsUnitsExprResult* result = fisics_extension_lookup_units_expr_result(ctx, argument);
        if (!result || !result->resolved) continue;
        if (!fisics_dim_equal(expected, result->dim)) {
            fisics_extension_diag_units_argument_dim_mismatch(ctx, argument, expected, result->dim);
            continue;
        }
        if (expectedUnit && result->unitResolved && result->unitDef && expectedUnit != result->unitDef) {
            fisics_extension_diag_units_implicit_unit_conversion(ctx, argument,
                "argument", result->unitDef, expectedUnit);
        }
    }
}
