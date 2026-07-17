// SPDX-License-Identifier: Apache-2.0

#include "analyze_decls_internal.h"
#include "Utils/profiler.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static int countParameterDeclarators(ASTNode** params, size_t paramCount);
static bool isVoidParameterDecl(ASTNode* param);
static bool isSyntheticUnnamedParameterName(const char* name);
static void stripTopLevelParameterQualifiers(ParsedType* type);
static void normalizeFunctionSignatureParameter(ParsedType* type,
                                                Scope* scope,
                                                bool canonicalizeAliases);
static bool parameterNameAlreadySeen(char** names, size_t count, const char* candidate);
static void parameterNameRemember(char*** names, size_t* count, size_t* capacity, char* name);
static int ascii_tolower(int c);
static void lower_inplace(char* s);
static int countStorageSpecifiers(const ParsedType* type);

void resetFunctionSignature(Symbol* sym) {
    if (!sym) return;
    sym->signature.params = NULL;
    sym->signature.paramCount = 0;
    sym->signature.isVariadic = false;
    sym->signature.hasPrototype = false;
}

void freeFunctionSignatureParameters(FunctionSignature* signature) {
    if (!signature || !signature->params) return;
    for (size_t i = 0; i < signature->paramCount; ++i) {
        parsedTypeFree(&signature->params[i]);
    }
    free(signature->params);
    signature->params = NULL;
    signature->paramCount = 0;
}

static bool arrayDerivationIsIncomplete(const TypeDerivation* deriv) {
    return deriv &&
           deriv->kind == TYPE_DERIVATION_ARRAY &&
           !deriv->as.array.hasConstantSize &&
           !deriv->as.array.isVLA &&
           !deriv->as.array.isFlexible &&
           deriv->as.array.sizeExpr == NULL;
}

static bool arrayDerivationHasKnownBound(const TypeDerivation* deriv) {
    return deriv &&
           deriv->kind == TYPE_DERIVATION_ARRAY &&
           deriv->as.array.hasConstantSize &&
           !deriv->as.array.isVLA &&
           !deriv->as.array.isFlexible;
}

static void freeParsedTypeArray(ParsedType* params, size_t count) {
    if (!params) return;
    for (size_t i = 0; i < count; ++i) {
        parsedTypeFree(&params[i]);
    }
    free(params);
}

static bool adoptNestedFunctionPrototype(TypeDerivation* accumulated,
                                         const TypeDerivation* incoming) {
    if (!accumulated || !incoming ||
        accumulated->kind != TYPE_DERIVATION_FUNCTION ||
        incoming->kind != TYPE_DERIVATION_FUNCTION ||
        accumulated->as.function.hasPrototype ||
        !incoming->as.function.hasPrototype) {
        return false;
    }

    size_t count = incoming->as.function.paramCount;
    ParsedType* params = NULL;
    if (count > 0) {
        if (!incoming->as.function.params) return false;
        params = calloc(count, sizeof(ParsedType));
        if (!params) return false;
        for (size_t i = 0; i < count; ++i) {
            params[i] = parsedTypeClone(&incoming->as.function.params[i]);
            if (params[i].kind == TYPE_INVALID &&
                incoming->as.function.params[i].kind != TYPE_INVALID) {
                freeParsedTypeArray(params, i + 1);
                return false;
            }
        }
    }

    freeParsedTypeArray(accumulated->as.function.params,
                        accumulated->as.function.paramCount);
    accumulated->as.function.params = params;
    accumulated->as.function.paramCount = count;
    accumulated->as.function.isVariadic = incoming->as.function.isVariadic;
    accumulated->as.function.hasPrototype = true;
    return true;
}

