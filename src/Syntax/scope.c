#include "scope.h"
#include <stdio.h>

Scope* createScope(Scope* parent) {
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
    return insertSymbol(&scope->table, sym);
}

Symbol* resolveInScopeChain(Scope* scope, const char* name) {
    while (scope) {
        Symbol* found = lookupSymbol(&scope->table, name);
        if (found) return found;
        scope = scope->parent;
    }
    return NULL;
}
