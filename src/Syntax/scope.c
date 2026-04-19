// SPDX-License-Identifier: Apache-2.0

#include "scope.h"
#include "Utils/profiler.h"
#include <stdio.h>

Scope* createScope(Scope* parent) {
    profiler_record_value("semantic_count_create_scope", 1);
    Scope* scope = malloc(sizeof(Scope));
    if (!scope) {
        fprintf(stderr, "Error: Failed to allocate memory for Scope.\n");
        return NULL;
    }
    initSymbolTable(&scope->table);
    scope->parent = parent;
    scope->depth = parent ? parent->depth + 1 : 0;
    scope->ctx = parent ? parent->ctx : NULL;
    scope->hasReturnType = parent ? parent->hasReturnType : false;
    scope->returnType = parent ? parent->returnType : makeInvalidType();
    scope->inFunction = parent ? parent->inFunction : false;
    scope->currentFunctionIsVariadic = parent ? parent->currentFunctionIsVariadic : false;
    scope->currentFunctionFixedParams = parent ? parent->currentFunctionFixedParams : 0;
    scope->currentFunctionName = parent ? parent->currentFunctionName : NULL;
    return scope;
}

void destroyScope(Scope* scope) {
    if (!scope) return;
    freeSymbolTable(&scope->table);
    scope->ctx = NULL;
    free(scope);
}

bool addToScope(Scope* scope, Symbol* sym) {
    profiler_record_value("semantic_count_scope_add", 1);
    return insertSymbol(&scope->table, sym);
}

Symbol* resolveInScopeChain(Scope* scope, const char* name) {
    profiler_record_value("semantic_count_scope_resolve_chain", 1);
    while (scope) {
        Symbol* found = lookupSymbol(&scope->table, name);
        if (found) {
            profiler_record_value("semantic_count_scope_resolve_chain_hit", 1);
            return found;
        }
        scope = scope->parent;
    }
    return NULL;
}
