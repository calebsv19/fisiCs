#include "analyze_expr.h"
#include "syntax_errors.h"
#include "symbol_table.h"
#include <string.h>

void analyzeExpression(ASTNode* node, Scope* scope) {
    if (!node) return;

    switch (node->type) {
        case AST_IDENTIFIER: {
            Symbol* sym = resolveInScopeChain(scope, node->valueNode.value);
            if (!sym) {
                addError(0, 0, "Undeclared identifier", node->valueNode.value);
            }
            break;
        }

        case AST_BINARY_EXPRESSION:
        case AST_COMMA_EXPRESSION: {
            analyzeExpression(node->expr.left, scope);
            analyzeExpression(node->expr.right, scope);
            break;
        }

        case AST_UNARY_EXPRESSION: {
            analyzeExpression(node->expr.left, scope);
            break;
        }

        case AST_CAST_EXPRESSION: {
            // Validate target type here if needed
            analyzeExpression(node->castExpr.expression, scope);
            break;
        }

        case AST_FUNCTION_CALL: {
            analyzeExpression(node->functionCall.callee, scope);
            for (size_t i = 0; i < node->functionCall.argumentCount; i++) {
                analyzeExpression(node->functionCall.arguments[i], scope);
            }
            break;
        }

        case AST_POINTER_DEREFERENCE:
            analyzeExpression(node->pointerDeref.pointer, scope);
            break;

        case AST_DOT_ACCESS:
        case AST_POINTER_ACCESS:
            analyzeExpression(node->memberAccess.base, scope);
            // In future: check if base type supports the field
            break;

        case AST_ARRAY_ACCESS:
            analyzeExpression(node->arrayAccess.array, scope);
            analyzeExpression(node->arrayAccess.index, scope);
            break;

        case AST_TERNARY_EXPRESSION:
            analyzeExpression(node->ternaryExpr.condition, scope);
            analyzeExpression(node->ternaryExpr.trueExpr, scope);
            analyzeExpression(node->ternaryExpr.falseExpr, scope);
            break;

        case AST_NUMBER_LITERAL:
        case AST_STRING_LITERAL:
        case AST_CHAR_LITERAL:
        case AST_SIZEOF:
            // Literals usually don’t need validation
            break;

        default:
            addError(0, 0, "Unhandled expression node", "No analysis implemented for this expression");
            break;
    }
}

