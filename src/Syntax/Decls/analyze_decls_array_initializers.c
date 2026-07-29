// SPDX-License-Identifier: Apache-2.0

#include "analyze_decls_internal.h"
#include "Utils/profiler.h"

static bool typeInfoIsStructLike(const TypeInfo* info) {
    return info && (info->category == TYPEINFO_STRUCT || info->category == TYPEINFO_UNION);
}

static bool typeInfoIsCharLike(const TypeInfo* info) {
    return info && info->category == TYPEINFO_INTEGER && info->bitWidth == 8;
}

static bool typeInfoIsWideCharLike(const TypeInfo* info) {
    return info && info->category == TYPEINFO_INTEGER && info->bitWidth > 8;
}

static const TypeDerivation* findFunctionDerivationInType(const ParsedType* type) {
    if (!type) return NULL;
    for (size_t i = 0; i < type->derivationCount; ++i) {
        const TypeDerivation* deriv = parsedTypeGetDerivation(type, i);
        if (deriv && deriv->kind == TYPE_DERIVATION_FUNCTION) {
            return deriv;
        }
    }
    return NULL;
}

static bool parsedTypeIsPlainVoidType(const ParsedType* type) {
    if (!type) return false;
    if (type->kind != TYPE_PRIMITIVE || type->primitiveType != TOKEN_VOID) return false;
    if (type->pointerDepth != 0) return false;
    if (type->derivationCount != 0) return false;
    return true;
}

static bool functionDerivationHasPrototype(const TypeDerivation* fn) {
    return fn && fn->kind == TYPE_DERIVATION_FUNCTION &&
           fn->as.function.hasPrototype;
}

static bool functionPrototypeParamsSurviveDefaultPromotions(
    const TypeDerivation* fn,
    Scope* scope) {
    if (!fn || fn->kind != TYPE_DERIVATION_FUNCTION ||
        fn->as.function.isVariadic) {
        return false;
    }
    if (fn->as.function.paramCount > 0 && !fn->as.function.params) {
        return false;
    }
    for (size_t i = 0; i < fn->as.function.paramCount; ++i) {
        TypeInfo declared =
            typeInfoFromParsedType(&fn->as.function.params[i], scope);
        TypeInfo promoted = defaultArgumentPromotion(declared);
        if (!typesAreEqual(&declared, &promoted)) {
            return false;
        }
    }
    return true;
}

static void normalizeFunctionDerivationParams(ParsedType* type) {
    if (!type) return;
    for (size_t i = 0; i < type->derivationCount; ++i) {
        TypeDerivation* deriv = &type->derivations[i];
        if (deriv->kind != TYPE_DERIVATION_FUNCTION) {
            continue;
        }
        if (deriv->as.function.isVariadic || deriv->as.function.paramCount != 1) {
            continue;
        }
        if (!parsedTypeIsPlainVoidType(&deriv->as.function.params[0])) {
            continue;
        }
        parsedTypeFree(&deriv->as.function.params[0]);
        free(deriv->as.function.params);
        deriv->as.function.params = NULL;
        deriv->as.function.paramCount = 0;
    }
}

static void normalizeArrayDerivationCompat(ParsedType* type) {
    if (!type) return;
    for (size_t i = 0; i < type->derivationCount; ++i) {
        TypeDerivation* deriv = &type->derivations[i];
        if (deriv->kind != TYPE_DERIVATION_ARRAY) {
            continue;
        }
        deriv->as.array.isVLA = false;
    }
}

static bool parsedTypesEqualForFunctionCompat(const ParsedType* lhs, const ParsedType* rhs) {
    if (!lhs || !rhs) return false;
    ParsedType lhsCopy = parsedTypeClone(lhs);
    ParsedType rhsCopy = parsedTypeClone(rhs);
    normalizeFunctionDerivationParams(&lhsCopy);
    normalizeFunctionDerivationParams(&rhsCopy);
    normalizeArrayDerivationCompat(&lhsCopy);
    normalizeArrayDerivationCompat(&rhsCopy);
    bool equal = parsedTypesStructurallyEqual(&lhsCopy, &rhsCopy);
    parsedTypeFree(&lhsCopy);
    parsedTypeFree(&rhsCopy);
    return equal;
}

