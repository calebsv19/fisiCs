// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "Extensions/extension_hooks.h"

struct ASTNode;
struct CompilerContext;

typedef void (*FisicsUnitsExprResultCallback)(const FisicsUnitsExprResult* result,
                                              void* userData);

/*
 * Expression-side units result storage for one compile pipeline run.
 * This is the explicit Phase 3 side-table seam used by later units algebra.
 */
bool fisics_extension_set_units_expr_result(struct CompilerContext* ctx,
                                            const struct ASTNode* node,
                                            FisicsDim8 dim,
                                            bool resolved);
bool fisics_extension_set_units_expr_result_with_unit(struct CompilerContext* ctx,
                                                      const struct ASTNode* node,
                                                      FisicsDim8 dim,
                                                      bool resolved,
                                                      const FisicsUnitDef* unitDef,
                                                      bool unitResolved);
void fisics_extension_clear_units_expr_results(struct CompilerContext* ctx);

const FisicsUnitsExprResult* fisics_extension_lookup_units_expr_result(
    const struct CompilerContext* ctx,
    const struct ASTNode* node);
const FisicsUnitsExprResult* fisics_extension_units_expr_result_at(
    const struct CompilerContext* ctx,
    size_t index);
void fisics_extension_for_each_units_expr_result(const struct CompilerContext* ctx,
                                                 FisicsUnitsExprResultCallback callback,
                                                 void* userData);

size_t fisics_extension_units_expr_result_count(const struct CompilerContext* ctx);
void fisics_extension_dump_units_expr_results(const struct CompilerContext* ctx, FILE* out);
