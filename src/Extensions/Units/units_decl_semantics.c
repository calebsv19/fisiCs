// SPDX-License-Identifier: Apache-2.0

#include "Extensions/Units/units_decl_semantics.h"

#include <stdlib.h>

#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Extensions/extension_diagnostics.h"
#include "Extensions/Units/units_parser.h"
#include "Extensions/Units/units_registry.h"

bool fisics_units_resolve_annotation(CompilerContext* ctx,
                                     const ASTNode* node,
                                     const char* exprText,
                                     bool overlayEnabled,
                                     FisicsDim8* outDim,
                                     char** outCanonical) {
    if (outCanonical) *outCanonical = NULL;
    if (!overlayEnabled) {
        fisics_extension_diag_units_disabled(ctx, node, exprText);
        return false;
    }

    char* detail = NULL;
    FisicsDim8 dim = fisics_dim_zero();
    if (!fisics_units_parse_dim_expr(exprText, &dim, &detail)) {
        fisics_extension_diag_invalid_units_expr(ctx, node, exprText, detail ? detail : "invalid dimension syntax");
        free(detail);
        return false;
    }
    free(detail);

    if (outDim) {
        *outDim = dim;
    }
    if (outCanonical) {
        *outCanonical = fisics_dim_to_string(dim);
    }
    return true;
}

bool fisics_units_resolve_unit_annotation(CompilerContext* ctx,
                                          const ASTNode* node,
                                          const char* unitText,
                                          bool overlayEnabled,
                                          bool hasResolvedDim,
                                          FisicsDim8 declDim,
                                          const FisicsUnitDef** outUnitDef) {
    if (outUnitDef) *outUnitDef = NULL;
    if (!overlayEnabled) {
        fisics_extension_diag_units_disabled(ctx, node, unitText);
        return false;
    }
    if (!hasResolvedDim) {
        fisics_extension_diag_unit_requires_dim(ctx, node, unitText);
        return false;
    }

    const FisicsUnitDef* unit = NULL;
    if (!fisics_unit_lookup(unitText, &unit) || !unit) {
        fisics_extension_diag_invalid_unit_attr(ctx,
                                                node,
                                                unitText,
                                                "unknown concrete unit; Phase 7 currently accepts only seeded registry entries");
        return false;
    }
    if (!fisics_dim_equal(unit->dim, declDim)) {
        fisics_extension_diag_unit_dim_mismatch(ctx, node, unitText, declDim, unit->dim);
        return false;
    }

    if (outUnitDef) {
        *outUnitDef = unit;
    }
    return true;
}
