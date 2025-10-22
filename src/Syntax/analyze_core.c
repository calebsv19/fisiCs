// analyze_core.c
#include "analyze_core.h"
#include "analyze_decls.h"
#include "analyze_expr.h"
#include "analyze_stmt.h"
#include "scope.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>

static void analyzeChildren(ASTNode** nodes, size_t n, Scope* scope) {
    if (!nodes) return;
    for (size_t i = 0; i < n; i++) {
        analyze(nodes[i], scope);
    }
}

void analyze(ASTNode* node, Scope* scope) {
    if (!node) return;

    switch (node->type) {
    case AST_PROGRAM:
        // Global scope: just walk children
        analyzeChildren(node->block.statements, node->block.statementCount, scope);
        break;

    case AST_BLOCK: {
        // New lexical scope for every compound statement
        Scope* inner = createScope(scope);
        analyzeChildren(node->block.statements, node->block.statementCount, inner);
        destroyScope(inner);
        break;
    }

    case AST_FUNCTION_DEFINITION: {
        // 1) Ensure the function symbol exists (let decl logic register it)
        analyzeDeclaration(node, scope);

        // 2) Create a function scope chained to the parent
        Scope* fscope = createScope(scope);

        // 3) Bind parameters (each parameter node is usually a VAR_DECL;
        //    support multiple names per declaration:  int a, b )
        for (size_t i = 0; i < node->functionDef.paramCount; i++) {
            ASTNode* p = node->functionDef.parameters[i];
            if (!p) continue;

            if (p->type == AST_VARIABLE_DECLARATION) {
                ParsedType pt = p->varDecl.declaredType;

                for (size_t k = 0; k < p->varDecl.varCount; k++) {
                    ASTNode* nameNode = p->varDecl.varNames[k];
                    if (!nameNode || nameNode->type != AST_IDENTIFIER) {
                        // Be forgiving but noisy during bring-up
                        fprintf(stderr, "Semantic: parameter[%zu] has non-identifier name\n", k);
                        continue;
                    }

                    // Create symbol
                    Symbol* s = (Symbol*)calloc(1, sizeof(Symbol));
                    if (!s) { fprintf(stderr, "OOM: Symbol param\n"); continue; }
                    s->kind = SYMBOL_VARIABLE;
                    s->name = nameNode->valueNode.value; // string owned by AST
                    s->type = pt;                         // copy-by-value of ParsedType
                    s->definition = p;

                    addToScope(fscope, s);
                }

                // (Optional) If you ever allow parameter default initializers, analyze them:
                // for (size_t k = 0; k < p->varDecl.varCount; k++) {
                //     DesignatedInit* init = p->varDecl.initializers[k];
                //     if (init && init->expression) analyzeExpression(init->expression, fscope);
                // }
            } else {
                // If your parser can emit other param node shapes, add cases here.
                fprintf(stderr, "Semantic: unexpected parameter node type %d\n", p->type);
            }
        }

        // 4) Analyze the function body under the function scope
        analyze(node->functionDef.body, fscope);

        destroyScope(fscope);
        break;
    }

    // Declarations at current scope
    case AST_FUNCTION_DECLARATION:
    case AST_VARIABLE_DECLARATION:
    case AST_ARRAY_DECLARATION:
    case AST_STRUCT_DEFINITION:
    case AST_UNION_DEFINITION:
    case AST_ENUM_DEFINITION:
    case AST_TYPEDEF:
        analyzeDeclaration(node, scope);
        break;

    // Statements/expressions
    case AST_IF_STATEMENT:
    case AST_WHILE_LOOP:
    case AST_FOR_LOOP:
    case AST_SWITCH:
    case AST_CASE:
    case AST_GOTO_STATEMENT:
    case AST_LABEL_DECLARATION:
    case AST_RETURN:
    case AST_BREAK:
    case AST_CONTINUE:
    case AST_ASM:
        analyzeStatement(node, scope);
        break;

    // Expressions that might appear at top-level (or via defensive calls)
    case AST_ASSIGNMENT:
    case AST_BINARY_EXPRESSION:
    case AST_UNARY_EXPRESSION:
    case AST_TERNARY_EXPRESSION:
    case AST_COMMA_EXPRESSION:
    case AST_CAST_EXPRESSION:
    case AST_COMPOUND_LITERAL:
    case AST_ARRAY_ACCESS:
    case AST_POINTER_ACCESS:
    case AST_POINTER_DEREFERENCE:
    case AST_FUNCTION_CALL:
    case AST_IDENTIFIER:
    case AST_NUMBER_LITERAL:
    case AST_CHAR_LITERAL:
    case AST_STRING_LITERAL:
    case AST_SIZEOF:
        analyzeExpression(node, scope);
        break;

    default:
        // Unknown/unsupported node kinds can be ignored for now
        // to keep bring-up smooth.
        // fprintf(stderr, "Semantic: unhandled node type %d\n", node->type);
        break;
    }
}