static void mergeCompatibleParsedTypeDetails(ParsedType* accumulated,
                                             const ParsedType* incoming,
                                             unsigned depth) {
    if (!accumulated || !incoming || depth > 32) return;

    size_t commonDerivations = accumulated->derivationCount < incoming->derivationCount
                                   ? accumulated->derivationCount
                                   : incoming->derivationCount;
    for (size_t i = 0; i < commonDerivations; ++i) {
        TypeDerivation* current = &accumulated->derivations[i];
        const TypeDerivation* next = &incoming->derivations[i];
        if (current->kind != next->kind) continue;
        if (arrayDerivationIsIncomplete(current) &&
            arrayDerivationHasKnownBound(next)) {
            current->as.array = next->as.array;
            continue;
        }
        if (current->kind == TYPE_DERIVATION_FUNCTION) {
            (void)adoptNestedFunctionPrototype(current, next);
        }
        if (current->kind != TYPE_DERIVATION_FUNCTION ||
            !current->as.function.hasPrototype ||
            !next->as.function.hasPrototype ||
            current->as.function.paramCount != next->as.function.paramCount) {
            continue;
        }
        for (size_t p = 0; p < current->as.function.paramCount; ++p) {
            mergeCompatibleParsedTypeDetails(&current->as.function.params[p],
                                             &next->as.function.params[p],
                                             depth + 1);
        }
    }

    if (accumulated->hasParamArrayInfo && incoming->hasParamArrayInfo &&
        !accumulated->paramArrayInfo.hasConstantSize &&
        !accumulated->paramArrayInfo.isVLA &&
        !accumulated->paramArrayInfo.isFlexible &&
        accumulated->paramArrayInfo.sizeExpr == NULL &&
        incoming->paramArrayInfo.hasConstantSize &&
        !incoming->paramArrayInfo.isVLA &&
        !incoming->paramArrayInfo.isFlexible) {
        accumulated->paramArrayInfo = incoming->paramArrayInfo;
    }
}

void mergeCompatibleParsedTypeDetailsInScope(ParsedType* accumulated,
                                             const ParsedType* incoming,
                                             Scope* scope) {
    if (!accumulated || !incoming || !scope) return;

    ParsedType current = parsedTypeClone(accumulated);
    ParsedType next = parsedTypeClone(incoming);
    if ((current.kind == TYPE_INVALID && accumulated->kind != TYPE_INVALID) ||
        (next.kind == TYPE_INVALID && incoming->kind != TYPE_INVALID)) {
        parsedTypeFree(&current);
        parsedTypeFree(&next);
        return;
    }

    canonicalizeParsedTypeInScope(&current, scope);
    canonicalizeParsedTypeInScope(&next, scope);
    mergeCompatibleParsedTypeDetails(&current, &next, 0);
    parsedTypeFree(accumulated);
    *accumulated = current;
    parsedTypeFree(&next);
}

void mergeCompatibleFunctionSignatures(FunctionSignature* accumulated,
                                       FunctionSignature* incoming) {
    if (!accumulated || !incoming) return;

    if (!accumulated->hasPrototype && incoming->hasPrototype) {
        freeFunctionSignatureParameters(accumulated);
        *accumulated = *incoming;
        incoming->params = NULL;
        incoming->paramCount = 0;
        return;
    }
    if (!accumulated->hasPrototype || !incoming->hasPrototype ||
        accumulated->paramCount != incoming->paramCount ||
        !accumulated->params || !incoming->params) {
        return;
    }
    for (size_t i = 0; i < accumulated->paramCount; ++i) {
        mergeCompatibleParsedTypeDetails(&accumulated->params[i],
                                         &incoming->params[i],
                                         0);
    }
}

static int countParameterDeclarators(ASTNode** params, size_t paramCount) {
    int total = 0;
    if (!params) return 0;
    for (size_t i = 0; i < paramCount; ++i) {
        ASTNode* param = params[i];
        if (!param || param->type != AST_VARIABLE_DECLARATION) {
            continue;
        }
        total += (int)param->varDecl.varCount;
    }
    return total;
}

static bool isVoidParameterDecl(ASTNode* param) {
    if (!param || param->type != AST_VARIABLE_DECLARATION) {
        return false;
    }
    const ParsedType* type = &param->varDecl.declaredType;
    return type->kind == TYPE_PRIMITIVE &&
           type->primitiveType == TOKEN_VOID &&
           type->pointerDepth == 0 &&
           param->varDecl.varCount == 1;
}

bool parsedTypeIsPlainVoid(const ParsedType* type) {
    return type &&
           type->kind == TYPE_PRIMITIVE &&
           type->primitiveType == TOKEN_VOID &&
           type->pointerDepth == 0 &&
           type->derivationCount == 0;
}

bool parsedTypeIsDirectFunction(const ParsedType* type) {
    return type &&
           type->derivationCount > 0 &&
           type->derivations &&
           type->derivations[0].kind == TYPE_DERIVATION_FUNCTION;
}

static bool isSyntheticUnnamedParameterName(const char* name) {
    static const char* kPrefix = "__unnamed_param";
    return name && strncmp(name, kPrefix, strlen(kPrefix)) == 0;
}

