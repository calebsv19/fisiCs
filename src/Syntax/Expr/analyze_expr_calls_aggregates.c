// SPDX-License-Identifier: Apache-2.0

#include "analyze_expr_internal.h"
#include "analyze_core.h"
#include "Extensions/extension_diagnostics.h"
#include "syntax_errors.h"
#include "symbol_table.h"
#include "const_eval.h"
#include "literal_utils.h"
#include "Parser/Helpers/designated_init.h"
#include "Parser/Helpers/parsed_type.h"
#include "Syntax/target_layout.h"
#include "Utils/profiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static TypeInfo sizeTypeFromScope(Scope* scope) {
    Symbol* sym = resolveInScopeChain(scope, "size_t");
    if (sym && sym->kind == SYMBOL_TYPEDEF) {
        profiler_record_value("semantic_count_type_info_site_symbol", 1);
        TypeInfo info = typeInfoFromSymbolCached(sym, scope);
        info.isLValue = false;
        return info;
    }
    const TargetLayout* tl = (scope && scope->ctx) ? cc_get_target_layout(scope->ctx) : tl_default();
    unsigned bits = tl && tl->pointerBits ? (unsigned)tl->pointerBits : 64;
    TypeInfo info = makeIntegerType(bits, true, TOKEN_LONG);
    info.isLValue = false;
    return info;
}
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
static bool pointerTargetsCompatibleForConditional(const TypeInfo* a, const TypeInfo* b) {
    if (!a || !b) return false;
    if (a->pointerDepth != b->pointerDepth) return false;
    if (typeInfoIsFunctionPointerLike(a) && typeInfoIsFunctionPointerLike(b)) {
        if (!a->originalType || !b->originalType) {
            return false;
        }
        ParsedType aTarget = parsedTypePointerTargetType(a->originalType);
        ParsedType bTarget = parsedTypePointerTargetType(b->originalType);
        if (aTarget.kind == TYPE_INVALID || bTarget.kind == TYPE_INVALID) {
            parsedTypeFree(&aTarget);
            parsedTypeFree(&bTarget);
            return false;
        }

        const TypeDerivation* aFn = NULL;
        const TypeDerivation* bFn = NULL;
        for (size_t i = 0; i < aTarget.derivationCount; ++i) {
            const TypeDerivation* d = parsedTypeGetDerivation(&aTarget, i);
            if (d && d->kind == TYPE_DERIVATION_FUNCTION) {
                aFn = d;
                break;
            }
        }
        for (size_t i = 0; i < bTarget.derivationCount; ++i) {
            const TypeDerivation* d = parsedTypeGetDerivation(&bTarget, i);
            if (d && d->kind == TYPE_DERIVATION_FUNCTION) {
                bFn = d;
                break;
            }
        }
        if (!aFn || !bFn) {
            parsedTypeFree(&aTarget);
            parsedTypeFree(&bTarget);
            return false;
        }

        ParsedType aRet = parsedTypeFunctionReturnType(&aTarget);
        ParsedType bRet = parsedTypeFunctionReturnType(&bTarget);
        bool retCompat = parsedTypesStructurallyEqual(&aRet, &bRet);
        parsedTypeFree(&aRet);
        parsedTypeFree(&bRet);
        if (!retCompat) {
            parsedTypeFree(&aTarget);
            parsedTypeFree(&bTarget);
            return false;
        }

        bool aHasParams = aFn->as.function.paramCount > 0 || aFn->as.function.isVariadic;
        bool bHasParams = bFn->as.function.paramCount > 0 || bFn->as.function.isVariadic;
        if (!aHasParams || !bHasParams) {
            parsedTypeFree(&aTarget);
            parsedTypeFree(&bTarget);
            return true;
        }

        if (aFn->as.function.isVariadic != bFn->as.function.isVariadic ||
            aFn->as.function.paramCount != bFn->as.function.paramCount) {
            parsedTypeFree(&aTarget);
            parsedTypeFree(&bTarget);
            return false;
        }

        for (size_t i = 0; i < aFn->as.function.paramCount; ++i) {
            if (!parsedTypesStructurallyEqual(&aFn->as.function.params[i],
                                              &bFn->as.function.params[i])) {
                parsedTypeFree(&aTarget);
                parsedTypeFree(&bTarget);
                return false;
            }
        }

        parsedTypeFree(&aTarget);
        parsedTypeFree(&bTarget);
        return true;
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

static bool functionSymbolsCompatibleForConditional(const Symbol* a, const Symbol* b) {
    if (!a || !b || a->kind != SYMBOL_FUNCTION || b->kind != SYMBOL_FUNCTION) {
        return false;
    }
    if (!parsedTypesStructurallyEqual(&a->type, &b->type)) {
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
        if (!parsedTypesStructurallyEqual(&a->signature.params[i], &b->signature.params[i])) {
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

static bool callableTargetFromTypeInfo(const TypeInfo* calleeInfo, Scope* scope, ParsedType* outTarget) {
    if (!calleeInfo || !calleeInfo->originalType || !outTarget) {
        return false;
    }

    memset(outTarget, 0, sizeof(*outTarget));
    outTarget->kind = TYPE_INVALID;

    ParsedType resolved = parsedTypeClone(calleeInfo->originalType);
    if (resolved.kind == TYPE_INVALID) {
        parsedTypeFree(&resolved);
        return false;
    }

    int depthGuard = 0;
    while (resolved.kind == TYPE_NAMED &&
           resolved.userTypeName &&
           resolved.derivationCount == 0 &&
           resolved.pointerDepth == 0 &&
           scope &&
           depthGuard < 32) {
        Symbol* sym = resolveInScopeChain(scope, resolved.userTypeName);
        if (!sym || sym->kind != SYMBOL_TYPEDEF) {
            break;
        }
        ParsedType next = parsedTypeClone(&sym->type);
        if (next.kind == TYPE_INVALID) {
            parsedTypeFree(&next);
            break;
        }
        parsedTypeFree(&resolved);
        resolved = next;
        depthGuard++;
    }

    if (calleeInfo->category == TYPEINFO_POINTER && calleeInfo->pointerDepth > 0) {
        ParsedType target = parsedTypePointerTargetType(&resolved);
        parsedTypeFree(&resolved);
        if (target.kind == TYPE_INVALID) {
            parsedTypeFree(&target);
            return false;
        }
        *outTarget = target;
        return true;
    }

    if (calleeInfo->category == TYPEINFO_FUNCTION || calleeInfo->isFunction) {
        *outTarget = resolved;
        return true;
    }

    parsedTypeFree(&resolved);
    return false;
}

static bool parsedTypeIsRestrictPointer(const ParsedType* pt) {
    if (!pt) return false;
    const TypeDerivation* d = parsedTypeGetDerivation(pt, 0);
    if (!d || d->kind != TYPE_DERIVATION_POINTER) return false;
    return d->as.pointer.isRestrict;
}

static void analyzeDesignatedInitializerExpr(DesignatedInit* init, Scope* scope) {
    if (!init) return;
    if (init->indexExpr) {
        analyzeExpression(init->indexExpr, scope);
    }
    if (init->expression) {
        analyzeExpression(init->expression, scope);
    }
}

static bool tryEvaluateArrayLengthExpr(ASTNode* sizeExpr, Scope* scope, size_t* outLen) {
    if (!sizeExpr || !outLen) return false;
    long long value = 0;
    bool ok = constEvalInteger(sizeExpr, scope, &value, true);
    if (!ok || value < 0) {
        return false;
    }
    *outLen = (size_t)value;
    return true;
}

static void inferCompoundLiteralArrayLength(ASTNode* node, Scope* scope) {
    if (!node || node->type != AST_COMPOUND_LITERAL) return;
    ParsedType* type = &node->compoundLiteral.literalType;
    if (!parsedTypeIsDirectArray(type)) return;

    TypeDerivation* topArray = parsedTypeGetMutableArrayDerivation(type, 0);
    if (!topArray) return;
    if (topArray->as.array.hasConstantSize || topArray->as.array.isVLA || topArray->as.array.isFlexible) {
        return;
    }

    if (topArray->as.array.sizeExpr) {
        size_t len = 0;
        if (tryEvaluateArrayLengthExpr(topArray->as.array.sizeExpr, scope, &len)) {
            topArray->as.array.hasConstantSize = true;
            topArray->as.array.constantSize = (long long)len;
            topArray->as.array.isVLA = false;
        } else {
            topArray->as.array.isVLA = true;
            type->isVLA = true;
        }
        return;
    }

    size_t sequentialCount = 0;
    size_t highestIndex = 0;
    bool sawAny = false;
    const char* arrayName = "<compound literal>";
    for (size_t i = 0; i < node->compoundLiteral.entryCount; ++i) {
        DesignatedInit* init = node->compoundLiteral.entries[i];
        if (!init) continue;
        if (init->indexExpr) {
            long long idx = 0;
            if (!constEvalInteger(init->indexExpr, scope, &idx, true)) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer),
                         "Array '%s' designator index must be a constant expression",
                         arrayName);
                addError(node->line, 0, buffer, NULL);
                continue;
            }
            if (idx < 0) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer),
                         "Array '%s' designator index %lld is negative",
                         arrayName,
                         idx);
                addError(node->line, 0, buffer, NULL);
                continue;
            }
            sequentialCount = (size_t)idx + 1;
            if (sequentialCount > highestIndex) {
                highestIndex = sequentialCount;
            }
            sawAny = true;
        } else {
            sequentialCount++;
            if (sequentialCount > highestIndex) {
                highestIndex = sequentialCount;
            }
            sawAny = true;
        }
    }

    if (sawAny) {
        topArray->as.array.hasConstantSize = true;
        topArray->as.array.constantSize = (long long)highestIndex;
        topArray->as.array.isVLA = false;
    }
}

