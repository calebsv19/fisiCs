#ifndef SCOPE_H
#define SCOPE_H

#include "symbol_table.h"
#include "Compiler/compiler_context.h"
#include <stdlib.h>

typedef struct Scope {
    struct Scope* parent;
    SymbolTable table;
    int depth;
    CompilerContext* ctx;
} Scope;

Scope* createScope(Scope* parent);
void destroyScope(Scope* scope);
bool addToScope(Scope* scope, Symbol* sym);
Symbol* resolveInScopeChain(Scope* scope, const char* name);

#endif // SCOPE_H
