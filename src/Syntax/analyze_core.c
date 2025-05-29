#include "analyze_core.h"
#include "analyze_decls.h"
#include "analyze_expr.h"
#include "analyze_stmt.h"
#include "syntax_errors.h"

void analyze(ASTNode* node, Scope* scope) {
    if (!node) return;

    switch (node->type) {
        case AST_PROGRAM:
        case AST_BLOCK:
            analyzeChildren(node->block.statements, node->block.statementCount, scope);
            break;

        case AST_FUNCTION_DEFINITION:
        case AST_FUNCTION_DECLARATION:
        case AST_VARIABLE_DECLARATION:
        case AST_TYPEDEF:
        case AST_STRUCT_DEFINITION:
        case AST_ENUM_DEFINITION:
            analyzeDeclaration(node, scope);
            break;

        case AST_ASSIGNMENT:
        case AST_BINARY_EXPRESSION:
        case AST_UNARY_EXPRESSION:
        case AST_CAST_EXPRESSION:
        case AST_FUNCTION_CALL:
        case AST_IDENTIFIER:
            analyzeExpression(node, scope);
            break;

        case AST_IF_STATEMENT:
        case AST_FOR_LOOP:
        case AST_WHILE_LOOP:
        case AST_RETURN:
        case AST_BREAK:
        case AST_CONTINUE:
        case AST_SWITCH:
        case AST_CASE:
            analyzeStatement(node, scope);
            break;

        default:
            addError(0, 0, "Unhandled AST node in semantic analysis", "Node type unrecognized in analyze()");
            break;
    }
}

void analyzeChildren(ASTNode** list, size_t count, Scope* scope) {
    for (size_t i = 0; i < count; i++) {
        analyze(list[i], scope);
    }
}

