// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Extensions/Units/units_model.h"
#include "Extensions/Units/units_registry.h"

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
void fisics_extension_diag_invalid_unit_attr(struct CompilerContext* ctx,
                                             const struct ASTNode* node,
                                             const char* unitText,
                                             const char* detail);
void fisics_extension_diag_duplicate_unit_attr(struct CompilerContext* ctx,
                                               const struct ASTNode* node);
void fisics_extension_diag_unit_requires_dim(struct CompilerContext* ctx,
                                             const struct ASTNode* node,
                                             const char* unitText);
void fisics_extension_diag_unit_dim_mismatch(struct CompilerContext* ctx,
                                             const struct ASTNode* node,
                                             const char* unitText,
                                             FisicsDim8 declDim,
                                             FisicsDim8 unitDim);
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
void fisics_extension_diag_units_implicit_unit_conversion(struct CompilerContext* ctx,
                                                          const struct ASTNode* node,
                                                          const char* context,
                                                          const FisicsUnitDef* sourceUnit,
                                                          const FisicsUnitDef* targetUnit);
void fisics_extension_diag_units_conversion_invalid_target(struct CompilerContext* ctx,
                                                           const struct ASTNode* node,
                                                           const char* targetText,
                                                           const char* detail);
void fisics_extension_diag_units_conversion_incompatible(struct CompilerContext* ctx,
                                                         const struct ASTNode* node,
                                                         const FisicsUnitDef* sourceUnit,
                                                         const FisicsUnitDef* targetUnit,
                                                         const char* detail);
void fisics_extension_diag_units_conversion_requires_source_unit(struct CompilerContext* ctx,
                                                                 const struct ASTNode* node,
                                                                 const char* targetText);
void fisics_extension_diag_units_conversion_requires_floating(struct CompilerContext* ctx,
                                                              const struct ASTNode* node,
                                                              const char* calleeName);
