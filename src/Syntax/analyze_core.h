// SPDX-License-Identifier: Apache-2.0

#ifndef ANALYZE_CORE_H
#define ANALYZE_CORE_H

#include "AST/ast_node.h"
#include "scope.h"

// Central dispatch
void analyze(ASTNode* node, Scope* scope);


#endif // ANALYZE_CORE_H