static const char* fallbackFunctionName(const char* name) {
    return name ? name : "<anonymous>";
}

static void reportArgumentCountError(ASTNode* call,
                                     const char* calleeName,
                                     size_t expected,
                                     size_t actual,
                                     bool tooFew) {
    char buffer[160];
    snprintf(buffer,
             sizeof(buffer),
             "Too %s arguments in call to '%s' (expected %zu, got %zu)",
             tooFew ? "few" : "many",
             fallbackFunctionName(calleeName),
             expected,
             actual);
    SourceRange loc = call ? call->location : (SourceRange){0};
    SourceRange callSite = call ? call->macroCallSite : (SourceRange){0};
    SourceRange macroDef = call ? call->macroDefinition : (SourceRange){0};
    addErrorWithRanges(loc, callSite, macroDef, buffer, NULL);
}

static void reportArgumentTypeError(ASTNode* argNode,
                                    size_t index,
                                    const char* calleeName,
                                    const char* message) {
    char buffer[200];
    snprintf(buffer,
             sizeof(buffer),
             "Argument %zu of '%s' %s",
             index + 1,
             fallbackFunctionName(calleeName),
             message);
    SourceRange loc = argNode ? argNode->location : (SourceRange){0};
    SourceRange callSite = argNode ? argNode->macroCallSite : (SourceRange){0};
    SourceRange macroDef = argNode ? argNode->macroDefinition : (SourceRange){0};
    addErrorWithRanges(loc, callSite, macroDef, buffer, NULL);
}

