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

static ParsedType charType(bool isUnsigned) {
    ParsedType t;
    memset(&t, 0, sizeof(t));
    t.kind = TYPE_PRIMITIVE;
    t.primitiveType = TOKEN_CHAR;
    t.isUnsigned = isUnsigned;
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

static ParsedType sizedIntType(unsigned bits, bool isUnsigned) {
    if (bits >= 64) {
        ParsedType t = longType(isUnsigned);
        t.isLong = true;
        return t;
    }
    ParsedType t = intType();
    t.isUnsigned = isUnsigned;
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

static ParsedType charPtrType(void) {
    ParsedType t = charType(false);
    return pointerTo(t);
}

static ParsedType constCharPtrType(void) {
    ParsedType t = charType(false);
    t.isConst = true;
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
    sym->signature.hasPrototype = true;
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
        printfSym->signature.hasPrototype = true;
        addToScope(scope, printfSym);
    }

    Symbol* fprintfSym = makeBuiltin("fprintf", SYMBOL_FUNCTION, intType(), NULL);
    if (fprintfSym) {
        fprintfSym->signature.isVariadic = true;
        fprintfSym->signature.paramCount = 0;
        fprintfSym->signature.hasPrototype = true;
        addToScope(scope, fprintfSym);
    }

    Symbol* vsnprintfSym = makeBuiltin("vsnprintf", SYMBOL_FUNCTION, intType(), NULL);
    if (vsnprintfSym) {
        vsnprintfSym->signature.isVariadic = true;
        vsnprintfSym->signature.paramCount = 4;
        vsnprintfSym->signature.hasPrototype = true;
        vsnprintfSym->signature.params = (ParsedType*)calloc(4, sizeof(ParsedType));
        if (vsnprintfSym->signature.params) {
            vsnprintfSym->signature.params[0] = charPtrType();
            vsnprintfSym->signature.params[1] = longType(true);
            vsnprintfSym->signature.params[2] = constCharPtrType();
            vsnprintfSym->signature.params[3] = voidPtrType();
        }
        addToScope(scope, vsnprintfSym);
    }

    Symbol* exitSym = makeBuiltin("exit", SYMBOL_FUNCTION, voidType(), NULL);
    if (exitSym) {
        exitSym->signature.paramCount = 1;
        exitSym->signature.hasPrototype = true;
        addToScope(scope, exitSym);
    }

    Symbol* objSizeSym = makeBuiltin("__builtin_object_size", SYMBOL_FUNCTION, longType(true), NULL);
    if (objSizeSym) {
        objSizeSym->signature.paramCount = 2;
        objSizeSym->signature.hasPrototype = true;
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
        memcpyChkSym->signature.hasPrototype = true;
        memcpyChkSym->signature.params = (ParsedType*)calloc(4, sizeof(ParsedType));
        if (memcpyChkSym->signature.params) {
            memcpyChkSym->signature.params[0] = voidPtrType();
            memcpyChkSym->signature.params[1] = constVoidPtrType();
            memcpyChkSym->signature.params[2] = longType(true);
            memcpyChkSym->signature.params[3] = longType(true);
        }
        addToScope(scope, memcpyChkSym);
    }

    Symbol* memsetChkSym = makeBuiltin("__builtin___memset_chk", SYMBOL_FUNCTION, voidPtrType(), NULL);
    if (memsetChkSym) {
        memsetChkSym->signature.paramCount = 4;
        memsetChkSym->signature.hasPrototype = true;
        memsetChkSym->signature.params = (ParsedType*)calloc(4, sizeof(ParsedType));
        if (memsetChkSym->signature.params) {
            memsetChkSym->signature.params[0] = voidPtrType();
            memsetChkSym->signature.params[1] = intType();
            memsetChkSym->signature.params[2] = longType(true);
            memsetChkSym->signature.params[3] = longType(true);
        }
        addToScope(scope, memsetChkSym);
    }

    Symbol* allocaSym = makeBuiltin("__builtin_alloca", SYMBOL_FUNCTION, voidPtrType(), NULL);
    if (allocaSym) {
        allocaSym->signature.paramCount = 1;
        allocaSym->signature.hasPrototype = true;
        allocaSym->signature.params = (ParsedType*)calloc(1, sizeof(ParsedType));
        if (allocaSym->signature.params) {
            allocaSym->signature.params[0] = longType(true);
        }
        addToScope(scope, allocaSym);
    }

    Symbol* strncpyChkSym = makeBuiltin("__builtin___strncpy_chk", SYMBOL_FUNCTION, charPtrType(), NULL);
    if (strncpyChkSym) {
        strncpyChkSym->signature.paramCount = 4;
        strncpyChkSym->signature.hasPrototype = true;
        strncpyChkSym->signature.params = (ParsedType*)calloc(4, sizeof(ParsedType));
        if (strncpyChkSym->signature.params) {
            strncpyChkSym->signature.params[0] = charPtrType();
            strncpyChkSym->signature.params[1] = constCharPtrType();
            strncpyChkSym->signature.params[2] = longType(true);
            strncpyChkSym->signature.params[3] = longType(true);
        }
        addToScope(scope, strncpyChkSym);
    }

    Symbol* strncatChkSym = makeBuiltin("__builtin___strncat_chk", SYMBOL_FUNCTION, charPtrType(), NULL);
    if (strncatChkSym) {
        strncatChkSym->signature.paramCount = 4;
        strncatChkSym->signature.hasPrototype = true;
        strncatChkSym->signature.params = (ParsedType*)calloc(4, sizeof(ParsedType));
        if (strncatChkSym->signature.params) {
            strncatChkSym->signature.params[0] = charPtrType();
            strncatChkSym->signature.params[1] = constCharPtrType();
            strncatChkSym->signature.params[2] = longType(true);
            strncatChkSym->signature.params[3] = longType(true);
        }
        addToScope(scope, strncatChkSym);
    }

    Symbol* strncatSym = makeBuiltin("strncat", SYMBOL_FUNCTION, charPtrType(), NULL);
    if (strncatSym) {
        strncatSym->signature.paramCount = 3;
        strncatSym->signature.hasPrototype = true;
        strncatSym->signature.params = (ParsedType*)calloc(3, sizeof(ParsedType));
        if (strncatSym->signature.params) {
            strncatSym->signature.params[0] = charPtrType();
            strncatSym->signature.params[1] = constCharPtrType();
            strncatSym->signature.params[2] = longType(true);
        }
        addToScope(scope, strncatSym);
    }

    Symbol* strcatSym = makeBuiltin("strcat", SYMBOL_FUNCTION, charPtrType(), NULL);
    if (strcatSym) {
        strcatSym->signature.paramCount = 2;
        strcatSym->signature.hasPrototype = true;
        strcatSym->signature.params = (ParsedType*)calloc(2, sizeof(ParsedType));
        if (strcatSym->signature.params) {
            strcatSym->signature.params[0] = charPtrType();
            strcatSym->signature.params[1] = constCharPtrType();
        }
        addToScope(scope, strcatSym);
    }

    Symbol* snprintfChkSym = makeBuiltin("__builtin___snprintf_chk", SYMBOL_FUNCTION, intType(), NULL);
    if (snprintfChkSym) {
        snprintfChkSym->signature.isVariadic = true;
        snprintfChkSym->signature.paramCount = 5;
        snprintfChkSym->signature.hasPrototype = true;
        snprintfChkSym->signature.params = (ParsedType*)calloc(5, sizeof(ParsedType));
        if (snprintfChkSym->signature.params) {
            snprintfChkSym->signature.params[0] = charPtrType();
            snprintfChkSym->signature.params[1] = longType(true);
            snprintfChkSym->signature.params[2] = intType();
            snprintfChkSym->signature.params[3] = longType(true);
            snprintfChkSym->signature.params[4] = constCharPtrType();
        }
        addToScope(scope, snprintfChkSym);
    }

    Symbol* vsnprintfChkSym = makeBuiltin("__builtin___vsnprintf_chk", SYMBOL_FUNCTION, intType(), NULL);
    if (vsnprintfChkSym) {
        vsnprintfChkSym->signature.isVariadic = true;
        // The final argument is platform-specific va_list storage; accept flexibly.
        vsnprintfChkSym->signature.paramCount = 0;
        vsnprintfChkSym->signature.hasPrototype = false;
        addToScope(scope, vsnprintfChkSym);
    }

    Symbol* sprintfChkSym = makeBuiltin("__builtin___sprintf_chk", SYMBOL_FUNCTION, intType(), NULL);
    if (sprintfChkSym) {
        sprintfChkSym->signature.isVariadic = true;
        sprintfChkSym->signature.paramCount = 4;
        sprintfChkSym->signature.hasPrototype = true;
        sprintfChkSym->signature.params = (ParsedType*)calloc(4, sizeof(ParsedType));
        if (sprintfChkSym->signature.params) {
            sprintfChkSym->signature.params[0] = charPtrType();
            sprintfChkSym->signature.params[1] = intType();
            sprintfChkSym->signature.params[2] = longType(true);
            sprintfChkSym->signature.params[3] = constCharPtrType();
        }
        addToScope(scope, sprintfChkSym);
    }

    Symbol* strcpyChkSym = makeBuiltin("__builtin___strcpy_chk", SYMBOL_FUNCTION, charPtrType(), NULL);
    if (strcpyChkSym) {
        strcpyChkSym->signature.paramCount = 3;
        strcpyChkSym->signature.hasPrototype = true;
        strcpyChkSym->signature.params = (ParsedType*)calloc(3, sizeof(ParsedType));
        if (strcpyChkSym->signature.params) {
            strcpyChkSym->signature.params[0] = charPtrType();
            strcpyChkSym->signature.params[1] = constCharPtrType();
            strcpyChkSym->signature.params[2] = longType(true);
        }
        addToScope(scope, strcpyChkSym);
    }

    Symbol* strcatChkSym = makeBuiltin("__builtin___strcat_chk", SYMBOL_FUNCTION, charPtrType(), NULL);
    if (strcatChkSym) {
        strcatChkSym->signature.paramCount = 3;
        strcatChkSym->signature.hasPrototype = true;
        strcatChkSym->signature.params = (ParsedType*)calloc(3, sizeof(ParsedType));
        if (strcatChkSym->signature.params) {
            strcatChkSym->signature.params[0] = charPtrType();
            strcatChkSym->signature.params[1] = constCharPtrType();
            strcatChkSym->signature.params[2] = longType(true);
        }
        addToScope(scope, strcatChkSym);
    }

    Symbol* memmoveChkSym = makeBuiltin("__builtin___memmove_chk", SYMBOL_FUNCTION, voidPtrType(), NULL);
    if (memmoveChkSym) {
        memmoveChkSym->signature.paramCount = 4;
        memmoveChkSym->signature.hasPrototype = true;
        memmoveChkSym->signature.params = (ParsedType*)calloc(4, sizeof(ParsedType));
        if (memmoveChkSym->signature.params) {
            memmoveChkSym->signature.params[0] = voidPtrType();
            memmoveChkSym->signature.params[1] = constVoidPtrType();
            memmoveChkSym->signature.params[2] = longType(true);
            memmoveChkSym->signature.params[3] = longType(true);
        }
        addToScope(scope, memmoveChkSym);
    }

    Symbol* nanfSym = makeBuiltin("__builtin_nanf", SYMBOL_FUNCTION, floatType(false), NULL);
    if (nanfSym) {
        nanfSym->signature.paramCount = 1;
        nanfSym->signature.hasPrototype = true;
        nanfSym->signature.params = (ParsedType*)calloc(1, sizeof(ParsedType));
        if (nanfSym->signature.params) {
            nanfSym->signature.params[0] = constCharPtrType();
        }
        addToScope(scope, nanfSym);
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

    Symbol* atomicLoad = makeBuiltin("atomic_load_explicit", SYMBOL_FUNCTION, longType(true), NULL);
    if (atomicLoad) {
        atomicLoad->signature.isVariadic = true;
        atomicLoad->signature.paramCount = 0;
        atomicLoad->signature.hasPrototype = true;
        addToScope(scope, atomicLoad);
    }
    Symbol* atomicStore = makeBuiltin("atomic_store_explicit", SYMBOL_FUNCTION, voidType(), NULL);
    if (atomicStore) {
        atomicStore->signature.isVariadic = true;
        atomicStore->signature.paramCount = 0;
        atomicStore->signature.hasPrototype = true;
        addToScope(scope, atomicStore);
    }
    Symbol* atomicExchange = makeBuiltin("atomic_exchange_explicit", SYMBOL_FUNCTION, longType(true), NULL);
    if (atomicExchange) {
        atomicExchange->signature.isVariadic = true;
        atomicExchange->signature.paramCount = 0;
        atomicExchange->signature.hasPrototype = true;
        addToScope(scope, atomicExchange);
    }
    Symbol* atomicInit = makeBuiltin("atomic_init", SYMBOL_FUNCTION, voidType(), NULL);
    if (atomicInit) {
        atomicInit->signature.isVariadic = true;
        atomicInit->signature.paramCount = 0;
        atomicInit->signature.hasPrototype = true;
        addToScope(scope, atomicInit);
    }
    Symbol* c11AtomicLoad = makeBuiltin("__c11_atomic_load", SYMBOL_FUNCTION, longType(true), NULL);
    if (c11AtomicLoad) {
        c11AtomicLoad->signature.isVariadic = true;
        c11AtomicLoad->signature.paramCount = 0;
        c11AtomicLoad->signature.hasPrototype = true;
        addToScope(scope, c11AtomicLoad);
    }
    Symbol* c11AtomicStore = makeBuiltin("__c11_atomic_store", SYMBOL_FUNCTION, voidType(), NULL);
    if (c11AtomicStore) {
        c11AtomicStore->signature.isVariadic = true;
        c11AtomicStore->signature.paramCount = 0;
        c11AtomicStore->signature.hasPrototype = true;
        addToScope(scope, c11AtomicStore);
    }
    Symbol* c11AtomicExchange = makeBuiltin("__c11_atomic_exchange", SYMBOL_FUNCTION, longType(true), NULL);
    if (c11AtomicExchange) {
        c11AtomicExchange->signature.isVariadic = true;
        c11AtomicExchange->signature.paramCount = 0;
        c11AtomicExchange->signature.hasPrototype = true;
        addToScope(scope, c11AtomicExchange);
    }
    Symbol* c11AtomicInit = makeBuiltin("__c11_atomic_init", SYMBOL_FUNCTION, voidType(), NULL);
    if (c11AtomicInit) {
        c11AtomicInit->signature.isVariadic = true;
        c11AtomicInit->signature.paramCount = 0;
        c11AtomicInit->signature.hasPrototype = true;
        addToScope(scope, c11AtomicInit);
    }

    // Math-related builtins used in system headers
    ParsedType floatRet = floatType(false);
    ParsedType doubleRet = floatType(true);
    ParsedType longDoubleRet = floatType(true);
    longDoubleRet.isLong = true;

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

    // Branch prediction hint
    ParsedType expectArgs[2] = { longType(false), longType(false) };
    Symbol* expectSym = makeBuiltinFunc("__builtin_expect", longType(false), 2, expectArgs);
    if (expectSym) addToScope(scope, expectSym);

    Symbol* constantPSym = makeBuiltin("__builtin_constant_p", SYMBOL_FUNCTION, intType(), NULL);
    if (constantPSym) {
        // GCC/Clang treat this as a special builtin predicate that accepts arbitrary expressions.
        constantPSym->signature.isVariadic = true;
        constantPSym->signature.paramCount = 0;
        constantPSym->signature.hasPrototype = true;
        addToScope(scope, constantPSym);
    }

    ParsedType bswap32Arg[1] = { intType() };
    Symbol* bswap32 = makeBuiltinFunc("__builtin_bswap32", intType(), 1, bswap32Arg);
    if (bswap32) addToScope(scope, bswap32);

    ParsedType bswap64Arg[1] = { longType(true) };
    Symbol* bswap64 = makeBuiltinFunc("__builtin_bswap64", longType(true), 1, bswap64Arg);
    if (bswap64) addToScope(scope, bswap64);

    ParsedType bzeroArgs[2] = { voidPtrType(), longType(true) };
    Symbol* bzeroSym = makeBuiltinFunc("__builtin_bzero", voidType(), 2, bzeroArgs);
    if (bzeroSym) addToScope(scope, bzeroSym);

    Symbol* mulOverflow = makeBuiltin("__builtin_mul_overflow", SYMBOL_FUNCTION, intType(), NULL);
    if (mulOverflow) {
        mulOverflow->signature.isVariadic = true;
        mulOverflow->signature.paramCount = 0;
        mulOverflow->signature.hasPrototype = true;
        addToScope(scope, mulOverflow);
    }

    // Varargs helpers used by system headers/macros.
    ParsedType voidRet = voidType();
    Symbol* vaStart = makeBuiltinFunc("__builtin_va_start", voidRet, 0, NULL);
    if (vaStart) {
        vaStart->signature.isVariadic = true;
        addToScope(scope, vaStart);
    }
    Symbol* vaEnd = makeBuiltinFunc("__builtin_va_end", voidRet, 0, NULL);
    if (vaEnd) {
        vaEnd->signature.isVariadic = true;
        addToScope(scope, vaEnd);
    }
    Symbol* vaCopy = makeBuiltinFunc("__builtin_va_copy", voidRet, 0, NULL);
    if (vaCopy) {
        vaCopy->signature.isVariadic = true;
        addToScope(scope, vaCopy);
    }
    ParsedType vaArgParams[2];
    vaArgParams[0] = pointerTo(intType()); // loose pointer placeholder
    vaArgParams[1] = intType();            // placeholder; real type comes from call site
    Symbol* vaArgSym = makeBuiltinFunc("__builtin_va_arg", intType(), 2, vaArgParams);
    if (vaArgSym) {
        vaArgSym->signature.isVariadic = true;
        addToScope(scope, vaArgSym);
    }

    // Common typedefs for standalone builds without headers
    const TargetLayout* tl = scope && scope->ctx ? cc_get_target_layout(scope->ctx) : tl_default();
    unsigned ptrBits = tl ? (unsigned)tl->pointerBits : 64;
    ParsedType sizeLike = sizedIntType(ptrBits, true);
    ParsedType ssizeLike = sizedIntType(ptrBits, false);
    Symbol* sz  = makeTypedef("size_t",    sizeLike, scope->ctx);
    Symbol* ssz = makeTypedef("ssize_t",   ssizeLike, scope->ctx);
    Symbol* ip  = makeTypedef("intptr_t",  ssizeLike, scope->ctx);
    Symbol* uip = makeTypedef("uintptr_t", sizeLike, scope->ctx);
    Symbol* pd  = makeTypedef("ptrdiff_t", ssizeLike, scope->ctx);
    Symbol* wc  = makeTypedef("wchar_t",   intType(), scope->ctx);
    Symbol* wint = makeTypedef("wint_t",   intType(), scope->ctx);
    Symbol* sig = makeTypedef("sig_atomic_t", intType(), scope->ctx);
    ParsedType maxAlignType = longDoubleRet;
    if (tl && tl->longDoubleBits == 0) {
        maxAlignType = longType(true);
    }
    Symbol* maxa = makeTypedef("max_align_t", maxAlignType, scope->ctx);
    if (sz)  addToScope(scope, sz);
    if (ssz) addToScope(scope, ssz);
    if (ip)  addToScope(scope, ip);
    if (uip) addToScope(scope, uip);
    if (pd)  addToScope(scope, pd);
    if (wc)  addToScope(scope, wc);
    if (wint) addToScope(scope, wint);
    if (sig) addToScope(scope, sig);
    if (maxa) addToScope(scope, maxa);

    // NULL as a weak builtin integer macro substitute
    Symbol* nullSym = makeBuiltin("NULL", SYMBOL_VARIABLE, voidPtrType(), NULL);
    if (nullSym) addToScope(scope, nullSym);

    // Floating-point limit constants commonly injected by compilers
    Symbol* fltMin = makeBuiltin("__FLT_MIN__", SYMBOL_VARIABLE, floatType(false), NULL);
    Symbol* dblMin = makeBuiltin("__DBL_MIN__", SYMBOL_VARIABLE, floatType(true), NULL);
    Symbol* ldMin  = makeBuiltin("__LDBL_MIN__", SYMBOL_VARIABLE, longDoubleRet, NULL);
    if (fltMin) addToScope(scope, fltMin);
    if (dblMin) addToScope(scope, dblMin);
    if (ldMin)  addToScope(scope, ldMin);

}
