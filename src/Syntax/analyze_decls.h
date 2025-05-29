#ifndef ANALYZE_DECLS_H
#define ANALYZE_DECLS_H

#include "../ast_node.h"
#include "scope.h"

void analyzeDeclaration(ASTNode* node, Scope* scope);

#endif // ANALYZE_DECLS_H

