#ifndef SCOPE_H
#define SCOPE_H

#include "symbol_table.h"
#include "Compiler/compiler_context.h"
#include "type_checker.h"
#include <stdlib.h>

typedef struct Scope {
    struct Scope* parent;
    SymbolTable table;
    int depth;
    CompilerContext* ctx;
    bool hasReturnType;
    TypeInfo returnType;
    bool inFunction;
    bool currentFunctionIsVariadic;
    size_t currentFunctionFixedParams;
    const char* currentFunctionName;
} Scope;

Scope* createScope(Scope* parent);
void destroyScope(Scope* scope);
bool addToScope(Scope* scope, Symbol* sym);
Symbol* resolveInScopeChain(Scope* scope, const char* name);

#endif // SCOPE_H
