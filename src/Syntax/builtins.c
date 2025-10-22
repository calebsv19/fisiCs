#include "builtins.h"
#include "syntax_errors.h"
#include <stdlib.h>
#include <string.h>

static Symbol* makeBuiltin(const char* name, SymbolKind kind, ParsedType type, ASTNode* def) {
    Symbol* sym = (Symbol*)calloc(1, sizeof(Symbol));
    if (!sym) return NULL;
    sym->name = strdup(name);
    sym->kind = kind;
    sym->type = type;
    sym->definition = def;
    return sym;
}

static ParsedType intType(void) {
    ParsedType t;
    memset(&t, 0, sizeof(t));
    t.kind = TYPE_PRIMITIVE;
    t.primitiveType = TOKEN_INT;
    return t;
}

void seedBuiltins(Scope* globalScope) {
    if (!globalScope) return;

    Scope* scope = globalScope;

    Symbol* printfSym = makeBuiltin("printf", SYMBOL_FUNCTION, intType(), NULL);
    if (printfSym) addToScope(scope, printfSym);

    Symbol* fprintfSym = makeBuiltin("fprintf", SYMBOL_FUNCTION, intType(), NULL);
    if (fprintfSym) addToScope(scope, fprintfSym);

    Symbol* exitSym = makeBuiltin("exit", SYMBOL_FUNCTION, intType(), NULL);
    if (exitSym) addToScope(scope, exitSym);

    Symbol* trueSym = (Symbol*)calloc(1, sizeof(Symbol));
    if (trueSym) {
        trueSym->name = strdup("true");
        trueSym->kind = SYMBOL_VARIABLE;
        trueSym->type = intType();
        trueSym->definition = NULL;
        addToScope(scope, trueSym);
    }

    Symbol* falseSym = (Symbol*)calloc(1, sizeof(Symbol));
    if (falseSym) {
        falseSym->name = strdup("false");
        falseSym->kind = SYMBOL_VARIABLE;
        falseSym->type = intType();
        falseSym->definition = NULL;
        addToScope(scope, falseSym);
    }

    if (scope->ctx) {
        cc_add_typedef(scope->ctx, "bool");
    }
}
