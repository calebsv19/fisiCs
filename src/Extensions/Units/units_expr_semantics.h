// SPDX-License-Identifier: Apache-2.0

#pragma once

struct Scope;
struct ASTNode;

void fisics_units_run_expr_semantics(struct ASTNode* root, struct Scope* globalScope);