static bool parsedTypeIsFunctionPointerLike(const ParsedType* type) {
    if (!type) return false;
    if (type->isFunctionPointer) return true;
    if (findFunctionDerivationInType(type) != NULL) return true;
    ParsedType target = parsedTypePointerTargetType(type);
    bool hasFunction = false;
    if (target.kind != TYPE_INVALID) {
        hasFunction = findFunctionDerivationInType(&target) != NULL;
    }
    parsedTypeFree(&target);
    return hasFunction;
}

static bool functionPointerTypesCompatibleForInitializer(const ParsedType* destType,
                                                         const ParsedType* srcType,
                                                         Scope* scope) {
    if (!destType || !srcType) return false;

    ParsedType destTarget = parsedTypePointerTargetType(destType);
    if (destTarget.kind == TYPE_INVALID) {
        parsedTypeFree(&destTarget);
        destTarget = parsedTypeClone(destType);
    }
    ParsedType srcTarget = parsedTypePointerTargetType(srcType);
    if (srcTarget.kind == TYPE_INVALID) {
        parsedTypeFree(&srcTarget);
        srcTarget = parsedTypeClone(srcType);
    }

    const TypeDerivation* destFn = findFunctionDerivationInType(&destTarget);
    const TypeDerivation* srcFn = findFunctionDerivationInType(&srcTarget);
    if (!destFn || !srcFn) {
        parsedTypeFree(&destTarget);
        parsedTypeFree(&srcTarget);
        return false;
    }

    ParsedType destRet = parsedTypeFunctionReturnType(&destTarget);
    ParsedType srcRet = parsedTypeFunctionReturnType(&srcTarget);
    bool retCompat = parsedTypesEqualForFunctionCompat(&destRet, &srcRet);
    if (!retCompat && scope) {
        TypeInfo destRetInfo = typeInfoFromParsedType(&destRet, scope);
        TypeInfo srcRetInfo = typeInfoFromParsedType(&srcRet, scope);
        retCompat = typesAreEqual(&destRetInfo, &srcRetInfo);
    }
    parsedTypeFree(&destRet);
    parsedTypeFree(&srcRet);
    if (!retCompat) {
        parsedTypeFree(&destTarget);
        parsedTypeFree(&srcTarget);
        return false;
    }

    bool destHasPrototype = functionDerivationHasPrototype(destFn);
    bool srcHasPrototype = functionDerivationHasPrototype(srcFn);
    if (destHasPrototype != srcHasPrototype) {
        const TypeDerivation* prototype = destHasPrototype ? destFn : srcFn;
        bool compatible =
            functionPrototypeParamsSurviveDefaultPromotions(prototype, scope);
        parsedTypeFree(&destTarget);
        parsedTypeFree(&srcTarget);
        return compatible;
    }
    if (destHasPrototype) {
        if (destFn->as.function.paramCount != srcFn->as.function.paramCount ||
            destFn->as.function.isVariadic != srcFn->as.function.isVariadic) {
            parsedTypeFree(&destTarget);
            parsedTypeFree(&srcTarget);
            return false;
        }
        for (size_t i = 0; i < destFn->as.function.paramCount; ++i) {
            bool paramCompat = parsedTypesEqualForFunctionCompat(&destFn->as.function.params[i],
                                                                 &srcFn->as.function.params[i]);
            if (!paramCompat && scope) {
                TypeInfo destParamInfo =
                    typeInfoFromParsedType(&destFn->as.function.params[i], scope);
                TypeInfo srcParamInfo =
                    typeInfoFromParsedType(&srcFn->as.function.params[i], scope);
                paramCompat = typesAreEqual(&destParamInfo, &srcParamInfo);
            }
            if (!paramCompat) {
                parsedTypeFree(&destTarget);
                parsedTypeFree(&srcTarget);
                return false;
            }
        }
    }

    profiler_record_value("semantic_count_type_info_decl_ptr_compat_dest", 1);
    TypeInfo destInfo = typeInfoFromParsedType(destType, NULL);
    profiler_record_value("semantic_count_type_info_decl_ptr_compat_src", 1);
    TypeInfo srcInfo = typeInfoFromParsedType(srcType, NULL);
    int depth = destInfo.pointerDepth < srcInfo.pointerDepth ? destInfo.pointerDepth
                                                             : srcInfo.pointerDepth;
    if (depth > TYPEINFO_MAX_POINTER_DEPTH) {
        depth = TYPEINFO_MAX_POINTER_DEPTH;
    }
    for (int i = 0; i < depth; ++i) {
        if (destInfo.pointerLevels[i].isConst != srcInfo.pointerLevels[i].isConst ||
            destInfo.pointerLevels[i].isVolatile != srcInfo.pointerLevels[i].isVolatile ||
            destInfo.pointerLevels[i].isRestrict != srcInfo.pointerLevels[i].isRestrict) {
            parsedTypeFree(&destTarget);
            parsedTypeFree(&srcTarget);
            return false;
        }
    }

    parsedTypeFree(&destTarget);
    parsedTypeFree(&srcTarget);
    return true;
}

