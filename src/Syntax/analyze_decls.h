// SPDX-License-Identifier: Apache-2.0

#ifndef ANALYZE_DECLS_H
#define ANALYZE_DECLS_H

#include "AST/ast_node.h"
#include "scope.h"

void analyzeDeclaration(ASTNode* node, Scope* scope);

#endif // ANALYZE_DECLS_H