static bool shouldDowngradeArgTypeMismatchToWarning(const Scope* scope,
                                                    const char* calleeName,
                                                    const TypeInfo* paramInfo,
                                                    const TypeInfo* argInfo) {
    (void)scope;
    if (!calleeName || strcmp(calleeName, "SDL_SetRenderDrawBlendMode") != 0) {
        return false;
    }
    if (!typeInfoIsPointerLike(paramInfo) || !typeInfoIsPointerLike(argInfo)) {
        return false;
    }
    return true;
}

static TypeInfo functionCallResultTypeFromSymbol(const Symbol* sym, Scope* scope) {
    if (!sym) {
        return makeInvalidType();
    }

    ParsedType retParsed = parsedTypeClone(&sym->type);
    if (retParsed.kind != TYPE_INVALID) {
        profiler_record_value("semantic_count_type_info_site_temp", 1);
        profiler_record_value("semantic_count_type_info_temp_fn_symbol_ret", 1);
        TypeInfo result = typeInfoFromParsedType(&retParsed, scope);
        typeInfoAdoptParsedType(&result, &retParsed);
        parsedTypeFree(&retParsed);
        result.isLValue = false;
        return result;
    }
    parsedTypeFree(&retParsed);
    profiler_record_value("semantic_count_type_info_site_symbol", 1);
    TypeInfo fallback = typeInfoFromSymbolCached((Symbol*)sym, scope);
    fallback.isLValue = false;
    return fallback;
}

static bool isFisicsUnitConvertHelper(const char* calleeName) {
    return calleeName &&
           (strcmp(calleeName, "fisics_convert_unit") == 0 ||
            strcmp(calleeName, "__builtin_fisics_convert_unit") == 0);
}