static void stripTopLevelParameterQualifiers(ParsedType* type) {
    if (!type) return;

    /* C function type compatibility ignores top-level qualifiers on parameters
       after array/function adjustment. */
    bool pointerLike =
        (type->pointerDepth > 0) ||
        (type->derivationCount > 0 &&
         type->derivations &&
         type->derivations[0].kind == TYPE_DERIVATION_POINTER);

    /* For pointer-typed parameters, ParsedType base qualifiers represent the
       pointed-to type, not the parameter's own top-level qualifiers. */
    if (!pointerLike) {
        type->isConst = false;
        type->isVolatile = false;
        type->isRestrict = false;
    }

    if (type->derivationCount == 0 || !type->derivations) {
        return;
    }

    TypeDerivation* outer = &type->derivations[0];
    if (outer->kind == TYPE_DERIVATION_POINTER) {
        outer->as.pointer.isConst = false;
        outer->as.pointer.isVolatile = false;
        outer->as.pointer.isRestrict = false;
    }
}

static void normalizeFunctionSignatureParameter(ParsedType* type,
                                                Scope* scope,
                                                bool canonicalizeAliases) {
    if (!type) return;

    if (canonicalizeAliases) {
        canonicalizeParsedTypeInScope(type, scope);
    } else {
        parsedTypeResolvePlainNamedTypedefInScope(type, scope);
    }
    evaluateArrayDerivations(type, scope);
    parsedTypeAdjustFunctionParameter(type);
    stripTopLevelParameterQualifiers(type);
}

static bool parameterNameAlreadySeen(char** names, size_t count, const char* candidate) {
    if (!candidate || !candidate[0]) return false;
    for (size_t i = 0; i < count; ++i) {
        if (names[i] && strcmp(names[i], candidate) == 0) {
            return true;
        }
    }
    return false;
}

static void parameterNameRemember(char*** names,
                                  size_t* count,
                                  size_t* capacity,
                                  char* name) {
    if (!names || !count || !capacity || !name || !name[0]) {
        return;
    }
    if (*count >= *capacity) {
        size_t newCap = *capacity ? (*capacity * 2) : 8;
        char** grown = realloc(*names, newCap * sizeof(char*));
        if (!grown) {
            return;
        }
        *names = grown;
        *capacity = newCap;
    }
    (*names)[(*count)++] = name;
}

bool validateFunctionParameters(ASTNode** params,
                                size_t paramCount,
                                bool isVariadic,
                                Scope* scope,
                                int line,
                                const char* funcName) {
    if (!params || paramCount == 0) {
        return true;
    }
    bool ok = true;
    bool sawVoidParameter = false;
    bool sawNonVoidParameter = false;
    char** seenNames = NULL;
    size_t seenCount = 0;
    size_t seenCapacity = 0;

    for (size_t i = 0; i < paramCount; ++i) {
        ASTNode* param = params[i];
        if (!param || param->type != AST_VARIABLE_DECLARATION) {
            continue;
        }
        ParsedType* perTypes = param->varDecl.declaredTypes;
        ASTNode** varNames = param->varDecl.varNames;
        for (size_t k = 0; k < param->varDecl.varCount; ++k) {
            const ParsedType* paramType = perTypes ? &perTypes[k] : &param->varDecl.declaredType;
            ASTNode* nameNode = varNames ? varNames[k] : NULL;
            const char* paramName =
                (nameNode && nameNode->type == AST_IDENTIFIER) ? nameNode->valueNode.value : NULL;
            if (isSyntheticUnnamedParameterName(paramName)) {
                paramName = NULL;
            }
            SourceRange paramLoc =
                nameNode ? nameNode->location : (param ? param->location : (SourceRange){0});
            SourceRange paramCallSite =
                nameNode ? nameNode->macroCallSite
                         : (param ? param->macroCallSite : (SourceRange){0});
            SourceRange paramDefSite =
                nameNode ? nameNode->macroDefinition
                         : (param ? param->macroDefinition : (SourceRange){0});

            StorageClass storage = deduceStorageClass(paramType);
            if (storage == STORAGE_EXTERN || storage == STORAGE_AUTO) {
                addErrorWithRanges(paramLoc,
                                   paramCallSite,
                                   paramDefSite,
                                   "Invalid storage class for function parameter",
                                   paramName ? paramName : funcName);
                ok = false;
            } else if (storage == STORAGE_STATIC && !parsedTypeIsDirectArray(paramType)) {
                addErrorWithRanges(paramLoc,
                                   paramCallSite,
                                   paramDefSite,
                                   "Invalid use of static storage class in parameter declaration",
                                   paramName ? paramName : funcName);
                ok = false;
            }

            if (parsedTypeIsPlainVoid(paramType)) {
                if (paramName && paramName[0]) {
                    addErrorWithRanges(paramLoc,
                                       paramCallSite,
                                       paramDefSite,
                                       "Parameter declared with type void must not have a name",
                                       paramName);
                    ok = false;
                }
                sawVoidParameter = true;
            } else {
                sawNonVoidParameter = true;
            }

            if (paramName && paramName[0]) {
                if (parameterNameAlreadySeen(seenNames, seenCount, paramName)) {
                    addErrorWithRanges(paramLoc,
                                       paramCallSite,
                                       paramDefSite,
                                       "Duplicate parameter name",
                                       paramName);
                    ok = false;
                } else {
                    parameterNameRemember(&seenNames,
                                          &seenCount,
                                          &seenCapacity,
                                          nameNode->valueNode.value);
                }
            }

            profiler_record_value("semantic_count_type_info_site_decl", 1);
            TypeInfo info = typeInfoFromParsedType(paramType, scope);
            bool directArray = parsedTypeIsDirectArray(paramType);
            bool directFunction = parsedTypeIsDirectFunction(paramType);
            if (!directArray &&
                !directFunction &&
                paramType->pointerDepth == 0 &&
                (info.category == TYPEINFO_STRUCT || info.category == TYPEINFO_UNION) &&
                !info.isComplete) {
                addErrorWithRanges(paramLoc,
                                   paramCallSite,
                                   paramDefSite,
                                   "Function parameter has incomplete type",
                                   paramName ? paramName : funcName);
                ok = false;
            }
        }
    }

    free(seenNames);

    if (sawVoidParameter && (sawNonVoidParameter || isVariadic)) {
        addError(line, 0, "Parameter list cannot combine 'void' with other parameters", funcName);
        ok = false;
    }

    return ok;
}

