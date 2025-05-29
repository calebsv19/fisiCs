#ifndef ANALYZE_CORE_H
#define ANALYZE_CORE_H

#include "../ast_node.h"
#include "scope.h"

// Central dispatch
void analyze(ASTNode* node, Scope* scope);

// Utility
void analyzeChildren(ASTNode** list, size_t count, Scope* scope);

#endif // ANALYZE_CORE_H

