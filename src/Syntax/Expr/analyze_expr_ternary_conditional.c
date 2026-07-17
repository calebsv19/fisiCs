// SPDX-License-Identifier: Apache-2.0

#include "analyze_expr_internal.h"

#include "symbol_table.h"
#include "Syntax/syntax_errors.h"

#include <string.h>

static bool typeInfoIsFunctionPointerLike(const TypeInfo* info) {
    if (!info || !info->originalType) return false;
    if (info->originalType->isFunctionPointer) return true;
    if (info->pointerDepth <= 0) return false;
    ParsedType target = parsedTypePointerTargetType(info->originalType);
    bool hasFunction = false;
    for (size_t i = 0; i < target.derivationCount; ++i) {
        const TypeDerivation* d = parsedTypeGetDerivation(&target, i);
        if (d && d->kind == TYPE_DERIVATION_FUNCTION) {
            hasFunction = true;
            break;
        }
    }
    parsedTypeFree(&target);
    return hasFunction;
}

static bool functionPointerTypeHasPrototypeParameters(const TypeInfo* info) {
    if (!typeInfoIsFunctionPointerLike(info) || !info->originalType) {
        return false;
    }
    const ParsedType* params = NULL;
    size_t count = 0;
    bool isVariadic = false;
    bool hasPrototype = false;
    if (!parsedTypeGetEffectiveFunctionPointerSignature(info->originalType,
                                                        &params,
                                                        &count,
                                                        &isVariadic,
                                                        &hasPrototype)) {
        return false;
    }
    return hasPrototype;
}

static bool pointerTargetsCompatibleForConditional(const TypeInfo* a,
                                                   const TypeInfo* b,
                                                   Scope* scope) {
    if (!a || !b) return false;
    if (a->pointerDepth != b->pointerDepth) return false;
    if (typeInfoIsFunctionPointerLike(a) && typeInfoIsFunctionPointerLike(b)) {
        return canAssignTypesInScope(a, b, scope) == ASSIGN_OK &&
               canAssignTypesInScope(b, a, scope) == ASSIGN_OK;
    }
    if (a->recordDefinition && b->recordDefinition &&
        a->recordDefinition != b->recordDefinition) {
        return false;
    }
    if (a->primitive != b->primitive) return false;
    if (a->tag != b->tag) return false;
    if (a->userTypeName == b->userTypeName) return true;
    if (!a->userTypeName || !b->userTypeName) return true;
    return strcmp(a->userTypeName, b->userTypeName) == 0;
}

static const Symbol* resolveFunctionDesignatorSymbol(ASTNode* expr, Scope* scope) {
    if (!expr || !scope) return NULL;
    if (expr->type == AST_IDENTIFIER) {
        Symbol* sym = resolveInScopeChain(scope, expr->valueNode.value);
        if (sym && sym->kind == SYMBOL_FUNCTION) {
            return sym;
        }
        return NULL;
    }
    if (expr->type == AST_UNARY_EXPRESSION &&
        expr->expr.op &&
        strcmp(expr->expr.op, "&") == 0 &&
        expr->expr.left &&
        expr->expr.left->type == AST_IDENTIFIER) {
        Symbol* sym = resolveInScopeChain(scope, expr->expr.left->valueNode.value);
        if (sym && sym->kind == SYMBOL_FUNCTION) {
            return sym;
        }
    }
    return NULL;
}

static bool functionSymbolTypeCompatibleForConditional(const ParsedType* a,
                                                       const ParsedType* b,
                                                       Scope* scope) {
    if (!a || !b) return false;
    TypeInfo aInfo = typeInfoFromParsedType(a, scope);
    TypeInfo bInfo = typeInfoFromParsedType(b, scope);
    if (typeInfoIsFunctionPointerLike(&aInfo) &&
        typeInfoIsFunctionPointerLike(&bInfo)) {
        return canAssignTypesInScope(&aInfo, &bInfo, scope) == ASSIGN_OK &&
               canAssignTypesInScope(&bInfo, &aInfo, scope) == ASSIGN_OK;
    }
    return parsedTypesStructurallyEqual(a, b);
}