void assignFunctionSignature(Symbol* sym,
                             ASTNode** params,
                             size_t paramCount,
                             bool isVariadic,
                             bool hasPrototype,
                             Scope* scope) {
    if (!sym) return;
    freeFunctionSignatureParameters(&sym->signature);
    sym->signature.params = NULL;
    sym->signature.paramCount = 0;
    sym->signature.isVariadic = isVariadic;
    sym->signature.hasPrototype = false;
    sym->signature.callConv = CALLCONV_DEFAULT;

    if (!params || paramCount == 0) {
        sym->signature.hasPrototype = hasPrototype;
        return;
    }

    int totalDecls = countParameterDeclarators(params, paramCount);
    if (totalDecls <= 0) {
        return;
    }

    if (totalDecls == 1 && isVoidParameterDecl(params[0]) && !isVariadic) {
        sym->signature.hasPrototype = true;
        return;
    }

    sym->signature.params = calloc((size_t)totalDecls, sizeof(ParsedType));
    if (!sym->signature.params) {
        sym->signature.paramCount = 0;
        return;
    }

    size_t idx = 0;
    for (size_t i = 0; i < paramCount; ++i) {
        ASTNode* param = params[i];
        if (!param || param->type != AST_VARIABLE_DECLARATION) {
            continue;
        }
        ParsedType* perTypes = param->varDecl.declaredTypes;
        for (size_t k = 0; k < param->varDecl.varCount; ++k) {
            if (idx < (size_t)totalDecls) {
                const ParsedType* srcType = perTypes ? &perTypes[k] : &param->varDecl.declaredType;
                ParsedType adjusted = parsedTypeClone(srcType);
                normalizeFunctionSignatureParameter(&adjusted, scope, false);
                sym->signature.params[idx] = adjusted;
                idx++;
            }
        }
    }
    sym->signature.paramCount = idx;
    sym->signature.hasPrototype = hasPrototype;
}

void assignFunctionSignatureFromParsedType(Symbol* sym,
                                           const ParsedType* functionType,
                                           Scope* scope) {
    if (!sym || !functionType || !parsedTypeIsDirectFunction(functionType)) {
        return;
    }

    const TypeDerivation* function = &functionType->derivations[0];
    resetFunctionSignature(sym);
    sym->signature.isVariadic = function->as.function.isVariadic;
    sym->signature.hasPrototype = function->as.function.hasPrototype;
    sym->signature.callConv = CALLCONV_DEFAULT;

    size_t count = function->as.function.paramCount;
    const ParsedType* params = function->as.function.params;
    if (count == 1 && params && parsedTypeIsPlainVoid(&params[0]) &&
        !function->as.function.isVariadic) {
        sym->signature.hasPrototype = true;
        return;
    }
    if (count == 0 || !params) {
        return;
    }

    sym->signature.params = calloc(count, sizeof(ParsedType));
    if (!sym->signature.params) {
        return;
    }
    for (size_t i = 0; i < count; ++i) {
        ParsedType adjusted = parsedTypeClone(&params[i]);
        normalizeFunctionSignatureParameter(&adjusted, scope, true);
        sym->signature.params[i] = adjusted;
    }
    sym->signature.paramCount = count;
}

