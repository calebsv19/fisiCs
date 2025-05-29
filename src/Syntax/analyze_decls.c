#include "analyze_decls.h"
#include "symbol_table.h"
#include "syntax_errors.h"
#include <string.h>
#include <stdlib.h>

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
                    addError(0, 0, "Redefinition of variable", ident->valueNode.value);
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
                addError(0, 0, "Redefinition of function", funcName->valueNode.value);
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
                addError(0, 0, "Redefinition of typedef", sym->name);
            }
            break;
        }

        case AST_STRUCT_DEFINITION:
        case AST_ENUM_DEFINITION: {
            ASTNode* name = (node->type == AST_STRUCT_DEFINITION)
                ? node->structDef.structName
                : node->enumDef.enumName;

            Symbol* sym = malloc(sizeof(Symbol));
            sym->name = strdup(name->valueNode.value);
            sym->kind = (node->type == AST_STRUCT_DEFINITION) ? SYMBOL_STRUCT : SYMBOL_ENUM;
            sym->definition = node;
            sym->next = NULL;

            // In these cases, we don’t attach ParsedType directly unless needed
            if (!addToScope(scope, sym)) {
                addError(0, 0, "Redefinition of type", sym->name);
            }
            break;
        }

        default:
            addError(0, 0, "Unknown declaration node", "This node is not handled in analyzeDeclaration()");
            break;
    }
}