static Symbol* initializerFunctionSymbol(ASTNode* expr, Scope* scope) {
    if (!expr || !scope) return NULL;
    switch (expr->type) {
        case AST_IDENTIFIER: {
            if (!expr->valueNode.value) return NULL;
            Symbol* sym = resolveInScopeChain(scope, expr->valueNode.value);
            return (sym && sym->kind == SYMBOL_FUNCTION) ? sym : NULL;
        }
        case AST_UNARY_EXPRESSION:
            if (expr->expr.op && strcmp(expr->expr.op, "&") == 0) {
                return initializerFunctionSymbol(expr->expr.left, scope);
            }
            return NULL;
        case AST_CAST_EXPRESSION:
            return initializerFunctionSymbol(expr->castExpr.expression, scope);
        default:
            return NULL;
    }
}

static bool functionDesignatorInitializerCompatible(const ParsedType* destType,
                                                    ASTNode* expr,
                                                    Scope* scope) {
    if (!destType || !expr || !scope) return false;
    Symbol* sym = initializerFunctionSymbol(expr, scope);
    if (!sym || sym->kind != SYMBOL_FUNCTION) return false;

    ParsedType destTarget = parsedTypePointerTargetType(destType);
    if (destTarget.kind == TYPE_INVALID) {
        parsedTypeFree(&destTarget);
        destTarget = parsedTypeClone(destType);
    }
    const TypeDerivation* destFn = findFunctionDerivationInType(&destTarget);
    if (!destFn) {
        parsedTypeFree(&destTarget);
        return false;
    }

    ParsedType destRet = parsedTypeFunctionReturnType(&destTarget);
    bool retCompat = parsedTypesStructurallyCompatibleInScope(&destRet, &sym->type, scope);
    parsedTypeFree(&destRet);
    if (!retCompat) {
        parsedTypeFree(&destTarget);
        return false;
    }

    bool destHasPrototype = functionDerivationHasPrototype(destFn);
    bool srcHasPrototype = sym->signature.hasPrototype;
    if (destHasPrototype != srcHasPrototype) {
        bool compatible = false;
        if (destHasPrototype) {
            compatible = functionPrototypeParamsSurviveDefaultPromotions(destFn, scope);
        } else if (!sym->signature.isVariadic) {
            compatible = true;
            for (size_t i = 0; i < sym->signature.paramCount; ++i) {
                TypeInfo declared =
                    typeInfoFromParsedType(&sym->signature.params[i], scope);
                TypeInfo promoted = defaultArgumentPromotion(declared);
                if (!typesAreEqual(&declared, &promoted)) {
                    compatible = false;
                    break;
                }
            }
        }
        parsedTypeFree(&destTarget);
        return compatible;
    }
    if (destHasPrototype) {
        if (destFn->as.function.paramCount != sym->signature.paramCount ||
            destFn->as.function.isVariadic != sym->signature.isVariadic) {
                parsedTypeFree(&destTarget);
                return false;
        }
        for (size_t i = 0; i < destFn->as.function.paramCount; ++i) {
            if (!parsedTypesStructurallyCompatibleInScope(&destFn->as.function.params[i],
                                                          &sym->signature.params[i],
                                                          scope)) {
                parsedTypeFree(&destTarget);
                return false;
            }
        }
    }

    parsedTypeFree(&destTarget);
    return true;
}