static bool prototypeAggregateIdentityConflictsShallow(const ParsedType* lhs,
                                                       const ParsedType* rhs,
                                                       Scope* scope) {
    if (!lhs || !rhs || !scope) return false;

    TypeInfo lhsInfo = typeInfoFromParsedType(lhs, scope);
    TypeInfo rhsInfo = typeInfoFromParsedType(rhs, scope);
    if (lhsInfo.tag == TAG_NONE || rhsInfo.tag == TAG_NONE) {
        return false;
    }
    if (lhsInfo.tag != rhsInfo.tag) {
        return true;
    }
    if (lhsInfo.recordDefinition && rhsInfo.recordDefinition) {
        return lhsInfo.recordDefinition != rhsInfo.recordDefinition;
    }
    /* Anonymous enum typedefs carry their declaration identity through the
       defining AST node, just as struct/union types do through
       recordDefinition.  Their typedef spelling is an ordinary identifier,
       not a visible enum tag, so falling through to the prototype-scope tag
       check would incorrectly treat two uses of the same typedef as distinct
       types. */
    if (lhsInfo.tag == TAG_ENUM && rhsInfo.tag == TAG_ENUM &&
        lhs->inlineEnumDef && rhs->inlineEnumDef) {
        return lhs->inlineEnumDef != rhs->inlineEnumDef;
    }

    const char* lhsName = lhsInfo.userTypeName;
    const char* rhsName = rhsInfo.userTypeName;
    Symbol* lhsVisible = lhsName ? resolveTagInScopeChain(scope, lhsName) : NULL;
    Symbol* rhsVisible = rhsName ? resolveTagInScopeChain(scope, rhsName) : NULL;
    bool lhsKnown = lhsVisible != NULL;
    bool rhsKnown = rhsVisible != NULL;
    if (scope->ctx) {
        CCTagKind lhsKind = lhsInfo.tag == TAG_UNION
                                ? CC_TAG_UNION
                                : (lhsInfo.tag == TAG_ENUM ? CC_TAG_ENUM : CC_TAG_STRUCT);
        CCTagKind rhsKind = rhsInfo.tag == TAG_UNION
                                ? CC_TAG_UNION
                                : (rhsInfo.tag == TAG_ENUM ? CC_TAG_ENUM : CC_TAG_STRUCT);
        lhsKnown = lhsKnown || (lhsName && cc_has_tag(scope->ctx, lhsKind, lhsName));
        rhsKnown = rhsKnown || (rhsName && cc_has_tag(scope->ctx, rhsKind, rhsName));
    }

    /*
     * A tag first mentioned inside a function prototype has prototype scope.
     * If neither parameter type resolves to a visible tag, two declarations
     * introduce distinct types even when their tag spelling is identical.
     */
    return !lhsKnown || !rhsKnown;
}

static bool prototypeParameterAggregateIdentityConflicts(const ParsedType* lhs,
                                                         const ParsedType* rhs,
                                                         Scope* scope,
                                                         unsigned depth) {
    if (!lhs || !rhs || !scope || depth > 32) return false;
    if (prototypeAggregateIdentityConflictsShallow(lhs, rhs, scope)) {
        return true;
    }

    const ParsedType* lhsParams = NULL;
    const ParsedType* rhsParams = NULL;
    size_t lhsCount = 0;
    size_t rhsCount = 0;
    bool lhsVariadic = false;
    bool rhsVariadic = false;
    bool lhsHasFunction = parsedTypeGetEffectiveFunctionPointerSignature(
        lhs, &lhsParams, &lhsCount, &lhsVariadic, NULL);
    bool rhsHasFunction = parsedTypeGetEffectiveFunctionPointerSignature(
        rhs, &rhsParams, &rhsCount, &rhsVariadic, NULL);
    if (!lhsHasFunction || !rhsHasFunction) {
        return false;
    }

    ParsedType lhsReturn = parsedTypeFunctionReturnType(lhs);
    ParsedType rhsReturn = parsedTypeFunctionReturnType(rhs);
    bool returnConflict =
        lhsReturn.kind != TYPE_INVALID &&
        rhsReturn.kind != TYPE_INVALID &&
        prototypeParameterAggregateIdentityConflicts(
            &lhsReturn, &rhsReturn, scope, depth + 1);
    parsedTypeFree(&lhsReturn);
    parsedTypeFree(&rhsReturn);
    if (returnConflict) {
        return true;
    }

    size_t commonCount = lhsCount < rhsCount ? lhsCount : rhsCount;
    for (size_t i = 0; i < commonCount; ++i) {
        if (prototypeParameterAggregateIdentityConflicts(
                &lhsParams[i], &rhsParams[i], scope, depth + 1)) {
            return true;
        }
    }
    return false;
}

