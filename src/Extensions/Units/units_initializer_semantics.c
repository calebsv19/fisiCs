// SPDX-License-Identifier: Apache-2.0
#include "Extensions/Units/units_initializer_semantics.h"

#include "AST/ast_node.h"
#include "Extensions/extension_diagnostics.h"
#include "Extensions/extension_units_expr_table.h"
#include "Extensions/extension_units_view.h"

#include "Extensions/Units/units_literal_semantics.h"

static void validate_initializer(struct CompilerContext* ctx, ASTNode* owner,
                                 ASTNode* expression, size_t declaratorIndex, unsigned depth, bool declarationOwned) {
    if (!ctx || !owner || !expression || depth > 64) return;
    const FisicsUnitsAnnotation* ann = fisics_extension_lookup_units_declarator_annotation(ctx, owner, declaratorIndex);
    if (!ann || !ann->resolved || ann->dimDuplicateCount > 1) return;

    /* Arrays (including nested braces and designated elements) inherit the
     * owning annotation for every scalar initializer; mixed record fields are
     * selected by the caller's existing record traversal instead. */
    if (expression->type == AST_COMPOUND_LITERAL) {
        for (size_t i = 0; i < expression->compoundLiteral.entryCount; ++i) {
            DesignatedInit* entry = expression->compoundLiteral.entries
                                       ? expression->compoundLiteral.entries[i] : NULL;
            if (entry) validate_initializer(ctx, owner, entry->expression, declaratorIndex, depth + 1, false);
        }
        return;
    }

    if (fisics_units_is_literal_value(expression)) {
        /* Preserve the existing side-table contract: only a direct number/char
         * initializer adopts its declaration's metadata. Wrapped constants and
         * record-field constants are accepted without rewriting their results. */
        if (declarationOwned && (expression->type == AST_NUMBER_LITERAL ||
                                  expression->type == AST_CHAR_LITERAL)) {
            (void)fisics_extension_set_units_expr_result_with_unit(ctx, expression,
                ann->dim, true, ann->unitResolved ? ann->unitDef : NULL, ann->unitResolved);
        }
        return;
    }
    const FisicsUnitsExprResult* result = fisics_extension_lookup_units_expr_result(ctx, expression);
    if (!result || !result->resolved) return;
    if (!fisics_dim_equal(result->dim, ann->dim)) {
        fisics_extension_diag_units_assign_dim_mismatch(ctx, expression, ann->dim, result->dim);
        return;
    }
    if (ann->unitResolved && ann->unitDef && result->unitResolved && result->unitDef &&
        ann->unitDef != result->unitDef) {
        fisics_extension_diag_units_implicit_unit_conversion(ctx, expression,
            "initializer", result->unitDef, ann->unitDef);
    }
}

void fisics_units_validate_initializer(struct CompilerContext* ctx, ASTNode* owner,
                                       ASTNode* expression, bool declarationOwned) {
    validate_initializer(ctx, owner, expression, 0, 0, declarationOwned);
}

void fisics_units_validate_declarator_initializer(struct CompilerContext* ctx, ASTNode* owner,
    size_t declaratorIndex, ASTNode* expression) {
    validate_initializer(ctx, owner, expression, declaratorIndex, 0, true);
}