TypeInfo analyzeFunctionCallExpression(ASTNode* node, Scope* scope) {
    const char* calleeName = NULL;
    if (node->functionCall.callee &&
        node->functionCall.callee->type == AST_IDENTIFIER) {
        calleeName = node->functionCall.callee->valueNode.value;
        if (calleeName &&
            (strcmp(calleeName, "__builtin_offsetof") == 0 ||
             strcmp(calleeName, "offsetof") == 0)) {
            if (node->functionCall.argumentCount != 2) {
                addError(node->line, 0, "__builtin_offsetof expects 2 arguments", NULL);
                return sizeTypeFromScope(scope);
            }
            ASTNode* typeArg = node->functionCall.arguments[0];
            ASTNode* fieldArg = node->functionCall.arguments[1];
            if (!typeArg || typeArg->type != AST_PARSED_TYPE) {
                addError(node->line, 0, "__builtin_offsetof expects a type as the first argument", NULL);
                return sizeTypeFromScope(scope);
            }
            if (!fieldArg || fieldArg->type != AST_IDENTIFIER) {
                addError(node->line, 0, "__builtin_offsetof expects a field name as the second argument", NULL);
                return sizeTypeFromScope(scope);
            }
            profiler_record_value("semantic_count_type_info_site_temp", 1);
            profiler_record_value("semantic_count_type_info_temp_offsetof_base", 1);
            TypeInfo base = typeInfoFromParsedType(&typeArg->parsedTypeNode.parsed, scope);
            if (base.category != TYPEINFO_STRUCT && base.category != TYPEINFO_UNION) {
                addError(node->line, 0, "__builtin_offsetof expects a struct or union type", NULL);
                return sizeTypeFromScope(scope);
            }
            size_t ignoredOffset = 0;
            bool bitfieldPath = false;
            if (!evalOffsetofFieldPath(&typeArg->parsedTypeNode.parsed,
                                       fieldArg->valueNode.value,
                                       scope,
                                       &ignoredOffset,
                                       &bitfieldPath)) {
                if (bitfieldPath) {
                    addError(node->line, 0, "__builtin_offsetof cannot be used on bitfields", fieldArg->valueNode.value);
                } else {
                    addError(node->line, 0, "Unknown field in __builtin_offsetof", fieldArg->valueNode.value);
                }
                return sizeTypeFromScope(scope);
            }
            return sizeTypeFromScope(scope);
        }
    }

    TypeInfo calleeInfo = analyzeExpression(node->functionCall.callee, scope);

    size_t argCount = node->functionCall.argumentCount;
    TypeInfo* argInfos = NULL;
    TypeInfo* argRawInfos = NULL;
    if (argCount > 0) {
        argInfos = calloc(argCount, sizeof(TypeInfo));
        argRawInfos = calloc(argCount, sizeof(TypeInfo));
    }
    for (size_t i = 0; i < argCount; i++) {
        ASTNode* argNode = node->functionCall.arguments ? node->functionCall.arguments[i] : NULL;
        TypeInfo argType = analyzeExpression(argNode, scope);
        if (argRawInfos) {
            argRawInfos[i] = argType;
        }
        if (argInfos) {
            argInfos[i] = decayToRValue(argType);
        }
    }

    Symbol* sym = NULL;
    if (node->functionCall.callee &&
        node->functionCall.callee->type == AST_IDENTIFIER) {
        calleeName = node->functionCall.callee->valueNode.value;
        sym = resolveInScopeChain(scope, calleeName);
    }

    TypeInfo result = makeInvalidType();
    if (calleeName &&
        (strcmp(calleeName, "__builtin_va_arg") == 0 ||
         strcmp(calleeName, "va_arg") == 0)) {
        if (argCount == 2 &&
            node->functionCall.arguments[1] &&
            node->functionCall.arguments[1]->type == AST_PARSED_TYPE) {
            profiler_record_value("semantic_count_type_info_site_temp", 1);
            profiler_record_value("semantic_count_type_info_temp_va_arg", 1);
            result = typeInfoFromParsedType(&node->functionCall.arguments[1]->parsedTypeNode.parsed, scope);
            result.isLValue = false;
            free(argInfos);
            free(argRawInfos);
            return result;
        }
    }
    if (isFisicsUnitConvertHelper(calleeName)) {
        if (argCount == 2 && argInfos) {
            if (!typeInfoIsFloating(&argInfos[0]) && scope && scope->ctx) {
                fisics_extension_diag_units_conversion_requires_floating(scope->ctx, node, calleeName);
            }
            result = argInfos[0];
            result.isLValue = false;
            free(argInfos);
            free(argRawInfos);
            return result;
        }
    }
    if (calleeName && strcmp(calleeName, "va_start") == 0) {
        if (!scope || !scope->inFunction) {
            addError(node->line, 0, "va_start can only appear inside a function", NULL);
        } else {
            if (argCount < 2) {
                addError(node->line, 0, "va_start expects at least two arguments", NULL);
            }
            if (!scope->currentFunctionIsVariadic) {
                addError(node->line, 0, "va_start used in non-variadic function", NULL);
            }
            if (scope->currentFunctionFixedParams == 0) {
                addError(node->line, 0, "va_start requires at least one named parameter", NULL);
            }
        }
    }
    if (sym && sym->kind == SYMBOL_FUNCTION) {
        FunctionSignature* sig = &sym->signature;
        node->functionCall.usesPrototype = sig->hasPrototype;
        if (!sig->hasPrototype) {
            if (argInfos) {
                for (size_t i = 0; i < argCount; ++i) {
                    argInfos[i] = defaultArgumentPromotion(argInfos[i]);
                }
            }
            result = functionCallResultTypeFromSymbol(sym, scope);
            free(argInfos);
            free(argRawInfos);
            result.isLValue = false;
            return result;
        }
        size_t expected = sig->paramCount;
        bool tooFew = argCount < expected;
        bool tooMany = !sig->isVariadic && argCount > expected;
        if (tooFew) {
            reportArgumentCountError(node, calleeName, expected, argCount, true);
        }
        if (tooMany) {
            reportArgumentCountError(node, calleeName, expected, argCount, false);
        }
        size_t pairCount = expected < argCount ? expected : argCount;
        bool* paramRestrict = pairCount ? calloc(pairCount, sizeof(bool)) : NULL;
        char** argPaths = pairCount ? calloc(pairCount, sizeof(char*)) : NULL;
        for (size_t i = 0; i < pairCount; ++i) {
            profiler_record_value("semantic_count_type_info_site_signature", 1);
            TypeInfo paramInfo = typeInfoFromParsedType(&sig->params[i], scope);
            if (paramInfo.isArray) {
                paramInfo = decayToRValue(paramInfo);
            }
            if (sig->params[i].hasParamArrayInfo &&
                sig->params[i].paramArrayInfo.hasStatic &&
                argRawInfos) {
                long long required = 0;
                bool haveRequired = false;
                const ParsedArrayInfo* paramArr = &sig->params[i].paramArrayInfo;
                if (paramArr->sizeExpr) {
                    haveRequired = constEvalInteger(paramArr->sizeExpr, scope, &required, true);
                } else if (paramArr->hasConstantSize) {
                    required = paramArr->constantSize;
                    haveRequired = true;
                }
                if (haveRequired && required > 0 && i < argCount) {
                    const TypeInfo* argRaw = &argRawInfos[i];
                    if (argRaw->isArray && argRaw->originalType) {
                        long long argSize = 0;
                        bool haveArgSize = false;
                        const TypeDerivation* argArr =
                            parsedTypeGetArrayDerivation(argRaw->originalType, 0);
                        if (argArr) {
                            if (argArr->as.array.sizeExpr) {
                                haveArgSize = constEvalInteger(argArr->as.array.sizeExpr,
                                                               scope,
                                                               &argSize,
                                                               true);
                            } else if (argArr->as.array.hasConstantSize) {
                                argSize = argArr->as.array.constantSize;
                                haveArgSize = true;
                            }
                        }
                        if (haveArgSize && argSize < required) {
                            addWarning(node->functionCall.arguments[i]->line,
                                       0,
                                       "Array argument smaller than 'static' parameter size",
                                       NULL);
                        }
                    }
                }
            }
            if (paramRestrict) {
                paramRestrict[i] = parsedTypeIsRestrictPointer(&sig->params[i]) ||
                                   ((paramInfo.pointerDepth > 0) && paramInfo.pointerLevels[0].isRestrict);
            }
            if (argPaths && node->functionCall.arguments) {
                argPaths[i] = analyzeExprAccessPath(node->functionCall.arguments[i]);
            }
            TypeInfo argInfo = argInfos ? argInfos[i] : makeInvalidType();
            if (argInfo.isArray) {
                argInfo = decayToRValue(argInfo);
            }
            AssignmentCheckResult check = canAssignTypesInScope(&paramInfo, &argInfo, scope);
            if (check == ASSIGN_INCOMPATIBLE &&
                typeInfoIsPointerLike(&paramInfo) &&
                typeInfoIsInteger(&argInfo) &&
                node->functionCall.arguments &&
                isNullPointerConstant(node->functionCall.arguments[i], scope)) {
                check = ASSIGN_OK;
            }
            if (check == ASSIGN_QUALIFIER_LOSS) {
                if (calleeName && strcmp(calleeName, "core_dataset_add_table_typed") == 0) {
                    ASTNode* argNode = node->functionCall.arguments ? node->functionCall.arguments[i] : node;
                    char buf[256];
                    snprintf(buf, sizeof(buf),
                             "Argument %zu of '%s' discards qualifiers from pointer target",
                             i + 1,
                             calleeName);
                    addWarning(argNode->line, 0, buf, NULL);
                } else {
                    reportArgumentTypeError(node->functionCall.arguments ? node->functionCall.arguments[i] : node,
                                            i,
                                            calleeName,
                                            "discards qualifiers from pointer target");
                }
            } else if (check == ASSIGN_INCOMPATIBLE) {
                ASTNode* argNode = node->functionCall.arguments ? node->functionCall.arguments[i] : node;
                if (shouldDowngradeArgTypeMismatchToWarning(scope,
                                                            calleeName,
                                                            &paramInfo,
                                                            &argInfo)) {
                    char buf[200];
                    snprintf(buf,
                             sizeof(buf),
                             "Argument %zu of '%s' has incompatible type",
                             i + 1,
                             fallbackFunctionName(calleeName));
                    addWarning(argNode ? argNode->line : 0, 0, buf, NULL);
                } else {
                    reportArgumentTypeError(argNode,
                                            i,
                                            calleeName,
                                            "has incompatible type");
                }
            }
        }
        if (paramRestrict && argPaths) {
            for (size_t i = 0; i < pairCount; ++i) {
                if (!paramRestrict[i] || !argPaths[i]) continue;
                for (size_t j = i + 1; j < pairCount; ++j) {
                    if (!paramRestrict[j] || !argPaths[j]) continue;
                    if (strcmp(argPaths[i], argPaths[j]) == 0) {
                        addWarning(node->line, 0, "Restrict parameters may alias the same object", argPaths[i]);
                    }
                }
            }
        }
        if (sig->isVariadic && argInfos) {
            for (size_t i = expected; i < argCount; ++i) {
                argInfos[i] = defaultArgumentPromotion(argInfos[i]);
            }
        }
        result = functionCallResultTypeFromSymbol(sym, scope);

        free(paramRestrict);
        if (argPaths) {
            for (size_t i = 0; i < pairCount; ++i) {
                free(argPaths[i]);
            }
            free(argPaths);
        }
    }

    if (result.category == TYPEINFO_INVALID) {
        ParsedType fnTarget;
        bool haveTarget = callableTargetFromTypeInfo(&calleeInfo, scope, &fnTarget);

        if (haveTarget) {
            const ParsedFunctionSignature* callSig = NULL;
            for (size_t i = 0; i < fnTarget.derivationCount; ++i) {
                const TypeDerivation* deriv = parsedTypeGetDerivation(&fnTarget, i);
                if (deriv && deriv->kind == TYPE_DERIVATION_FUNCTION) {
                    callSig = &deriv->as.function;
                    break;
                }
            }

            if (callSig) {
                size_t expected = callSig->paramCount;
                bool tooFew = argCount < expected;
                bool tooMany = !callSig->isVariadic && argCount > expected;
                if (tooFew) {
                    reportArgumentCountError(node, calleeName, expected, argCount, true);
                }
                if (tooMany) {
                    reportArgumentCountError(node, calleeName, expected, argCount, false);
                }
            }

            parsedTypeFree(&fnTarget);
        }
    }

    if (result.category == TYPEINFO_INVALID) {
        ParsedType fnTarget;
        bool haveTarget = callableTargetFromTypeInfo(&calleeInfo, scope, &fnTarget);
        if (haveTarget) {
            ParsedType retParsed = parsedTypeFunctionReturnType(&fnTarget);
            if (retParsed.kind != TYPE_INVALID) {
                profiler_record_value("semantic_count_type_info_site_temp", 1);
                profiler_record_value("semantic_count_type_info_temp_fn_callable_ret", 1);
                result = typeInfoFromParsedType(&retParsed, scope);
                typeInfoAdoptParsedType(&result, &retParsed);
            }
            parsedTypeFree(&retParsed);
        }
        if (haveTarget) {
            parsedTypeFree(&fnTarget);
        }
    }

    free(argInfos);
    free(argRawInfos);

    if (result.category == TYPEINFO_INVALID) {
        result = makeIntegerType(32, true, TOKEN_INT);
    }
    result.isLValue = false;
    return result;
}

