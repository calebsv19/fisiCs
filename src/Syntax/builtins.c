#include "builtins.h"
#include "Parser/Helpers/parsed_type.h"
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

static ParsedType longType(bool isUnsigned) {
    ParsedType t;
    memset(&t, 0, sizeof(t));
    t.kind = TYPE_PRIMITIVE;
    t.primitiveType = TOKEN_LONG;
    t.isLong = true;
    t.isUnsigned = isUnsigned;
    return t;
}

static ParsedType voidType(void) {
    ParsedType t;
    memset(&t, 0, sizeof(t));
    t.kind = TYPE_PRIMITIVE;
    t.primitiveType = TOKEN_VOID;
    return t;
}

static ParsedType floatType(bool isDouble) {
    ParsedType t;
    memset(&t, 0, sizeof(t));
    t.kind = TYPE_PRIMITIVE;
    t.primitiveType = isDouble ? TOKEN_DOUBLE : TOKEN_FLOAT;
    t.isLong = isDouble;
    return t;
}

static ParsedType pointerTo(ParsedType base) {
    parsedTypeAppendPointer(&base);
    return base;
}

static ParsedType constVoidPtrType(void) {
    ParsedType t = voidType();
    t.isConst = true;
    return pointerTo(t);
}

static ParsedType voidPtrType(void) {
    ParsedType t = voidType();
    return pointerTo(t);
}

static Symbol* makeTypedef(const char* name, ParsedType base, CompilerContext* ctx) {
    Symbol* sym = makeBuiltin(name, SYMBOL_TYPEDEF, base, NULL);
    if (sym && ctx) {
        cc_add_typedef(ctx, name);
    }
    return sym;
}

static Symbol* makeBuiltinFunc(const char* name, ParsedType retType, size_t paramCount, const ParsedType* params) {
    Symbol* sym = makeBuiltin(name, SYMBOL_FUNCTION, retType, NULL);
    if (!sym) return NULL;
    sym->signature.paramCount = paramCount;
    if (paramCount > 0 && params) {
        sym->signature.params = (ParsedType*)calloc(paramCount, sizeof(ParsedType));
        if (!sym->signature.params) return sym;
        for (size_t i = 0; i < paramCount; ++i) {
            sym->signature.params[i] = params[i];
        }
    }
    return sym;
}

