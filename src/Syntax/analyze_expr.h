#ifndef ANALYZE_EXPR_H
#define ANALYZE_EXPR_H

#include "AST/ast_node.h"
#include "scope.h"
#include "type_checker.h"

TypeInfo analyzeExpression(ASTNode* node, Scope* scope);
TypeInfo decayToRValue(TypeInfo info);

#endif // ANALYZE_EXPR_H