TypeInfo analyzePointerDereferenceExpression(ASTNode* node, Scope* scope) {
    TypeInfo base = analyzeExpression(node->pointerDeref.pointer, scope);
    if (base.category == TYPEINFO_POINTER && base.pointerDepth > 0) {
        if (base.originalType) {
            ParsedType targetParsed = parsedTypePointerTargetType(base.originalType);
            if (targetParsed.kind != TYPE_INVALID) {
                profiler_record_value("semantic_count_type_info_site_temp", 1);
                profiler_record_value("semantic_count_type_info_temp_deref_target", 1);
                TypeInfo targetInfo = typeInfoFromParsedType(&targetParsed, scope);
                typeInfoAdoptParsedType(&targetInfo, &targetParsed);
                targetInfo.isLValue = true;
                parsedTypeFree(&targetParsed);
                return targetInfo;
            }
            parsedTypeFree(&targetParsed);
        }
        typeInfoDropPointerLevel(&base);
        if (base.pointerDepth == 0) {
            restoreBaseCategory(&base);
        }
        base.isArray = false;
        base.isLValue = true;
        return base;
    }
    if (typeInfoIsKnown(&base)) {
        reportOperandError(node, "pointer operand", "*");
    }
    return makeInvalidType();
}

TypeInfo analyzeMemberAccessExpression(ASTNode* node, Scope* scope) {
    bool isArrowAccess = (node->type == AST_POINTER_ACCESS);
    TypeInfo base = analyzeExpression(node->memberAccess.base, scope);
    if (isArrowAccess) {
        if (base.isArray) {
            base = decayToRValue(base);
        }
        if (base.category == TYPEINFO_POINTER && base.pointerDepth > 0) {
            typeInfoDropPointerLevel(&base);
            if (base.pointerDepth == 0) {
                restoreBaseCategory(&base);
            }
            base.isArray = false;
            base.isLValue = true;
        } else if (typeInfoIsKnown(&base)) {
            reportOperandError(node, "pointer operand", "->");
            return makeInvalidType();
        }
    }

    if (!(base.category == TYPEINFO_STRUCT || base.category == TYPEINFO_UNION)) {
        if (typeInfoIsKnown(&base)) {
            reportOperandError(node, "struct/union operand", isArrowAccess ? "->" : ".");
        }
        return makeInvalidType();
    }

    const ParsedType* fieldType = analyzeExprLookupFieldType(&base, node->memberAccess.field, scope);
    if (fieldType) {
        profiler_record_value("semantic_count_type_info_site_temp", 1);
        profiler_record_value("semantic_count_type_info_temp_member_field", 1);
        TypeInfo fieldInfo = typeInfoFromParsedType(fieldType, scope);
        fieldInfo.originalType = fieldType;
        fieldInfo.isLValue = true;
        if (fieldInfo.category == TYPEINFO_POINTER) {
            const TypeDerivation* deriv = NULL;
            if (fieldType->derivationCount > 0) {
                deriv = parsedTypeGetDerivation(fieldType, fieldType->derivationCount - 1);
            }
            if (deriv && deriv->kind == TYPE_DERIVATION_POINTER) {
                fieldInfo.isConst = deriv->as.pointer.isConst;
                fieldInfo.isVolatile = deriv->as.pointer.isVolatile;
                fieldInfo.isRestrict = deriv->as.pointer.isRestrict;
            }
        }
        fieldInfo.isConst = base.isConst || parsedTypeTopLevelConst(fieldType);
        const CCTagFieldLayout* lay = analyzeExprLookupFieldLayout(&base, node->memberAccess.field, scope);
        if (lay && lay->isBitfield && !lay->isZeroWidth) {
            fieldInfo.isBitfield = true;
            fieldInfo.bitfieldLayout = lay;
        } else if (analyzeExprLookupFieldIsBitfield(&base, node->memberAccess.field, scope)) {
            fieldInfo.isBitfield = true;
        }
        return fieldInfo;
    }

    addError(node->line, 0, "Unknown field in member access", NULL);
    return makeInvalidType();
}

