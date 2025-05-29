#ifndef ANALYZE_STMT_H
#define ANALYZE_STMT_H

#include "../ast_node.h"
#include "scope.h"

void analyzeStatement(ASTNode* node, Scope* scope);

#endif // ANALYZE_STMT_H