static bool functionSymbolsCompatibleForConditional(const Symbol* a,
                                                     const Symbol* b,
                                                     Scope* scope) {
    if (!a || !b || a->kind != SYMBOL_FUNCTION || b->kind != SYMBOL_FUNCTION) {
        return false;
    }
    if (!functionSymbolTypeCompatibleForConditional(&a->type, &b->type, scope)) {
        return false;
    }
    bool aHasParams = a->signature.paramCount > 0 || a->signature.isVariadic;
    bool bHasParams = b->signature.paramCount > 0 || b->signature.isVariadic;
    if (!aHasParams || !bHasParams) {
        return true;
    }
    if (a->signature.isVariadic != b->signature.isVariadic ||
        a->signature.paramCount != b->signature.paramCount) {
        return false;
    }
    for (size_t i = 0; i < a->signature.paramCount; ++i) {
        if (!functionSymbolTypeCompatibleForConditional(
                &a->signature.params[i], &b->signature.params[i], scope)) {
            return false;
        }
    }
    return true;
}

static bool typeInfoIsScalar(const TypeInfo* info) {
    return typeInfoIsArithmetic(info) || typeInfoIsPointerLike(info);
}

static TypeInfo mergePointerConditionalType(TypeInfo a, TypeInfo b) {
    TypeInfo result = a;
    result.isConst = a.isConst || b.isConst;
    result.isVolatile = a.isVolatile || b.isVolatile;
    result.isRestrict = a.isRestrict || b.isRestrict;
    int depth = result.pointerDepth;
    if (depth > TYPEINFO_MAX_POINTER_DEPTH) {
        depth = TYPEINFO_MAX_POINTER_DEPTH;
    }
    for (int i = 0; i < depth; ++i) {
        result.pointerLevels[i].isConst = a.pointerLevels[i].isConst || b.pointerLevels[i].isConst;
        result.pointerLevels[i].isVolatile = a.pointerLevels[i].isVolatile || b.pointerLevels[i].isVolatile;
        result.pointerLevels[i].isRestrict = a.pointerLevels[i].isRestrict || b.pointerLevels[i].isRestrict;
    }
    result.isLValue = false;
    return result;
}

