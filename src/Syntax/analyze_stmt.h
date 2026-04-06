// SPDX-License-Identifier: Apache-2.0

#ifndef ANALYZE_STMT_H
#define ANALYZE_STMT_H

#include "AST/ast_node.h"
#include "scope.h"
void analyzeStatement(ASTNode* node, Scope* scope);
void validateGotoScopes(ASTNode* node);

#endif // ANALYZE_STMT_H
