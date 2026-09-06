// SPDX-License-Identifier: Apache-2.0

#pragma once

struct ASTNode;
struct Scope;

void fisics_semantic_metadata_run(struct ASTNode* root, struct Scope* globalScope);
