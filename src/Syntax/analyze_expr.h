#ifndef ANALYZE_EXPR_H
#define ANALYZE_EXPR_H

#include "../ast_node.h"
#include "scope.h"

void analyzeExpression(ASTNode* node, Scope* scope);

#endif // ANALYZE_EXPR_H

