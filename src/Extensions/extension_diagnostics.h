// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Extensions/Units/units_model.h"

struct ASTNode;
struct CompilerContext;

/*
 * Current public namespace ownership:
 * - 4001-4003: scaffold/bootstrap units diagnostics already emitted in Phase 2-4
 * - 4100-4199: reserved for Phase 5 physics-units semantic diagnostics
 */
void fisics_extension_diag_units_disabled(struct CompilerContext* ctx,
                                          const struct ASTNode* node,
                                          const char* exprText);
void fisics_extension_diag_invalid_units_expr(struct CompilerContext* ctx,
                                              const struct ASTNode* node,
                                              const char* exprText,
                                              const char* detail);
void fisics_extension_diag_duplicate_units_attr(struct CompilerContext* ctx,
                                                const struct ASTNode* node);
void fisics_extension_diag_units_assign_dim_mismatch(struct CompilerContext* ctx,
                                                     const struct ASTNode* node,
                                                     FisicsDim8 lhsDim,
                                                     FisicsDim8 rhsDim);
void fisics_extension_diag_units_add_dim_mismatch(struct CompilerContext* ctx,
                                                  const struct ASTNode* node,
                                                  FisicsDim8 lhsDim,
                                                  FisicsDim8 rhsDim);
void fisics_extension_diag_units_sub_dim_mismatch(struct CompilerContext* ctx,
                                                  const struct ASTNode* node,
                                                  FisicsDim8 lhsDim,
                                                  FisicsDim8 rhsDim);
void fisics_extension_diag_units_compare_dim_mismatch(struct CompilerContext* ctx,
                                                      const struct ASTNode* node,
                                                      FisicsDim8 lhsDim,
                                                      FisicsDim8 rhsDim);
void fisics_extension_diag_units_exponent_overflow(struct CompilerContext* ctx,
                                                   const struct ASTNode* node,
                                                   const char* op,
                                                   FisicsDim8 lhsDim,
                                                   FisicsDim8 rhsDim);
void fisics_extension_diag_units_ternary_dim_mismatch(struct CompilerContext* ctx,
                                                      const struct ASTNode* node,
                                                      FisicsDim8 trueDim,
                                                      FisicsDim8 falseDim);
