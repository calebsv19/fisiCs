#ifndef CONST_EVAL_H
#define CONST_EVAL_H

#include <stdbool.h>
#include <stdint.h>

#include "AST/ast_node.h"

struct Scope;

// Evaluate an integer constant expression in a semantic (non-codegen) context.
// Returns true when the expression is a valid constant and stores the value in `out`.
// `allowEnumRefs` lets callers decide whether bare enum constants are permitted.
bool constEvalInteger(ASTNode* expr,
                      struct Scope* scope,
                      long long* out,
                      bool allowEnumRefs);

#endif // CONST_EVAL_H