static bool prototypeParametersSurviveDefaultPromotions(
    const FunctionSignature* signature,
    Scope* scope) {
    if (!signature || !signature->hasPrototype || signature->isVariadic) {
        return false;
    }
    if (signature->paramCount > 0 && !signature->params) {
        return false;
    }
    for (size_t i = 0; i < signature->paramCount; ++i) {
        TypeInfo declared = typeInfoFromParsedType(&signature->params[i], scope);
        TypeInfo promoted = defaultArgumentPromotion(declared);
        if (!typesAreEqual(&declared, &promoted)) {
            return false;
        }
    }
    return true;
}

static bool prototypeCompatibleWithOldStyleDefinition(
    const FunctionSignature* prototype,
    const FunctionSignature* oldStyle,
    Scope* scope) {
    if (!prototype || !oldStyle || !prototype->hasPrototype ||
        oldStyle->hasPrototype || oldStyle->paramCount == 0) {
        return false;
    }
    if (prototype->isVariadic || oldStyle->isVariadic ||
        prototype->paramCount != oldStyle->paramCount ||
        !prototype->params || !oldStyle->params) {
        return false;
    }
    for (size_t i = 0; i < prototype->paramCount; ++i) {
        if (parsedTypesStructurallyCompatibleInScope(&prototype->params[i],
                                                     &oldStyle->params[i],
                                                     scope)) {
            continue;
        }
        TypeInfo declared = typeInfoFromParsedType(&oldStyle->params[i], scope);
        TypeInfo promoted = defaultArgumentPromotion(declared);
        TypeInfo expected = typeInfoFromParsedType(&prototype->params[i], scope);
        if (!typesAreEqual(&expected, &promoted)) {
            return false;
        }
    }
    return true;
}

bool functionSignaturesCompatible(const FunctionSignature* lhs,
                                  const FunctionSignature* rhs,
                                  Scope* scope) {
    if (!lhs || !rhs) return true;
    if (!lhs->hasPrototype || !rhs->hasPrototype) {
        if (!lhs->hasPrototype && !rhs->hasPrototype) {
            return true;
        }
        const FunctionSignature* prototype = lhs->hasPrototype ? lhs : rhs;
        const FunctionSignature* oldStyle = lhs->hasPrototype ? rhs : lhs;
        if (oldStyle->paramCount > 0) {
            return prototypeCompatibleWithOldStyleDefinition(
                prototype, oldStyle, scope);
        }
        return prototypeParametersSurviveDefaultPromotions(prototype, scope);
    }
    if ((lhs->paramCount > 0 && !lhs->params) || (rhs->paramCount > 0 && !rhs->params)) {
        return true;
    }
    if (lhs->paramCount != rhs->paramCount) return false;
    if (lhs->isVariadic != rhs->isVariadic) return false;
    for (size_t i = 0; i < lhs->paramCount; ++i) {
        if (prototypeParameterAggregateIdentityConflicts(
                &lhs->params[i], &rhs->params[i], scope, 0)) {
            return false;
        }
        if (!parsedTypesStructurallyCompatibleInScope(&lhs->params[i], &rhs->params[i], scope)) {
            return false;
        }
    }
    return true;
}

const char* safeIdentifierName(ASTNode* node) {
    if (node && node->type == AST_IDENTIFIER && node->valueNode.value) {
        return node->valueNode.value;
    }
    return "<unnamed>";
}

bool scopeIsFileScope(Scope* scope) {
    return scope && scope->parent == NULL;
}

static int ascii_tolower(int c) {
    if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
    return c;
}

static void lower_inplace(char* s) {
    if (!s) return;
    for (char* p = s; *p; ++p) {
        *p = (char)ascii_tolower((unsigned char)*p);
    }
}

static bool attr_payload_has_word(const char* payload, const char* word) {
    if (!payload || !word || !word[0]) return false;
    size_t wordLength = strlen(word);
    const char* match = payload;
    while ((match = strstr(match, word)) != NULL) {
        char left = match == payload ? '\0' : match[-1];
        char right = match[wordLength];
        bool leftBoundary = left == '\0' ||
                            (!isalnum((unsigned char)left) && left != '_');
        bool rightBoundary = right == '\0' ||
                             (!isalnum((unsigned char)right) && right != '_');
        if (leftBoundary && rightBoundary) return true;
        match += wordLength;
    }
    return false;
}

