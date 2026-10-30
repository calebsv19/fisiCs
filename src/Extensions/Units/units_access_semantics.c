// SPDX-License-Identifier: Apache-2.0
#include "Extensions/Units/units_access_semantics.h"

#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Extensions/extension_units_call_contracts.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/extension_units_expr_table.h"

void fisics_units_capture_access_type(CompilerContext* ctx, ASTNode* node,
                                      const ParsedType* type) {
    if (!cc_overlay_physics_units_enabled(ctx)) return;
    (void)fisics_extension_note_units_type_binding(ctx, node, type);
    if (node && node->type == AST_ARRAY_ACCESS)
        fisics_units_note_origin(ctx,node,
            fisics_units_lookup_origin(ctx,node->arrayAccess.array));
    if (node && node->type == AST_POINTER_DEREFERENCE)
        fisics_units_note_origin(ctx,node,
            fisics_units_lookup_origin(ctx,node->pointerDeref.pointer));
}

void fisics_units_record_access_result(CompilerContext* ctx, ASTNode* node) {
    const ParsedType* type = fisics_extension_lookup_units_symbol_type_binding(ctx, node);
    FisicsDim8 dim = fisics_dim_zero();
    const FisicsUnitDef* unit = NULL;
    bool resolved = false, unitResolved = false;
    if (!fisics_units_parse_type_contract(type, &dim, &resolved, &unit, &unitResolved) || !resolved)
        return;
    (void)fisics_extension_set_units_expr_result_with_unit(ctx, node, dim, true,
                                                         unit, unitResolved);
}
