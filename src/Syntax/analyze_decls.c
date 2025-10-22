#include "analyze_decls.h"
#include "symbol_table.h"
#include "syntax_errors.h"
#include "scope.h"
#include "Compiler/compiler_context.h"
#include "Parser/Helpers/designated_init.h"
#include "analyze_expr.h"
#include <string.h>
#include <stdlib.h>

static void analyzeDesignatedInitializer(DesignatedInit* init, Scope* scope) {
    if (!init || !scope) return;
    if (init->indexExpr) {
        analyzeExpression(init->indexExpr, scope);
    }
    if (init->expression) {
        analyzeExpression(init->expression, scope);
    }
}

static void analyzeDesignatedInitializerList(DesignatedInit** list, size_t count, Scope* scope) {
    if (!list) return;
    for (size_t i = 0; i < count; ++i) {
        analyzeDesignatedInitializer(list[i], scope);
    }
}

void analyzeDeclaration(ASTNode* node, Scope* scope) {
    switch (node->type) {
        case AST_VARIABLE_DECLARATION: {
            for (size_t i = 0; i < node->varDecl.varCount; i++) {
                ASTNode* ident = node->varDecl.varNames[i];
                Symbol* sym = malloc(sizeof(Symbol));
                sym->name = strdup(ident->valueNode.value);
                sym->kind = SYMBOL_VARIABLE;
                sym->type = node->varDecl.declaredType;
                sym->definition = node;
                sym->next = NULL;

                if (!addToScope(scope, sym)) {
                    addError(ident ? ident->line : node->line, 0, "Redefinition of variable", ident->valueNode.value);
                }

                if (i < node->varDecl.varCount && node->varDecl.initializers) {
                    analyzeDesignatedInitializer(node->varDecl.initializers[i], scope);
                }
            }
            break;
        }

        case AST_FUNCTION_DECLARATION:
        case AST_FUNCTION_DEFINITION: {
            ASTNode* funcName = node->type == AST_FUNCTION_DEFINITION
                ? node->functionDef.funcName
                : node->functionDecl.funcName;

            Symbol* sym = malloc(sizeof(Symbol));
            sym->name = strdup(funcName->valueNode.value);
            sym->kind = SYMBOL_FUNCTION;
            sym->type = node->type == AST_FUNCTION_DEFINITION
                ? node->functionDef.returnType
                : node->functionDecl.returnType;
            sym->definition = node;
            sym->next = NULL;

            if (!addToScope(scope, sym)) {
                addError(funcName ? funcName->line : node->line, 0, "Redefinition of function", funcName->valueNode.value);
            }
            break;
        }

        case AST_TYPEDEF: {
            Symbol* sym = malloc(sizeof(Symbol));
            sym->name = strdup(node->typedefStmt.alias->valueNode.value);
            sym->kind = SYMBOL_TYPEDEF;
            sym->type = node->typedefStmt.baseType;
            sym->definition = node;
            sym->next = NULL;

            if (!addToScope(scope, sym)) {
                addError(node ? node->line : 0, 0, "Redefinition of typedef", sym->name);
            }
            if (scope->ctx) {
                cc_add_typedef(scope->ctx, node->typedefStmt.alias->valueNode.value);
            }
            break;
        }

        case AST_ARRAY_DECLARATION: {
            ASTNode* nameNode = node->arrayDecl.varName;
            if (!nameNode || nameNode->type != AST_IDENTIFIER) {
                break;
            }

            Symbol* sym = malloc(sizeof(Symbol));
            sym->name = strdup(nameNode->valueNode.value);
            sym->kind = SYMBOL_VARIABLE;
            sym->type = node->arrayDecl.declaredType;
            sym->definition = node;
            sym->next = NULL;

            if (!addToScope(scope, sym)) {
                addError(nameNode ? nameNode->line : node->line, 0, "Redefinition of variable", nameNode->valueNode.value);
            }

            ASTNode* dim = node->arrayDecl.arraySize;
            while (dim) {
                analyzeExpression(dim, scope);
                dim = dim->nextStmt;
            }

            analyzeDesignatedInitializerList(node->arrayDecl.initializers, node->arrayDecl.valueCount, scope);
            break;
        }

        case AST_STRUCT_DEFINITION:
        case AST_UNION_DEFINITION:
        case AST_ENUM_DEFINITION: {
            ASTNode* name = (node->type == AST_ENUM_DEFINITION)
                ? node->enumDef.enumName
                : node->structDef.structName;
            if (!name || name->type != AST_IDENTIFIER) {
                break;
            }

            if (scope->ctx) {
                CCTagKind tagKind = CC_TAG_STRUCT;
                if (node->type == AST_UNION_DEFINITION) tagKind = CC_TAG_UNION;
                else if (node->type == AST_ENUM_DEFINITION) tagKind = CC_TAG_ENUM;

                if (!cc_add_tag(scope->ctx, tagKind, name->valueNode.value)) {
                    addError(name ? name->line : node->line, 0, "Redefinition of type", name->valueNode.value);
                }
            }
            break;
        }

        default:
            addError(node ? node->line : 0, 0, "Unknown declaration node", "This node is not handled in analyzeDeclaration()");
            break;
    }
}
