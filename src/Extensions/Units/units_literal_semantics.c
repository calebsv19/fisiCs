// SPDX-License-Identifier: Apache-2.0
#include "Extensions/Units/units_literal_semantics.h"
#include "AST/ast_node.h"
#include <string.h>

/* A declaration may supply the physical interpretation of authored numeric
 * constants. Never extend that exception to identifiers or calls: a known
 * dimensionless variable/expression is still subject to dimension equality. */
static bool literal_value(const ASTNode* node, unsigned depth) {
    if (!node || depth > 64) return false;
    switch (node->type) {
        case AST_NUMBER_LITERAL:
        case AST_CHAR_LITERAL:
            return true;
        case AST_CAST_EXPRESSION:
            return literal_value(node->castExpr.expression, depth + 1);
        case AST_UNARY_EXPRESSION:
            return node->expr.op &&
                   (!strcmp(node->expr.op, "+") || !strcmp(node->expr.op, "-") ||
                    !strcmp(node->expr.op, "~") || !strcmp(node->expr.op, "!")) &&
                   literal_value(node->expr.left, depth + 1);
        case AST_BINARY_EXPRESSION:
            return literal_value(node->expr.left, depth + 1) &&
                   literal_value(node->expr.right, depth + 1);
        case AST_TERNARY_EXPRESSION:
            return literal_value(node->ternaryExpr.condition, depth + 1) &&
                   literal_value(node->ternaryExpr.trueExpr, depth + 1) &&
                   literal_value(node->ternaryExpr.falseExpr, depth + 1);
        default:
            return false;
    }
}

bool fisics_units_is_literal_value(const ASTNode* node) {
    return literal_value(node, 0);
}