TypeInfo analyzeTernaryExpression(ASTNode* node, Scope* scope) {
    TypeInfo condInfo = analyzeExpression(node->ternaryExpr.condition, scope);
    condInfo = decayToRValue(condInfo);
    TypeInfo trueInfo = decayToRValue(analyzeExpression(node->ternaryExpr.trueExpr, scope));
    TypeInfo falseInfo = decayToRValue(analyzeExpression(node->ternaryExpr.falseExpr, scope));
    bool conditionValid = true;
    if (condInfo.category != TYPEINFO_INVALID && !typeInfoIsScalar(&condInfo)) {
        addError(node->line, 0, "Conditional operator first operand must be scalar", NULL);
        conditionValid = false;
    }

    const Symbol* trueFnSym = resolveFunctionDesignatorSymbol(node->ternaryExpr.trueExpr, scope);
    const Symbol* falseFnSym = resolveFunctionDesignatorSymbol(node->ternaryExpr.falseExpr, scope);
    if (trueFnSym && falseFnSym) {
        bool fnCompat = functionSymbolsCompatibleForConditional(trueFnSym, falseFnSym, scope);
        if (!fnCompat &&
            conditionValid &&
            trueInfo.category != TYPEINFO_INVALID &&
            falseInfo.category != TYPEINFO_INVALID) {
            addError(node->line, 0, "Incompatible types in ternary expression", NULL);
            return makeInvalidType();
        }
    }

    bool ok = true;
    bool truePtr = typeInfoIsPointerLike(&trueInfo);
    bool falsePtr = typeInfoIsPointerLike(&falseInfo);
    bool trueNull = isNullPointerConstant(node->ternaryExpr.trueExpr, scope);
    bool falseNull = isNullPointerConstant(node->ternaryExpr.falseExpr, scope);

    if (typeInfoIsArithmetic(&trueInfo) && typeInfoIsArithmetic(&falseInfo)) {
        TypeInfo merged = usualArithmeticConversion(trueInfo, falseInfo, &ok);
        if (!ok) {
            if (typeInfoIsKnown(&trueInfo) || typeInfoIsKnown(&falseInfo)) {
                reportOperandError(node, "compatible operands", "?:");
            }
            return makeInvalidType();
        }
        merged.isLValue = false;
        return merged;
    }

    if ((trueInfo.category == TYPEINFO_STRUCT || trueInfo.category == TYPEINFO_UNION) &&
        (falseInfo.category == TYPEINFO_STRUCT || falseInfo.category == TYPEINFO_UNION) &&
        typesAreEqual(&trueInfo, &falseInfo)) {
        trueInfo.isConst = trueInfo.isConst || falseInfo.isConst;
        trueInfo.isVolatile = trueInfo.isVolatile || falseInfo.isVolatile;
        trueInfo.isRestrict = trueInfo.isRestrict || falseInfo.isRestrict;
        trueInfo.isLValue = false;
        return trueInfo;
    }

    if (trueInfo.category == TYPEINFO_VOID &&
        falseInfo.category == TYPEINFO_VOID &&
        trueInfo.pointerDepth == 0 &&
        falseInfo.pointerDepth == 0 &&
        !trueInfo.isFunction &&
        !falseInfo.isFunction) {
        TypeInfo mergedVoid = trueInfo;
        mergedVoid.isConst = mergedVoid.isConst || falseInfo.isConst;
        mergedVoid.isVolatile = mergedVoid.isVolatile || falseInfo.isVolatile;
        mergedVoid.isRestrict = mergedVoid.isRestrict || falseInfo.isRestrict;
        mergedVoid.isLValue = false;
        return mergedVoid;
    }

    TypeInfo truePtrInfo = trueInfo;
    TypeInfo falsePtrInfo = falseInfo;

    /*
     * A null pointer constant written as (void *)0 is itself pointer-like.
     * Select the non-null operand's type before the general two-pointer merge
     * so function-pointer conditionals retain their callable type.
     */
    if (truePtr && falseNull) {
        truePtrInfo.isLValue = false;
        return truePtrInfo;
    }
    if (falsePtr && trueNull) {
        falsePtrInfo.isLValue = false;
        return falsePtrInfo;
    }

    if (truePtr && falsePtr) {
        bool bothFunctionPointers =
            typeInfoIsFunctionPointerLike(&truePtrInfo) &&
            typeInfoIsFunctionPointerLike(&falsePtrInfo);
        if ((truePtrInfo.primitive == TOKEN_VOID && truePtrInfo.pointerDepth == falsePtrInfo.pointerDepth) ||
            (falsePtrInfo.primitive == TOKEN_VOID && truePtrInfo.pointerDepth == falsePtrInfo.pointerDepth)) {
            TypeInfo v = (truePtrInfo.primitive == TOKEN_VOID) ? truePtrInfo : falsePtrInfo;
            TypeInfo other = (truePtrInfo.primitive == TOKEN_VOID) ? falsePtrInfo : truePtrInfo;
            return mergePointerConditionalType(v, other);
        }
        if (!bothFunctionPointers && typesAreEqual(&truePtrInfo, &falsePtrInfo)) {
            return mergePointerConditionalType(truePtrInfo, falsePtrInfo);
        }
        if (pointerTargetsCompatibleForConditional(&truePtrInfo, &falsePtrInfo, scope)) {
            if (bothFunctionPointers) {
                bool trueHasPrototype =
                    functionPointerTypeHasPrototypeParameters(&truePtrInfo);
                bool falseHasPrototype =
                    functionPointerTypeHasPrototypeParameters(&falsePtrInfo);
                if (trueHasPrototype != falseHasPrototype) {
                    return trueHasPrototype
                        ? mergePointerConditionalType(truePtrInfo, falsePtrInfo)
                        : mergePointerConditionalType(falsePtrInfo, truePtrInfo);
                }
            }
            return mergePointerConditionalType(truePtrInfo, falsePtrInfo);
        }
    }

    if (conditionValid &&
        trueInfo.category != TYPEINFO_INVALID &&
        falseInfo.category != TYPEINFO_INVALID) {
        addError(node->line, 0, "Incompatible types in ternary expression", NULL);
    }
    return makeInvalidType();
}
