// SPDX-License-Identifier: Apache-2.0

#ifndef CONTROL_FLOW_H
#define CONTROL_FLOW_H

#include "AST/ast_node.h"
#include "scope.h"

void analyzeControlFlow(ASTNode* root, Scope* globalScope);

#endif // CONTROL_FLOW_H
