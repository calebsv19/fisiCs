// SPDX-License-Identifier: Apache-2.0

#pragma once

struct ASTNode;
struct CompilerContext;

void fisics_extension_diag_units_disabled(struct CompilerContext* ctx,
                                          const struct ASTNode* node,
                                          const char* exprText);
void fisics_extension_diag_invalid_units_expr(struct CompilerContext* ctx,
                                              const struct ASTNode* node,
                                              const char* exprText,
                                              const char* detail);
void fisics_extension_diag_duplicate_units_attr(struct CompilerContext* ctx,
                                                const struct ASTNode* node);