void applyInteropAttributes(Symbol* sym, ASTNode* node, Scope* scope, bool allowWarn) {
    if (!sym || !node || node->attributeCount == 0 || !node->attributes) return;
    CompilerContext* ctx = scope ? scope->ctx : NULL;
    const TargetLayout* tl = ctx ? cc_get_target_layout(ctx) : NULL;
    bool warnIgnored = allowWarn && cc_warn_ignored_interop(ctx);
    bool errorIgnored = cc_error_ignored_interop(ctx);
    for (size_t i = 0; i < node->attributeCount; ++i) {
        ASTAttribute* attr = node->attributes[i];
        if (!attr || !attr->payload) continue;
        char* tmp = strdup(attr->payload);
        if (!tmp) continue;
        lower_inplace(tmp);
        bool isGnu = attr->syntax == AST_ATTRIBUTE_SYNTAX_GNU;
        bool isDeclspec = attr->syntax == AST_ATTRIBUTE_SYNTAX_DECLSPEC;

        if (isGnu && attr_payload_has_word(tmp, "weak")) {
            sym->isWeak = true;
        }

        if ((isGnu && (strstr(tmp, "stdcall") || strstr(tmp, "__stdcall"))) ||
            (isDeclspec && strstr(tmp, "stdcall"))) {
            bool supported = tl && tl->supportsStdcall;
            if (!supported) {
                if (errorIgnored) {
                    addError(node->line, 0, "__stdcall ignored on this target", sym->name);
                } else if (warnIgnored) {
                    addWarning(node->line, 0, "__stdcall ignored on this target", sym->name);
                }
                free(tmp);
                continue;
            }
            if (sym->signature.callConv != CALLCONV_DEFAULT &&
                sym->signature.callConv != CALLCONV_STDCALL) {
                addError(node->line, 0, "Conflicting calling convention on redeclaration", sym->name);
            }
            sym->signature.callConv = CALLCONV_STDCALL;
        } else if ((isGnu && strstr(tmp, "fastcall")) ||
                   (isDeclspec && strstr(tmp, "fastcall"))) {
            bool supported = tl && tl->supportsFastcall;
            if (!supported) {
                if (errorIgnored) {
                    addError(node->line, 0, "__fastcall ignored on this target", sym->name);
                } else if (warnIgnored) {
                    addWarning(node->line, 0, "__fastcall ignored on this target", sym->name);
                }
                free(tmp);
                continue;
            }
            if (sym->signature.callConv != CALLCONV_DEFAULT &&
                sym->signature.callConv != CALLCONV_FASTCALL) {
                addError(node->line, 0, "Conflicting calling convention on redeclaration", sym->name);
            }
            sym->signature.callConv = CALLCONV_FASTCALL;
        } else if ((isGnu && strstr(tmp, "cdecl")) || (isDeclspec && strstr(tmp, "cdecl"))) {
            sym->signature.callConv = CALLCONV_CDECL;
        }

        if (isDeclspec && strstr(tmp, "dllexport")) {
            bool supported = tl && tl->supportsDllStorage;
            if (!supported) {
                if (errorIgnored) {
                    addError(node->line, 0, "__declspec(dllexport) ignored on this target", sym->name);
                } else if (warnIgnored) {
                    addWarning(node->line, 0, "__declspec(dllexport) ignored on this target", sym->name);
                }
                free(tmp);
                continue;
            }
            if (sym->dllStorage != DLL_STORAGE_NONE && sym->dllStorage != DLL_STORAGE_EXPORT) {
                addError(node->line, 0, "Conflicting dllimport/dllexport on redeclaration", sym->name);
            }
            sym->dllStorage = DLL_STORAGE_EXPORT;
        } else if (isDeclspec && strstr(tmp, "dllimport")) {
            bool supported = tl && tl->supportsDllStorage;
            if (!supported) {
                if (errorIgnored) {
                    addError(node->line, 0, "__declspec(dllimport) ignored on this target", sym->name);
                } else if (warnIgnored) {
                    addWarning(node->line, 0, "__declspec(dllimport) ignored on this target", sym->name);
                }
                free(tmp);
                continue;
            }
            if (sym->dllStorage != DLL_STORAGE_NONE && sym->dllStorage != DLL_STORAGE_IMPORT) {
                addError(node->line, 0, "Conflicting dllimport/dllexport on redeclaration", sym->name);
            }
            sym->dllStorage = DLL_STORAGE_IMPORT;
        }
        free(tmp);
    }
}

StorageClass deduceStorageClass(const ParsedType* type) {
    if (!type) return STORAGE_NONE;
    if (type->isExtern) return STORAGE_EXTERN;
    if (type->isStatic) return STORAGE_STATIC;
    if (type->isRegister) return STORAGE_REGISTER;
    if (type->isAuto) return STORAGE_AUTO;
    return STORAGE_NONE;
}

