// SPDX-License-Identifier: Apache-2.0
#pragma once
struct ASTNode;
struct CompilerContext;
struct Scope;

/* Check known parameter dimensions independently of optional concrete units. */
void fisics_units_validate_call_arguments(struct CompilerContext* ctx,
                                          struct ASTNode* call,
                                          struct Scope* rootScope);