TypeInfo analyzeArrayAccessExpression(ASTNode* node, Scope* scope) {
    TypeInfo arrayInfo = analyzeExpression(node->arrayAccess.array, scope);
    analyzeExpression(node->arrayAccess.index, scope);
    if (arrayInfo.originalType && parsedTypeIsDirectArray(arrayInfo.originalType)) {
        ParsedType elementParsed = parsedTypeArrayElementType(arrayInfo.originalType);
        profiler_record_value("semantic_count_type_info_site_temp", 1);
        profiler_record_value("semantic_count_type_info_temp_array_element", 1);
        TypeInfo elementInfo = typeInfoFromParsedType(&elementParsed, scope);
        ParsedType* owned = malloc(sizeof(ParsedType));
        if (owned) {
            *owned = elementParsed;
            elementInfo.originalType = owned;
        } else {
            parsedTypeFree(&elementParsed);
            elementInfo.originalType = NULL;
        }
        elementInfo.isLValue = true;
        return elementInfo;
    }
    if (arrayInfo.isArray) {
        arrayInfo = decayToRValue(arrayInfo);
    }
    if (arrayInfo.category == TYPEINFO_POINTER && arrayInfo.pointerDepth > 0) {
        if (arrayInfo.originalType) {
            ParsedType targetParsed = parsedTypePointerTargetType(arrayInfo.originalType);
            if (targetParsed.kind != TYPE_INVALID) {
                profiler_record_value("semantic_count_type_info_site_temp", 1);
                profiler_record_value("semantic_count_type_info_temp_array_pointer_target", 1);
                TypeInfo targetInfo = typeInfoFromParsedType(&targetParsed, scope);
                typeInfoAdoptParsedType(&targetInfo, &targetParsed);
                parsedTypeFree(&targetParsed);
                targetInfo.isLValue = true;
                return targetInfo;
            }
            parsedTypeFree(&targetParsed);
        }
        typeInfoDropPointerLevel(&arrayInfo);
        if (arrayInfo.pointerDepth == 0) {
            restoreBaseCategory(&arrayInfo);
        }
        arrayInfo.isVLA = false;
        arrayInfo.isArray = false;
        arrayInfo.isLValue = true;
        return arrayInfo;
    }
    if (typeInfoIsKnown(&arrayInfo)) {
        reportOperandError(node, "pointer or array base", "[]");
    }
    return makeInvalidType();
}

