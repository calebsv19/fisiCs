#include "analyze_stmt.h"
#include "analyze_core.h"
#include "analyze_expr.h"
#include "syntax_errors.h"
#include "type_checker.h"

void analyzeStatement(ASTNode* node, Scope* scope) {
    switch (node->type) {
        case AST_IF_STATEMENT:
            analyze(node->ifStmt.condition, scope);
            analyze(node->ifStmt.thenBranch, scope);
            if (node->ifStmt.elseBranch) {
                analyze(node->ifStmt.elseBranch, scope);
            }
            break;

        case AST_FOR_LOOP: {
            Scope* inner = createScope(scope);
            analyze(node->forLoop.initializer, inner);
            analyze(node->forLoop.condition, inner);
            analyze(node->forLoop.increment, inner);
            analyze(node->forLoop.body, inner);
            destroyScope(inner);
            break;
        }

        case AST_WHILE_LOOP:
            analyze(node->whileLoop.condition, scope);
            analyze(node->whileLoop.body, scope);
            break;

        case AST_RETURN:
            if (node->returnStmt.returnValue) {
                TypeInfo retVal = analyzeExpression(node->returnStmt.returnValue, scope);
                retVal = decayToRValue(retVal);
                if (scope && scope->hasReturnType) {
                    AssignmentCheckResult res = canAssignTypes(&scope->returnType, &retVal);
                    if (res == ASSIGN_QUALIFIER_LOSS) {
                        addError(node->line, 0, "Return discards qualifiers from pointer target", NULL);
                    }
                }
            } else {
                if (scope && scope->hasReturnType && scope->returnType.category != TYPEINFO_VOID) {
                    addError(node->line, 0, "Non-void function must return a value", NULL);
                }
            }
            break;

        case AST_BREAK:
        case AST_CONTINUE:
            // Could track enclosing loop for validation
            break;

        case AST_SWITCH:
            analyze(node->switchStmt.condition, scope);
            for (size_t i = 0; i < node->switchStmt.caseListSize; i++) {
                analyze(node->switchStmt.caseList[i], scope);
            }
            break;

        case AST_CASE:
            if (node->caseStmt.caseValue) {
                analyze(node->caseStmt.caseValue, scope);
            }
            for (size_t i = 0; i < node->caseStmt.caseBodySize; i++) {
                analyze(node->caseStmt.caseBody[i], scope);
            }
            if (node->caseStmt.nextCase) {
                analyze(node->caseStmt.nextCase, scope);
            }
            break;

        case AST_LABEL_DECLARATION:
            if (node->label.statement) {
                analyze(node->label.statement, scope);
            }
            break;

        case AST_GOTO_STATEMENT:
            // Goto/label validation not implemented yet
            break;

        case AST_BLOCK:
            for (size_t i = 0; i < node->block.statementCount; ++i) {
                analyze(node->block.statements[i], scope);
            }
            break;

        case AST_ASSIGNMENT:
        case AST_BINARY_EXPRESSION:
        case AST_UNARY_EXPRESSION:
        case AST_FUNCTION_CALL:
        case AST_COMPOUND_LITERAL:
        case AST_CAST_EXPRESSION:
            (void)analyzeExpression(node, scope);
            break;

        default:
            addError(node ? node->line : 0, 0, "Unhandled statement node", "No analysis implemented for this statement type");
            break;
    }
}