void seedBuiltins(Scope* globalScope) {
    if (!globalScope) return;

    Scope* scope = globalScope;

    Symbol* printfSym = makeBuiltin("printf", SYMBOL_FUNCTION, intType(), NULL);
    if (printfSym) {
        printfSym->signature.isVariadic = true;
        printfSym->signature.paramCount = 0;
        addToScope(scope, printfSym);
    }

    Symbol* fprintfSym = makeBuiltin("fprintf", SYMBOL_FUNCTION, intType(), NULL);
    if (fprintfSym) {
        fprintfSym->signature.isVariadic = true;
        fprintfSym->signature.paramCount = 0;
        addToScope(scope, fprintfSym);
    }

    Symbol* exitSym = makeBuiltin("exit", SYMBOL_FUNCTION, voidType(), NULL);
    if (exitSym) {
        exitSym->signature.paramCount = 1;
        addToScope(scope, exitSym);
    }

    Symbol* objSizeSym = makeBuiltin("__builtin_object_size", SYMBOL_FUNCTION, longType(true), NULL);
    if (objSizeSym) {
        objSizeSym->signature.paramCount = 2;
        objSizeSym->signature.params = (ParsedType*)calloc(2, sizeof(ParsedType));
        if (objSizeSym->signature.params) {
            objSizeSym->signature.params[0] = constVoidPtrType();
            objSizeSym->signature.params[1] = intType();
        }
        addToScope(scope, objSizeSym);
    }

    Symbol* memcpyChkSym = makeBuiltin("__builtin___memcpy_chk", SYMBOL_FUNCTION, voidPtrType(), NULL);
    if (memcpyChkSym) {
        memcpyChkSym->signature.paramCount = 4;
        memcpyChkSym->signature.params = (ParsedType*)calloc(4, sizeof(ParsedType));
        if (memcpyChkSym->signature.params) {
            memcpyChkSym->signature.params[0] = voidPtrType();
            memcpyChkSym->signature.params[1] = constVoidPtrType();
            memcpyChkSym->signature.params[2] = longType(true);
            memcpyChkSym->signature.params[3] = longType(true);
        }
        addToScope(scope, memcpyChkSym);
    }

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

    // Math-related builtins used in system headers
    ParsedType floatRet = floatType(false);
    ParsedType doubleRet = floatType(true);
    ParsedType longDoubleRet = doubleRet; // treat long double as double for now

    ParsedType floatArg[1] = { floatRet };
    ParsedType doubleArg[1] = { doubleRet };
    ParsedType longDoubleArg[1] = { longDoubleRet };

    const char* mathFuncs[] = {
        "__builtin_fabsf", "__builtin_fabs", "__builtin_fabsl",
        "__builtin_inff", "__builtin_inf", "__builtin_infl"
    };
    Symbol* mathSyms[6] = {0};
    mathSyms[0] = makeBuiltinFunc(mathFuncs[0], floatRet, 1, floatArg);
    mathSyms[1] = makeBuiltinFunc(mathFuncs[1], doubleRet, 1, doubleArg);
    mathSyms[2] = makeBuiltinFunc(mathFuncs[2], longDoubleRet, 1, longDoubleArg);
    mathSyms[3] = makeBuiltinFunc(mathFuncs[3], floatRet, 0, NULL);
    mathSyms[4] = makeBuiltinFunc(mathFuncs[4], doubleRet, 0, NULL);
    mathSyms[5] = makeBuiltinFunc(mathFuncs[5], longDoubleRet, 0, NULL);
    for (size_t i = 0; i < 6; ++i) {
        if (mathSyms[i]) addToScope(scope, mathSyms[i]);
    }

    // Common typedefs for standalone builds without headers
    Symbol* sz  = makeTypedef("size_t",    longType(true), scope->ctx);
    Symbol* ssz = makeTypedef("ssize_t",   longType(false), scope->ctx);
    Symbol* ip  = makeTypedef("intptr_t",  longType(false), scope->ctx);
    Symbol* uip = makeTypedef("uintptr_t", longType(true), scope->ctx);
    Symbol* pd  = makeTypedef("ptrdiff_t", longType(false), scope->ctx);
    Symbol* wc  = makeTypedef("wchar_t",   intType(), scope->ctx);
    Symbol* wint = makeTypedef("wint_t",   intType(), scope->ctx);
    Symbol* sig = makeTypedef("sig_atomic_t", intType(), scope->ctx);
    if (sz)  addToScope(scope, sz);
    if (ssz) addToScope(scope, ssz);
    if (ip)  addToScope(scope, ip);
    if (uip) addToScope(scope, uip);
    if (pd)  addToScope(scope, pd);
    if (wc)  addToScope(scope, wc);
    if (wint) addToScope(scope, wint);
    if (sig) addToScope(scope, sig);

    // NULL as a weak builtin integer macro substitute
    Symbol* nullSym = makeBuiltin("NULL", SYMBOL_VARIABLE, voidPtrType(), NULL);
    if (nullSym) addToScope(scope, nullSym);

    // Floating-point limit constants commonly injected by compilers
    Symbol* fltMin = makeBuiltin("__FLT_MIN__", SYMBOL_VARIABLE, floatType(false), NULL);
    Symbol* dblMin = makeBuiltin("__DBL_MIN__", SYMBOL_VARIABLE, floatType(true), NULL);
    Symbol* ldMin  = makeBuiltin("__LDBL_MIN__", SYMBOL_VARIABLE, floatType(true), NULL);
    if (fltMin) addToScope(scope, fltMin);
    if (dblMin) addToScope(scope, dblMin);
    if (ldMin)  addToScope(scope, ldMin);

    if (scope->ctx) {
        cc_add_typedef(scope->ctx, "bool");
    }
}