static bool sourcePathIsVirtualRemap(const char* path) {
    if (!path || !path[0]) return false;
    const char* base = strrchr(path, '/');
    base = base ? base + 1 : path;
    return strncmp(base, "virtual_", 8) == 0;
}

static void reportErrorAtLineOnlyAstNode(ASTNode* primary,
                                         ASTNode* fallback,
                                         int fallbackLine,
                                         const char* message,
                                         const char* hint) {
    ASTNode* node = primary ? primary : fallback;
    if (node && sourcePathIsVirtualRemap(node->location.start.file)) {
        SourceRange loc = node->location;
        if (loc.start.line <= 0) {
            loc.start.line = fallbackLine > 0 ? fallbackLine : node->line;
        }
        loc.start.column = 0;
        loc.end = loc.start;
        addErrorWithRanges(loc, node->macroCallSite, node->macroDefinition, message, hint);
        return;
    }
    addError(fallbackLine, 0, message, hint);
}

static bool isStringLiteralInitializer(DesignatedInit* init) {
    return init && init->expression && init->expression->type == AST_STRING_LITERAL;
}

static bool arrayInnerBlockSize(const ParsedType* type, size_t* outSize) {
    if (!type || !outSize) return false;
    ParsedType elem = parsedTypeArrayElementType(type);
    if (elem.kind == TYPE_INVALID) return false;
    size_t dims = parsedTypeCountDerivationsOfKind(&elem, TYPE_DERIVATION_ARRAY);
    size_t block = 1;
    for (size_t i = 0; i < dims; ++i) {
        const TypeDerivation* arr = parsedTypeGetArrayDerivation(&elem, i);
        if (!arr ||
            arr->as.array.isVLA ||
            !arr->as.array.hasConstantSize ||
            arr->as.array.constantSize < 0) {
            parsedTypeFree(&elem);
            return false;
        }
        block *= (size_t)arr->as.array.constantSize;
    }
    parsedTypeFree(&elem);
    *outSize = block;
    return true;
}