SymbolLinkage deduceLinkage(const ParsedType* type, bool fileScope) {
    StorageClass sc = deduceStorageClass(type);
    if (sc == STORAGE_STATIC) return LINKAGE_INTERNAL;
    if (sc == STORAGE_EXTERN || fileScope) return LINKAGE_EXTERNAL;
    return LINKAGE_NONE;
}

static int countStorageSpecifiers(const ParsedType* type) {
    if (!type) return 0;
    int count = 0;
    if (type->isExtern) count++;
    if (type->isStatic) count++;
    if (type->isRegister) count++;
    if (type->isAuto) count++;
    return count;
}

bool validateStorageUsage(const ParsedType* type,
                          bool fileScope,
                          bool isFunction,
                          bool isTypedef,
                          int line,
                          const char* nameHint) {
    if (!type) return true;

    int storageCount = countStorageSpecifiers(type);
    if (storageCount > 1) {
        addError(line, 0, "Conflicting storage class specifiers", nameHint);
        return false;
    }

    if (isTypedef) {
        if (storageCount > 0) {
            addError(line,
                     0,
                     "Typedef cannot combine with other storage class specifiers",
                     nameHint);
            return false;
        }
        return true;
    }

    StorageClass storage = deduceStorageClass(type);
    if (!isFunction && fileScope && (storage == STORAGE_AUTO || storage == STORAGE_REGISTER)) {
        addError(line, 0, "Invalid storage class at file scope", nameHint);
        return false;
    }

    if (isFunction && (storage == STORAGE_AUTO || storage == STORAGE_REGISTER)) {
        addError(line, 0, "Invalid storage class for function declaration", nameHint);
        return false;
    }
    if (isFunction && !fileScope && storage == STORAGE_STATIC) {
        addError(line, 0, "Invalid storage class for function declaration", nameHint);
        return false;
    }

    return true;
}

bool validateRestrictUsage(const ParsedType* type,
                           Scope* scope,
                           int line,
                           const char* nameHint) {
    if (!type || !type->isRestrict) return true;
    TypeInfo info = typeInfoFromParsedType(type, scope);
    if (info.pointerDepth > 0) return true;
    addError(line, 0, "restrict qualifier requires a pointer type", nameHint);
    return false;
}

bool validatePrimitiveSpecifierUsage(const ParsedType* type,
                                     int line,
                                     const char* nameHint) {
    if (!type || type->kind != TYPE_PRIMITIVE) return true;

    if (type->isSigned && type->isUnsigned) {
        addError(line, 0, "Type cannot be both signed and unsigned", nameHint);
        return false;
    }
    if (type->isShort && type->isLong) {
        addError(line, 0, "Type cannot be both short and long", nameHint);
        return false;
    }

    if (type->isComplex || type->isImaginary) {
        if (type->primitiveType != TOKEN_FLOAT && type->primitiveType != TOKEN_DOUBLE) {
            addError(line, 0, "Invalid type specifier combination for _Complex", nameHint);
            return false;
        }
    }

    switch (type->primitiveType) {
        case TOKEN_FLOAT:
            if (type->isSigned || type->isUnsigned || type->isShort || type->isLong) {
                addError(line, 0, "Invalid type specifier combination for float", nameHint);
                return false;
            }
            break;
        case TOKEN_DOUBLE:
            if (type->isSigned || type->isUnsigned || type->isShort) {
                addError(line, 0, "Invalid type specifier combination for double", nameHint);
                return false;
            }
            break;
        case TOKEN_BOOL:
            if (type->isSigned || type->isUnsigned || type->isShort || type->isLong) {
                addError(line, 0, "Invalid type specifier combination for _Bool", nameHint);
                return false;
            }
            break;
        case TOKEN_CHAR:
            if (type->isShort || type->isLong) {
                addError(line, 0, "Invalid type specifier combination for char", nameHint);
                return false;
            }
            break;
        case TOKEN_VOID:
            if (type->isSigned || type->isUnsigned || type->isShort || type->isLong) {
                addError(line, 0, "Invalid type specifier combination for void", nameHint);
                return false;
            }
            break;
        default:
            break;
    }

    return true;
}

void analyzeDesignatedInitializer(DesignatedInit* init, Scope* scope) {
    if (!init || !scope) return;
    if (init->indexExpr) {
        (void)analyzeExpression(init->indexExpr, scope);
    }
    if (init->expression) {
        (void)analyzeExpression(init->expression, scope);
    }
}
