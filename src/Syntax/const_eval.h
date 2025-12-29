#ifndef CONST_EVAL_H
#define CONST_EVAL_H

#include <stdbool.h>
#include <stdint.h>

#include "AST/ast_node.h"

struct Scope;

typedef struct {
    bool isConst;
    long long value;
    unsigned bitWidth;
    bool isUnsigned;
} ConstEvalResult;

// Evaluate an integer constant expression in a semantic (non-codegen) context.
// `allowEnumRefs` lets callers decide whether bare enum constants are permitted.
ConstEvalResult constEval(ASTNode* expr,
                          struct Scope* scope,
                          bool allowEnumRefs);

// Convenience wrapper to match older call sites; returns true on success and
// writes the value into `out`.
bool constEvalInteger(ASTNode* expr,
                      struct Scope* scope,
                      long long* out,
                      bool allowEnumRefs);

#endif // CONST_EVAL_H
