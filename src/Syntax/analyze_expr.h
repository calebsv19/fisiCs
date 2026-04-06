// SPDX-License-Identifier: Apache-2.0

#ifndef ANALYZE_EXPR_H
#define ANALYZE_EXPR_H

#include "AST/ast_node.h"
#include "scope.h"
#include "type_checker.h"

TypeInfo analyzeExpression(ASTNode* node, Scope* scope);
TypeInfo decayToRValue(TypeInfo info);
bool evalOffsetofFieldPath(const ParsedType* baseType,
                           const char* fieldPath,
                           Scope* scope,
                           size_t* offsetOut,
                           bool* bitfieldOut);

#endif // ANALYZE_EXPR_H
