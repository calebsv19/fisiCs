// SPDX-License-Identifier: Apache-2.0

// analyze_core.c
#include "analyze_core.h"
#include "analyze_decls.h"
#include "analyze_expr.h"
#include "analyze_stmt.h"
#include "scope.h"
#include "symbol_table.h"
#include "type_checker.h"
#include "Parser/Helpers/parsed_type.h"
#include "Utils/profiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void analyzeChildren(ASTNode** nodes, size_t n, Scope* scope) {
    if (!nodes) return;
    for (size_t i = 0; i < n; i++) {
        analyze(nodes[i], scope);
    }
}

static void addFuncBuiltinIdentifiers(Scope* scope, const char* funcName) {
    if (!scope) return;
    Symbol* sym = (Symbol*)calloc(1, sizeof(Symbol));
    if (!sym) return;
    sym->name = strdup("__func__");
    if (!sym->name) {
        free(sym);
        return;
    }
    sym->kind = SYMBOL_VARIABLE;
    sym->hasDefinition = true;
    sym->definition = NULL;
    ParsedType t;
    memset(&t, 0, sizeof(t));
    t.kind = TYPE_PRIMITIVE;
    t.tag = TAG_NONE;
    t.primitiveType = TOKEN_CHAR;
    t.isConst = true;
    parsedTypeAppendPointer(&t);
    sym->type = t;
    primeSymbolTypeInfoCache(sym, scope);
    addToScope(scope, sym);
    (void)funcName;
}

void analyze(ASTNode* node, Scope* scope) {
    if (!node) return;
    ProfilerScope analyzeScope = profiler_begin("semantic_analyze_dispatch");
    profiler_record_value("semantic_count_analyze_node", 1);

    switch (node->type) {
    case AST_PROGRAM: {
        // Global scope: just walk children
        ASTNode** statements = node->block.statements;
        size_t statementCount = node->block.statementCount;
        analyzeChildren(statements, statementCount, scope);
        break;
    }

    case AST_BLOCK:
        analyzeStatement(node, scope);
        break;

    case AST_FUNCTION_DEFINITION: {
        // 1) Ensure the function symbol exists (let decl logic register it)
        analyzeDeclaration(node, scope);

        // 2) Create a function scope chained to the parent
        Scope* fscope = createScope(scope);
        if (fscope) {
            fscope->hasReturnType = true;
            profiler_record_value("semantic_count_type_info_site_return", 1);
            fscope->returnType = typeInfoFromParsedType(&node->functionDef.returnType, scope);
            fscope->inFunction = true;
            fscope->currentFunctionIsVariadic = node->functionDef.isVariadic;
            fscope->currentFunctionName = NULL;
            ASTNode* funcNameNode = node->functionDef.funcName;
            if (funcNameNode) {
                fscope->currentFunctionName = funcNameNode->valueNode.value;
            }
            addFuncBuiltinIdentifiers(fscope, fscope->currentFunctionName);
            size_t fixedCount = 0;
            ASTNode** params = node->functionDef.parameters;
            for (size_t i = 0; i < node->functionDef.paramCount; ++i) {
                ASTNode* p = params ? params[i] : NULL;
                if (!p || p->type != AST_VARIABLE_DECLARATION) continue;
                fixedCount += p->varDecl.varCount;
            }
            fscope->currentFunctionFixedParams = fixedCount;
        }

        // 3) Bind parameters (each parameter node is usually a VAR_DECL;
        //    support multiple names per declaration:  int a, b )
        ASTNode** params = node->functionDef.parameters;
        for (size_t i = 0; i < node->functionDef.paramCount; i++) {
            ASTNode* p = params ? params[i] : NULL;
            if (!p) continue;

            if (p->type == AST_VARIABLE_DECLARATION) {
                ParsedType* perTypes = p->varDecl.declaredTypes;
                ASTNode** varNames = p->varDecl.varNames;
                for (size_t k = 0; k < p->varDecl.varCount; k++) {
                    ASTNode* nameNode = varNames ? varNames[k] : NULL;
                    if (!nameNode || nameNode->type != AST_IDENTIFIER) {
                        // Be forgiving but noisy during bring-up
                        fprintf(stderr, "Semantic: parameter[%zu] has non-identifier name\n", k);
                        continue;
                    }
                    const ParsedType* typeForParam = &p->varDecl.declaredType;
                    if (perTypes) {
                        typeForParam = &perTypes[k];
                    }

                    // Create symbol
            Symbol* s = (Symbol*)calloc(1, sizeof(Symbol));
            if (!s) { fprintf(stderr, "OOM: Symbol param\n"); continue; }
            s->kind = SYMBOL_VARIABLE;
            const char* paramName = nameNode->valueNode.value;
            s->name = strdup(paramName);
            if (!s->name) {
                fprintf(stderr, "OOM: Symbol param name\n");
                free(s);
                continue;
            }
            s->type = *typeForParam;              // copy-by-value of ParsedType
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
        ASTNode* functionBody = node->functionDef.body;
        validateGotoScopes(functionBody);
        analyze(functionBody, fscope);

        destroyScope(fscope);
        break;
    }

    // Declarations at current scope
    case AST_FUNCTION_DECLARATION:
    case AST_VARIABLE_DECLARATION:
    case AST_STRUCT_DEFINITION:
    case AST_UNION_DEFINITION:
    case AST_ENUM_DEFINITION:
    case AST_TYPEDEF:
    case AST_STATIC_ASSERT:
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
    case AST_STATEMENT_EXPRESSION:
    case AST_ARRAY_ACCESS:
    case AST_POINTER_ACCESS:
    case AST_POINTER_DEREFERENCE:
    case AST_FUNCTION_CALL:
    case AST_IDENTIFIER:
    case AST_NUMBER_LITERAL:
    case AST_CHAR_LITERAL:
    case AST_STRING_LITERAL:
    case AST_SIZEOF:
    case AST_ALIGNOF:
        (void)analyzeExpression(node, scope);
        break;

    default:
        // Unknown/unsupported node kinds can be ignored for now
        // to keep bring-up smooth.
        // fprintf(stderr, "Semantic: unhandled node type %d\n", node->type);
        break;
    }

    profiler_end(analyzeScope);
}