TypeInfo analyzeCompoundLiteralExpression(ASTNode* node, Scope* scope) {
    for (size_t i = 0; i < node->compoundLiteral.entryCount; ++i) {
        analyzeDesignatedInitializerExpr(node->compoundLiteral.entries[i], scope);
    }
    inferCompoundLiteralArrayLength(node, scope);
    profiler_record_value("semantic_count_type_info_site_temp", 1);
    profiler_record_value("semantic_count_type_info_temp_compound_literal", 1);
    TypeInfo info = typeInfoFromParsedType(&node->compoundLiteral.literalType, scope);
    node->compoundLiteral.isStaticStorage = scope ? (scope->depth == 0) : false;
    info.isLValue = true;
    return info;
}

TypeInfo analyzeStatementExpressionValue(ASTNode* node, Scope* scope) {
    if (!node->statementExpr.block) {
        return makeInvalidType();
    }
    Scope* inner = createScope(scope);
    TypeInfo result = makeInvalidType();
    ASTNode* block = node->statementExpr.block;
    if (block->type == AST_BLOCK) {
        size_t count = block->block.statementCount;
        for (size_t i = 0; i < count; ++i) {
            ASTNode* stmt = block->block.statements[i];
            if (!stmt) continue;
            bool last = (i + 1 == count);
            if (last && isExpressionNodeType(stmt->type)) {
                result = analyzeExpression(stmt, inner);
            } else {
                analyze(stmt, inner);
            }
        }
    } else {
        analyze(block, inner);
    }
    destroyScope(inner);
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
        bool fnCompat = functionSymbolsCompatibleForConditional(trueFnSym, falseFnSym);
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

    if (truePtr && falsePtr) {
        if ((truePtrInfo.primitive == TOKEN_VOID && truePtrInfo.pointerDepth == falsePtrInfo.pointerDepth) ||
            (falsePtrInfo.primitive == TOKEN_VOID && truePtrInfo.pointerDepth == falsePtrInfo.pointerDepth)) {
            TypeInfo v = (truePtrInfo.primitive == TOKEN_VOID) ? truePtrInfo : falsePtrInfo;
            TypeInfo other = (truePtrInfo.primitive == TOKEN_VOID) ? falsePtrInfo : truePtrInfo;
            return mergePointerConditionalType(v, other);
        }
        if (typesAreEqual(&truePtrInfo, &falsePtrInfo)) {
            return mergePointerConditionalType(truePtrInfo, falsePtrInfo);
        }
        if (pointerTargetsCompatibleForConditional(&truePtrInfo, &falsePtrInfo)) {
            return mergePointerConditionalType(truePtrInfo, falsePtrInfo);
        }
    }

    if (truePtr && falseNull) {
        truePtrInfo.isLValue = false;
        return truePtrInfo;
    }
    if (falsePtr && trueNull) {
        falsePtrInfo.isLValue = false;
        return falsePtrInfo;
    }

    if (conditionValid &&
        trueInfo.category != TYPEINFO_INVALID &&
        falseInfo.category != TYPEINFO_INVALID) {
        addError(node->line, 0, "Incompatible types in ternary expression", NULL);
    }
    return makeInvalidType();
}