void validateArrayInitializerEntries(ParsedType* type,
                                     const char* arrayName,
                                     DesignatedInit** values,
                                     size_t valueCount,
                                     Scope* scope,
                                     ASTNode* contextNode,
                                     long long* outInferredLength) {
    if (!type || !scope) return;

    TypeDerivation* topArray = parsedTypeGetMutableArrayDerivation(type, 0);
    if (!topArray) return;
    bool hasDeclaredLen = topArray->as.array.hasConstantSize && !topArray->as.array.isVLA;
    size_t declaredLen = hasDeclaredLen ? (size_t)topArray->as.array.constantSize : 0;

    ParsedType elementParsed = parsedTypeArrayElementType(type);
    if (elementParsed.kind == TYPE_INVALID) {
        return;
    }
    parsedTypeResolvePlainNamedTypedefInScope(&elementParsed, scope);
    profiler_record_value("semantic_count_type_info_site_decl", 1);
    TypeInfo elementInfo = typeInfoFromParsedType(&elementParsed, scope);
    bool elementIsArray = parsedTypeIsDirectArray(&elementParsed);
    ASTNode* lastValueExpr = contextNode;
    for (size_t i = 0; i < valueCount; ++i) {
        if (values && values[i] && values[i]->expression) {
            lastValueExpr = values[i]->expression;
        }
    }

    if (valueCount == 1 && isStringLiteralInitializer(values[0])) {
        ASTNode* stringExpr = values[0] ? values[0]->expression : contextNode;
        const char* payload = NULL;
        LiteralEncoding enc = ast_literal_encoding(values[0]->expression->valueNode.value, &payload);
        bool narrowCompat = (enc == LIT_ENC_NARROW || enc == LIT_ENC_UTF8) && typeInfoIsCharLike(&elementInfo);
        bool wideCompat = enc == LIT_ENC_WIDE && typeInfoIsWideCharLike(&elementInfo);
        if (narrowCompat || wideCompat) {
            int charBitWidth = elementInfo.bitWidth ? (int)elementInfo.bitWidth : 8;
            LiteralDecodeResult res = wideCompat
                ? decode_c_string_literal_wide(payload ? payload : "", charBitWidth, NULL, NULL)
                : decode_c_string_literal(payload ? payload : "", charBitWidth, NULL, NULL);
            size_t needed = res.length + 1;
            if (!res.ok) {
                char buffer[256];
                snprintf(buffer,
                         sizeof(buffer),
                         "Invalid escape sequence in string literal for array '%s'",
                         arrayName);
                reportErrorAtLineOnlyAstNode(contextNode,
                                             stringExpr,
                                             contextNode ? contextNode->line : 0,
                                             buffer,
                                             NULL);
                parsedTypeFree(&elementParsed);
                return;
            } else if (res.overflow) {
                char buffer[256];
                snprintf(buffer,
                         sizeof(buffer),
                         "String literal for array '%s' contains code points not representable in element type",
                         arrayName);
                reportErrorAtLineOnlyAstNode(contextNode,
                                             stringExpr,
                                             contextNode ? contextNode->line : 0,
                                             buffer,
                                             NULL);
                parsedTypeFree(&elementParsed);
                return;
            } else if (hasDeclaredLen && needed > declaredLen) {
                char buffer[256];
                snprintf(buffer,
                         sizeof(buffer),
                         "String literal for array '%s' is too long (needs %zu, size %zu)",
                         arrayName,
                         needed,
                         declaredLen);
                reportErrorAtLineOnlyAstNode(contextNode,
                                             stringExpr,
                                             contextNode ? contextNode->line : 0,
                                             buffer,
                                             NULL);
                parsedTypeFree(&elementParsed);
                return;
            } else if (!hasDeclaredLen && outInferredLength) {
                *outInferredLength = (long long)needed;
                parsedTypeFree(&elementParsed);
                return;
            } else {
                parsedTypeFree(&elementParsed);
                return;
            }
        }
    }

    bool hasDesignators = false;
    bool hasCompound = false;
    size_t scalarCount = 0;
    for (size_t i = 0; i < valueCount; ++i) {
        DesignatedInit* init = values[i];
        if (!init || !init->expression) continue;
        if (init->indexExpr || init->fieldName) {
            hasDesignators = true;
        }
        if (init->expression->type == AST_COMPOUND_LITERAL) {
            hasCompound = true;
        }
        scalarCount++;
    }

    if (elementIsArray && !hasDesignators && !hasCompound) {
        size_t block = 0;
        if (arrayInnerBlockSize(type, &block) && block > 0) {
            if (hasDeclaredLen) {
                size_t capacity = declaredLen * block;
                if (scalarCount > capacity) {
                    char buffer[256];
                    snprintf(buffer,
                             sizeof(buffer),
                             "Too many initializers for array '%s' (size %zu)",
                             arrayName,
                             capacity);
                    reportErrorAtLineOnlyAstNode(contextNode,
                                                 lastValueExpr,
                                                 contextNode ? contextNode->line : 0,
                                                 buffer,
                                                 NULL);
                }
                parsedTypeFree(&elementParsed);
                return;
            }
            if (outInferredLength && scalarCount > 0) {
                size_t inferred = (scalarCount + block - 1) / block;
                *outInferredLength = (long long)inferred;
                parsedTypeFree(&elementParsed);
                return;
            }
        }
    }

    size_t sequentialCount = 0;
    size_t highestIndex = 0;
    bool sawAny = false;
    for (size_t i = 0; i < valueCount; ++i) {
        DesignatedInit* init = values[i];
        if (!init) continue;
        if (init->indexExpr) {
            long long indexValue = 0;
            if (!constEvalInteger(init->indexExpr, scope, &indexValue, true)) {
                char buffer[256];
                snprintf(buffer,
                         sizeof(buffer),
                         "Array '%s' designator index must be a constant expression",
                         arrayName);
                reportErrorAtAstNode(init->indexExpr, init->indexExpr->line, buffer, NULL);
            } else if (indexValue < 0) {
                char buffer[256];
                snprintf(buffer,
                         sizeof(buffer),
                         "Array '%s' designator index %lld is negative",
                         arrayName,
                         indexValue);
                reportErrorAtAstNode(init->indexExpr, init->indexExpr->line, buffer, NULL);
            } else if (hasDeclaredLen && (size_t)indexValue >= declaredLen) {
                char buffer[256];
                snprintf(buffer,
                         sizeof(buffer),
                         "Array '%s' designator index %lld is out of bounds (size %zu)",
                         arrayName,
                         indexValue,
                         declaredLen);
                reportErrorAtAstNode(init->indexExpr, init->indexExpr->line, buffer, NULL);
            }
            if (indexValue >= 0) {
                size_t candidate = (size_t)indexValue + 1;
                sequentialCount = candidate;
                if (candidate > highestIndex) {
                    highestIndex = candidate;
                }
                sawAny = true;
            }
        } else {
            if (hasDeclaredLen && sequentialCount >= declaredLen) {
                char buffer[256];
                snprintf(buffer,
                         sizeof(buffer),
                         "Too many initializers for array '%s' (size %zu)",
                         arrayName,
                         declaredLen);
                reportErrorAtLineOnlyAstNode(contextNode,
                                             init->expression,
                                             contextNode ? contextNode->line : 0,
                                             buffer,
                                             NULL);
            }
            sequentialCount++;
            if (sequentialCount > highestIndex) {
                highestIndex = sequentialCount;
            }
            sawAny = true;
        }

        if (init->expression && init->expression->type == AST_COMPOUND_LITERAL) {
            if (elementIsArray) {
                validateArrayInitializerEntries(&elementParsed,
                                                arrayName,
                                                init->expression->compoundLiteral.entries,
                                                init->expression->compoundLiteral.entryCount,
                                                scope,
                                                contextNode,
                                                NULL);
            } else if (!typeInfoIsStructLike(&elementInfo)) {
                validateScalarCompoundLiteral(init->expression, contextNode, arrayName);
            }
        } else if (!elementIsArray && init->expression) {
            TypeInfo rhs = analyzeExpression(init->expression, scope);
            rhs = decayToRValue(rhs);
            if (rhs.category != TYPEINFO_INVALID) {
                AssignmentCheckResult assign = canAssignTypesInScope(&elementInfo, &rhs, scope);
                if (assign == ASSIGN_INCOMPATIBLE &&
                    elementInfo.originalType &&
                    parsedTypeIsFunctionPointerLike(elementInfo.originalType) &&
                    functionDesignatorInitializerCompatible(elementInfo.originalType,
                                                           init->expression,
                                                           scope)) {
                    assign = ASSIGN_OK;
                }
                if (assign == ASSIGN_INCOMPATIBLE &&
                    typeInfoIsPointerLike(&elementInfo) &&
                    typeInfoIsInteger(&rhs)) {
                    long long zero = 1;
                    if (constEvalInteger(init->expression, scope, &zero, true) && zero == 0) {
                        assign = ASSIGN_OK;
                    }
                }
                if (assign == ASSIGN_INCOMPATIBLE &&
                    typeInfoIsStructLike(&elementInfo) &&
                    typeInfoIsInteger(&rhs)) {
                    long long zero = 1;
                    if (constEvalInteger(init->expression, scope, &zero, true) && zero == 0) {
                        assign = ASSIGN_OK;
                    }
                }
                if (assign == ASSIGN_OK &&
                    typeInfoIsPointerLike(&elementInfo) &&
                    typeInfoIsPointerLike(&rhs) &&
                    elementInfo.originalType &&
                    rhs.originalType &&
                    parsedTypeIsFunctionPointerLike(elementInfo.originalType) &&
                    parsedTypeIsFunctionPointerLike(rhs.originalType) &&
                    !functionPointerTypesCompatibleForInitializer(elementInfo.originalType,
                                                                  rhs.originalType,
                                                                  scope)) {
                    assign = ASSIGN_INCOMPATIBLE;
                }
                if (assign == ASSIGN_INCOMPATIBLE) {
                    char buffer[256];
                    snprintf(buffer,
                             sizeof(buffer),
                             "Incompatible initializer type for '%s'",
                             arrayName ? arrayName : "<unnamed>");
                    addError(init->expression->line, 0, buffer, NULL);
                } else if (assign == ASSIGN_QUALIFIER_LOSS) {
                    char buffer[256];
                    snprintf(buffer,
                             sizeof(buffer),
                             "Initializer for '%s' discards qualifiers from pointer target",
                             arrayName ? arrayName : "<unnamed>");
                    addError(init->expression->line, 0, buffer, NULL);
                }
            }
        }
    }

    parsedTypeFree(&elementParsed);

    if (!hasDeclaredLen && outInferredLength && sawAny) {
        *outInferredLength = (long long)highestIndex;
    }
}

void validateVariableArrayInitializer(ParsedType* type,
                                      DesignatedInit* init,
                                      ASTNode* nameNode,
                                      Scope* scope) {
    if (!type || !scope) return;
    const char* arrayName = safeIdentifierName(nameNode);
    if (!init || !init->expression) {
        return;
    }
    long long inferredLen = -1;
    if (init->expression->type == AST_COMPOUND_LITERAL) {
        validateArrayInitializerEntries(type,
                                        arrayName,
                                        init->expression->compoundLiteral.entries,
                                        init->expression->compoundLiteral.entryCount,
                                        scope,
                                        nameNode,
                                        &inferredLen);
    } else if (init->expression->type == AST_STRING_LITERAL) {
        DesignatedInit* single[1];
        single[0] = init;
        validateArrayInitializerEntries(type,
                                        arrayName,
                                        single,
                                        1,
                                        scope,
                                        nameNode,
                                        &inferredLen);
    } else {
        char buffer[256];
        snprintf(buffer,
                 sizeof(buffer),
                 "Initializer for array '%s' must be brace-enclosed",
                 arrayName);
        addError(nameNode ? nameNode->line : 0, 0, buffer, NULL);
        return;
    }

    if (inferredLen >= 0) {
        TypeDerivation* topArray = parsedTypeGetMutableArrayDerivation(type, 0);
        if (topArray) {
            topArray->as.array.hasConstantSize = true;
            topArray->as.array.constantSize = inferredLen;
            topArray->as.array.isVLA = false;
        }
    }
}
