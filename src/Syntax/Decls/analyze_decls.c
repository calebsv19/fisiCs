// SPDX-License-Identifier: Apache-2.0

#include "analyze_decls_internal.h"
#include "Extensions/extension_units_view.h"
#include "Utils/profiler.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static bool parsedTypeHasGnuWeakAttribute(const ParsedType* type) {
    if (!type || !type->attributes) return false;
    for (size_t i = 0; i < type->attributeCount; ++i) {
        const ASTAttribute* attr = type->attributes[i];
        if (!attr || attr->syntax != AST_ATTRIBUTE_SYNTAX_GNU || !attr->payload) {
            continue;
        }
        const char* word = "weak";
        size_t wordLength = strlen(word);
        const char* match = attr->payload;
        while ((match = strstr(match, word)) != NULL) {
            char left = match == attr->payload ? '\0' : match[-1];
            char right = match[wordLength];
            bool leftBoundary = left == '\0' ||
                                (!isalnum((unsigned char)left) && left != '_');
            bool rightBoundary = right == '\0' ||
                                 (!isalnum((unsigned char)right) && right != '_');
            if (leftBoundary && rightBoundary) return true;
            match += wordLength;
        }
    }
    return false;
}

static bool declSourcePathIsVirtualRemap(const char* path) {
    if (!path || !path[0]) return false;
    const char* base = strrchr(path, '/');
    base = base ? base + 1 : path;
    return strncmp(base, "virtual_", 8) == 0;
}

static SourceRange variableConflictSpellingRange(ASTNode* currentName,
                                                 ASTNode* currentDecl,
                                                 ASTNode* previousDecl) {
    SourceRange current =
        (currentName && currentName->location.start.file) ? currentName->location
                                                          : (currentDecl ? currentDecl->location
                                                                         : (SourceRange){0});
    if (declSourcePathIsVirtualRemap(current.start.file)) {
        return current;
    }

    SourceRange previous = varDeclBestSpellingRange(previousDecl);
    if (declSourcePathIsVirtualRemap(previous.start.file)) {
        return previous;
    }
    return current;
}

static SourceRange variableConflictMacroCallSite(ASTNode* currentName,
                                                 ASTNode* currentDecl,
                                                 ASTNode* previousDecl) {
    SourceRange current =
        (currentName && currentName->macroCallSite.start.file) ? currentName->macroCallSite
                                                               : (currentDecl ? currentDecl->macroCallSite
                                                                              : (SourceRange){0});
    SourceRange currentSpelling =
        (currentName && currentName->location.start.file) ? currentName->location
                                                          : (currentDecl ? currentDecl->location
                                                                         : (SourceRange){0});
    if (declSourcePathIsVirtualRemap(currentSpelling.start.file)) {
        return current;
    }

    SourceRange previous = varDeclBestMacroCallSite(previousDecl);
    SourceRange previousSpelling = varDeclBestSpellingRange(previousDecl);
    if (declSourcePathIsVirtualRemap(previousSpelling.start.file)) {
        return previous;
    }
    return current;
}

static SourceRange variableConflictMacroDefinition(ASTNode* currentName,
                                                   ASTNode* currentDecl,
                                                   ASTNode* previousDecl) {
    SourceRange current =
        (currentName && currentName->macroDefinition.start.file) ? currentName->macroDefinition
                                                                 : (currentDecl ? currentDecl->macroDefinition
                                                                                : (SourceRange){0});
    SourceRange currentSpelling =
        (currentName && currentName->location.start.file) ? currentName->location
                                                          : (currentDecl ? currentDecl->location
                                                                         : (SourceRange){0});
    if (declSourcePathIsVirtualRemap(currentSpelling.start.file)) {
        return current;
    }

    SourceRange previous = varDeclBestMacroDefinition(previousDecl);
    SourceRange previousSpelling = varDeclBestSpellingRange(previousDecl);
    if (declSourcePathIsVirtualRemap(previousSpelling.start.file)) {
        return previous;
    }
    return current;
}

static void addVariableTypeConflictError(ASTNode* currentName,
                                         ASTNode* currentDecl,
                                         ASTNode* previousDecl,
                                         const char* name) {
    addErrorWithRanges(variableConflictSpellingRange(currentName, currentDecl, previousDecl),
                       variableConflictMacroCallSite(currentName, currentDecl, previousDecl),
                       variableConflictMacroDefinition(currentName, currentDecl, previousDecl),
                       "Conflicting types for variable",
                       name);
}

static bool directArrayHasKnownBound(const ParsedType* type) {
    if (!parsedTypeIsDirectArray(type) || type->derivationCount == 0) {
        return false;
    }
    const TypeDerivation* deriv = &type->derivations[0];
    return deriv->kind == TYPE_DERIVATION_ARRAY &&
           deriv->as.array.hasConstantSize &&
           !deriv->as.array.isVLA &&
           !deriv->as.array.isFlexible;
}

static bool parsedTypesLexicallyCompatibleInScope(const ParsedType* a,
                                                  const ParsedType* b,
                                                  Scope* scope) {
    if (!parsedTypesStructurallyCompatibleInScope(a, b, scope)) {
        return false;
    }
    TypeInfo lhs = typeInfoFromParsedType(a, scope);
    TypeInfo rhs = typeInfoFromParsedType(b, scope);
    return !lhs.recordDefinition ||
           !rhs.recordDefinition ||
           lhs.recordDefinition == rhs.recordDefinition;
}

static bool shouldAdoptCompletedArrayDefinition(const Symbol* existing,
                                                const ParsedType* currentType,
                                                bool newDefinition) {
    if (!existing || !currentType || !newDefinition) {
        return false;
    }
    return parsedTypeIsDirectArray(&existing->type) &&
           !directArrayHasKnownBound(&existing->type) &&
           directArrayHasKnownBound(currentType);
}

static Symbol* resolveLinkedVariableInScopeChain(Scope* scope, const char* name) {
    for (Scope* current = scope; current; current = current->parent) {
        Symbol* found = lookupSymbol(&current->table, name);
        if (!found || found->kind != SYMBOL_VARIABLE) {
            continue;
        }
        if (found->linkage != LINKAGE_NONE) {
            return found;
        }
    }
    return NULL;
}

static const char* staticAssertHint(ASTNode* node) {
    if (!node || node->type != AST_STATIC_ASSERT) return NULL;
    if (!node->expr.right || node->expr.right->type != AST_STRING_LITERAL) return NULL;
    return node->expr.right->valueNode.value;
}

static void analyzeStaticAssertDeclaration(ASTNode* node, Scope* scope) {
    if (!node) return;
    ASTNode* condition = node->expr.left;
    long long value = 0;
    if (!condition || !constEvalInteger(condition, scope, &value, true)) {
        addErrorWithRanges(condition ? condition->location : node->location,
                           condition ? condition->macroCallSite : node->macroCallSite,
                           condition ? condition->macroDefinition : node->macroDefinition,
                           "Static assertion requires an integer constant expression",
                           staticAssertHint(node));
        return;
    }
    if (value == 0) {
        addErrorWithRanges(condition ? condition->location : node->location,
                           condition ? condition->macroCallSite : node->macroCallSite,
                           condition ? condition->macroDefinition : node->macroDefinition,
                           "Static assertion failed",
                           staticAssertHint(node));
    }
}

static void analyzeInlineAggregateDefinition(const ParsedType* type, Scope* scope) {
    if (!type) return;
    if (type->inlineStructOrUnionDef) {
        analyzeDeclaration(type->inlineStructOrUnionDef, scope);
    }
    if (type->inlineEnumDef) {
        analyzeDeclaration(type->inlineEnumDef, scope);
    }
}

static bool analyzeFunctionTypedefDeclarator(ASTNode* node,
                                             ASTNode* ident,
                                             ParsedType* surfaceType,
                                             Scope* scope,
                                             StorageClass storage,
                                             SymbolLinkage linkage,
                                             bool hasInitializer) {
    if (!node || !ident || !surfaceType || !scope ||
        surfaceType->kind != TYPE_NAMED ||
        surfaceType->pointerDepth != 0 ||
        surfaceType->derivationCount != 0 ||
        surfaceType->isFunctionPointer) {
        return false;
    }

    ParsedType functionType = parsedTypeClone(surfaceType);
    canonicalizeParsedTypeInScope(&functionType, scope);
    if (!parsedTypeIsDirectFunction(&functionType)) {
        parsedTypeFree(&functionType);
        return false;
    }

    if (hasInitializer) {
        addErrorWithRanges(ident->location,
                           ident->macroCallSite,
                           ident->macroDefinition,
                           "Function declaration cannot have an initializer",
                           ident->valueNode.value);
        parsedTypeFree(&functionType);
        return true;
    }

    ParsedType returnType = parsedTypeFunctionReturnType(&functionType);
    Symbol candidate = {0};
    candidate.type = returnType;
    assignFunctionSignatureFromParsedType(&candidate, &functionType, scope);

    Symbol* existing = lookupSymbol(&scope->table, ident->valueNode.value);
    bool linkedFromParentScope = false;
    if (!existing && !scopeIsFileScope(scope)) {
        for (Scope* current = scope->parent; current; current = current->parent) {
            Symbol* linked = lookupSymbol(&current->table, ident->valueNode.value);
            if (!linked) {
                continue;
            }
            if (linked->linkage != LINKAGE_NONE) {
                existing = linked;
                linkedFromParentScope = true;
            }
            break;
        }
    }
    if (existing) {
        if (linkedFromParentScope) {
            linkage = existing->linkage;
        }
        bool compatible =
            existing->kind == SYMBOL_FUNCTION &&
            parsedTypesLexicallyCompatibleInScope(&existing->type,
                                                  &candidate.type,
                                                  scope) &&
            functionSignaturesCompatible(&existing->signature,
                                         &candidate.signature,
                                         scope);
        bool allowExternAfterStatic =
            scopeIsFileScope(scope) &&
            storage == STORAGE_EXTERN &&
            existing->linkage == LINKAGE_INTERNAL &&
            linkage == LINKAGE_EXTERNAL;
        bool linkageConflict =
            !allowExternAfterStatic &&
            ((existing->linkage == LINKAGE_INTERNAL && linkage != LINKAGE_INTERNAL) ||
             (linkage == LINKAGE_INTERNAL && existing->linkage != LINKAGE_INTERNAL));
        compatible = compatible && !linkageConflict;
        if (!compatible) {
            addErrorWithRanges(ident->location,
                               ident->macroCallSite,
                               ident->macroDefinition,
                               "Conflicting types for function",
                               ident->valueNode.value);
        } else {
            mergeCompatibleFunctionSignatures(&existing->signature,
                                              &candidate.signature);
            applyInteropAttributes(existing, node, scope, true);
        }
        freeFunctionSignatureParameters(&candidate.signature);
        parsedTypeFree(&candidate.type);
        parsedTypeFree(&functionType);
        return true;
    }

    Symbol* sym = calloc(1, sizeof(Symbol));
    if (!sym) {
        freeFunctionSignatureParameters(&candidate.signature);
        parsedTypeFree(&candidate.type);
        parsedTypeFree(&functionType);
        return true;
    }
    sym->name = strdup(ident->valueNode.value);
    sym->kind = SYMBOL_FUNCTION;
    sym->type = candidate.type;
    sym->signature = candidate.signature;
    sym->definition = node;
    sym->storage = storage;
    sym->linkage = linkage;
    sym->hasDefinition = false;
    sym->isTentative = false;
    primeSymbolTypeInfoCache(sym, scope);
    applyInteropAttributes(sym, node, scope, true);
    if (!addToScope(scope, sym)) {
        addErrorWithRanges(ident->location,
                           ident->macroCallSite,
                           ident->macroDefinition,
                           "Conflicting types for function",
                           ident->valueNode.value);
    }
    parsedTypeFree(&functionType);
    return true;
}

void analyzeDeclaration(ASTNode* node, Scope* scope) {
    ProfilerScope declScope = profiler_begin("semantic_analyze_declaration");
    profiler_record_value("semantic_count_analyze_declaration", 1);
    switch (node->type) {
        case AST_STATIC_ASSERT:
            analyzeStaticAssertDeclaration(node, scope);
            break;
        case AST_VARIABLE_DECLARATION: {
            if (!validatePrimitiveSpecifierUsage(&node->varDecl.declaredType, node->line, NULL)) {
                break;
            }
            ParsedType* declaredTypes = node->varDecl.declaredTypes;
            for (size_t i = 0; i < node->varDecl.varCount; i++) {
                ASTNode* ident = node->varDecl.varNames[i];
                Symbol* boundSym = NULL;
                ParsedType* varType = declaredTypes ? &declaredTypes[i]
                                                    : &node->varDecl.declaredType;
                const char* nameHint = (ident && ident->type == AST_IDENTIFIER)
                                           ? ident->valueNode.value
                                           : NULL;
                int declLine = ident ? ident->line : node->line;

                ParsedType effectiveDeclaratorType = parsedTypeClone(varType);
                canonicalizeParsedTypeInScope(&effectiveDeclaratorType, scope);
                bool isFunctionTypedefDeclarator =
                    varType->kind == TYPE_NAMED &&
                    varType->pointerDepth == 0 &&
                    varType->derivationCount == 0 &&
                    !varType->isFunctionPointer &&
                    parsedTypeIsDirectFunction(&effectiveDeclaratorType);
                parsedTypeFree(&effectiveDeclaratorType);

                if (!validateStorageUsage(varType,
                                          scopeIsFileScope(scope),
                                          isFunctionTypedefDeclarator,
                                          false,
                                          declLine,
                                          nameHint)) {
                    continue;
                }
                if (!validateRestrictUsage(varType, scope, declLine, nameHint)) {
                    continue;
                }
                if (!validatePrimitiveSpecifierUsage(varType, declLine, nameHint)) {
                    continue;
                }
                analyzeInlineAggregateDefinition(varType, scope);
                freezeAggregateAliasesAtDeclaration(varType, scope, 0);
                evaluateArrayDerivations(varType, scope);
                if (varType->kind == TYPE_NAMED) {
                    ParsedType effectiveType = parsedTypeClone(varType);
                    canonicalizeParsedTypeInScope(&effectiveType, scope);
                    if (parsedTypeHasVLA(&effectiveType)) {
                        varType->isVLA = true;
                    }
                    parsedTypeFree(&effectiveType);
                }
                StorageClass storage = deduceStorageClass(varType);
                SymbolLinkage linkage =
                    isFunctionTypedefDeclarator && !scopeIsFileScope(scope)
                        ? LINKAGE_EXTERNAL
                        : deduceLinkage(varType, scopeIsFileScope(scope));
                bool hasInitializer = node->varDecl.initializers &&
                                      i < node->varDecl.varCount &&
                                      node->varDecl.initializers[i] != NULL;
                if (analyzeFunctionTypedefDeclarator(node,
                                                     ident,
                                                     varType,
                                                     scope,
                                                     storage,
                                                     linkage,
                                                     hasInitializer)) {
                    continue;
                }
                profiler_record_value("semantic_count_type_info_site_decl", 1);
                TypeInfo varInfo = typeInfoFromParsedType(varType, scope);
                bool fileScope = scopeIsFileScope(scope);
                if (!fileScope && storage == STORAGE_EXTERN && hasInitializer) {
                    addErrorWithRanges(ident ? ident->location : node->location,
                                       ident ? ident->macroCallSite : node->macroCallSite,
                                       ident ? ident->macroDefinition : node->macroDefinition,
                                       "Block-scope extern declaration cannot have an initializer",
                                       ident ? ident->valueNode.value : NULL);
                    continue;
                }
                bool tentative = fileScope &&
                                 linkage == LINKAGE_EXTERNAL &&
                                 storage != STORAGE_EXTERN &&
                                 !hasInitializer;
                bool newDefinition = false;
                if (storage == STORAGE_EXTERN) {
                    newDefinition = hasInitializer;
                } else if (!fileScope) {
                    newDefinition = true;
                } else if (storage == STORAGE_STATIC) {
                    newDefinition = true;
                } else {
                    newDefinition = !tentative || hasInitializer;
                }

                if ((varInfo.category == TYPEINFO_STRUCT || varInfo.category == TYPEINFO_UNION) &&
                    !varInfo.isComplete) {
                    addErrorWithRanges(ident ? ident->location : node->location,
                                       ident ? ident->macroCallSite : node->macroCallSite,
                                       ident ? ident->macroDefinition : node->macroDefinition,
                                       "Variable has incomplete type",
                                       ident ? ident->valueNode.value : NULL);
                    continue;
                }
                if (varInfo.category == TYPEINFO_ARRAY && !varInfo.isComplete) {
                    addErrorWithRanges(ident ? ident->location : node->location,
                                       ident ? ident->macroCallSite : node->macroCallSite,
                                       ident ? ident->macroDefinition : node->macroDefinition,
                                       "Array has incomplete element type",
                                       ident ? ident->valueNode.value : NULL);
                    continue;
                }

                const FisicsUnitsAnnotation* unitsAnn =
                    fisics_extension_lookup_units_annotation(scope ? scope->ctx : NULL, node);
                Symbol* existing = lookupSymbol(&scope->table, ident->valueNode.value);
                if (existing) {
                    if (existing->kind != SYMBOL_VARIABLE) {
                        addErrorWithRanges(ident ? ident->location : node->location,
                                           ident ? ident->macroCallSite : node->macroCallSite,
                                           ident ? ident->macroDefinition : node->macroDefinition,
                                           "Conflicting declaration kind",
                                           ident ? ident->valueNode.value : NULL);
                        continue;
                    }
                    if (existing->isParameter) {
                        addErrorWithRanges(ident ? ident->location : node->location,
                                           ident ? ident->macroCallSite : node->macroCallSite,
                                           ident ? ident->macroDefinition : node->macroDefinition,
                                           "Redefinition of parameter",
                                           ident ? ident->valueNode.value : NULL);
                        continue;
                    }
                    if (existing->linkage != linkage) {
                        bool conflict =
                            (existing->linkage == LINKAGE_INTERNAL && linkage != LINKAGE_INTERNAL) ||
                            (linkage == LINKAGE_INTERNAL && existing->linkage != LINKAGE_INTERNAL);
                        if (conflict) {
                            addErrorWithRanges(ident ? ident->location : node->location,
                                               ident ? ident->macroCallSite : node->macroCallSite,
                                               ident ? ident->macroDefinition : node->macroDefinition,
                                               "Conflicting linkage for variable",
                                               ident ? ident->valueNode.value : NULL);
                            continue;
                        }
                        if (existing->linkage == LINKAGE_NONE) {
                            existing->linkage = linkage;
                        }
                    }
                    if (!parsedTypesStructurallyCompatibleInScope(&existing->type, varType, scope)) {
                        addVariableTypeConflictError(ident,
                                                     node,
                                                     existing->definition,
                                                     ident ? ident->valueNode.value : NULL);
                        continue;
                    }
                    if (existing->hasDefinition && newDefinition && !existing->isTentative) {
                        addErrorWithRanges(ident ? ident->location : node->location,
                                           ident ? ident->macroCallSite : node->macroCallSite,
                                           ident ? ident->macroDefinition : node->macroDefinition,
                                           "Redefinition of variable",
                                           ident ? ident->valueNode.value : NULL);
                        continue;
                    }
                    existing->isTentative = existing->isTentative || tentative;
                    if (newDefinition) {
                        if (shouldAdoptCompletedArrayDefinition(existing, varType, newDefinition)) {
                            existing->type = *varType;
                        }
                        existing->hasDefinition = true;
                        existing->isTentative = false;
                        existing->definition = node;
                    }
                    existing->isWeak = existing->isWeak ||
                                       parsedTypeHasGnuWeakAttribute(varType);
                    symbolAttachUnitsAnnotation(existing, unitsAnn, i);
                    applyInteropAttributes(existing, node, scope, true);
                    primeSymbolTypeInfoCache(existing, scope);
                    boundSym = existing;
                } else {
                    if (!fileScope && storage == STORAGE_EXTERN) {
                        Symbol* linked =
                            resolveLinkedVariableInScopeChain(scope, ident->valueNode.value);
                        if (!linked) {
                            for (Scope* current = scope; current; current = current->parent) {
                                Symbol* candidate =
                                    lookupSymbol(&current->table, ident->valueNode.value);
                                if (candidate && candidate->linkage != LINKAGE_NONE) {
                                    linked = candidate;
                                    break;
                                }
                            }
                        }
                        if (linked) {
                            if (linked->kind != SYMBOL_VARIABLE) {
                                addErrorWithRanges(ident ? ident->location : node->location,
                                                   ident ? ident->macroCallSite : node->macroCallSite,
                                                   ident ? ident->macroDefinition : node->macroDefinition,
                                                   "Conflicting declaration kind",
                                                   ident ? ident->valueNode.value : NULL);
                                continue;
                            }
                            if (!parsedTypesStructurallyCompatibleInScope(&linked->type, varType, scope)) {
                                addVariableTypeConflictError(ident,
                                                             node,
                                                             linked->definition,
                                                             ident ? ident->valueNode.value : NULL);
                                continue;
                            }
                        }
                    }
                    Symbol* sym = calloc(1, sizeof(Symbol));
                    if (!sym) continue;
                    sym->name = strdup(ident->valueNode.value);
                    sym->kind = SYMBOL_VARIABLE;
                    sym->type = *varType;
                    sym->definition = node;
                    sym->storage = storage;
                    sym->linkage = linkage;
                    sym->hasDefinition = newDefinition;
                    sym->isTentative = tentative;
                    sym->isWeak = parsedTypeHasGnuWeakAttribute(varType);
                    sym->next = NULL;
                    symbolAttachUnitsAnnotation(sym, unitsAnn, i);
                    resetFunctionSignature(sym);
                    primeSymbolTypeInfoCache(sym, scope);
                    applyInteropAttributes(sym, node, scope, true);

                    if (!addToScope(scope, sym)) {
                        addErrorWithRanges(ident ? ident->location : node->location,
                                           ident ? ident->macroCallSite : node->macroCallSite,
                                           ident ? ident->macroDefinition : node->macroDefinition,
                                           "Redefinition of variable",
                                           ident ? ident->valueNode.value : NULL);
                        free(sym->name);
                        free(sym);
                        continue;
                    }
                    boundSym = sym;
                }

                bool staticStorage = scopeIsFileScope(scope) ||
                                     (varType && (varType->isStatic || varType->isExtern));
                if (varType) {
                    evaluateArrayDerivations(varType, scope);
                }
                const ParsedType* resolvedVar = resolveTypedefInScope(varType, scope);
                bool canAdoptTypedefArrayShape =
                    varType &&
                    varType->pointerDepth == 0 &&
                    varType->derivationCount == 0 &&
                    !varType->isFunctionPointer;
                const ParsedType* arrayType =
                    (canAdoptTypedefArrayShape &&
                     resolvedVar &&
                     parsedTypeIsDirectArray(resolvedVar))
                        ? resolvedVar
                        : varType;
                bool isArrayVar = arrayType && parsedTypeIsDirectArray(arrayType);
                if (isArrayVar) {
                    if (parsedTypeHasVLA(arrayType) && staticStorage) {
                        char buffer[256];
                        snprintf(buffer,
                                 sizeof(buffer),
                                 "Variable-length array '%s' is not allowed at static storage duration",
                                 ident && ident->valueNode.value
                                     ? ident->valueNode.value
                                     : "<unnamed>");
                        addError(ident ? ident->line : node->line, 0, buffer, NULL);
                    }
                }

                if (i < node->varDecl.varCount && node->varDecl.initializers) {
                    DesignatedInit* init = node->varDecl.initializers[i];
                    if (boundSym) {
                        boundSym->initializer = init;
                    }
                    analyzeDesignatedInitializer(init, scope);
                    maybeRecordConstIntegerValue(boundSym, varType, init, scope);
                    if (isArrayVar) {
                        validateVariableArrayInitializer((ParsedType*)arrayType, init, ident, scope);
                    } else {
                        validateVariableInitializer(varType, init, ident, scope, staticStorage);
                    }
                }
            }
            break;
        }

        case AST_FUNCTION_DECLARATION:
        case AST_FUNCTION_DEFINITION: {
            ASTNode* funcName = node->type == AST_FUNCTION_DEFINITION
                                    ? node->functionDef.funcName
                                    : node->functionDecl.funcName;
            ParsedType* returnType = node->type == AST_FUNCTION_DEFINITION
                                         ? &node->functionDef.returnType
                                         : &node->functionDecl.returnType;
            const char* funcHint = (funcName && funcName->type == AST_IDENTIFIER)
                                       ? funcName->valueNode.value
                                       : NULL;
            int funcLine = funcName ? funcName->line : node->line;
            bool fileScope = scopeIsFileScope(scope);

            if (!validateStorageUsage(returnType,
                                      fileScope,
                                      true,
                                      false,
                                      funcLine,
                                      funcHint)) {
                break;
            }
            if (!validateRestrictUsage(returnType, scope, funcLine, funcHint)) {
                break;
            }
            if (!validatePrimitiveSpecifierUsage(returnType, funcLine, funcHint)) {
                break;
            }
            freezeAggregateAliasesAtDeclaration(returnType, scope, 0);
            ASTNode** params = node->type == AST_FUNCTION_DEFINITION
                                   ? node->functionDef.parameters
                                   : node->functionDecl.parameters;
            size_t paramCount = node->type == AST_FUNCTION_DEFINITION
                                    ? node->functionDef.paramCount
                                    : node->functionDecl.paramCount;
            bool isVariadic = node->type == AST_FUNCTION_DEFINITION
                                  ? node->functionDef.isVariadic
                                  : node->functionDecl.isVariadic;
            for (size_t p = 0; p < paramCount; ++p) {
                ASTNode* param = params ? params[p] : NULL;
                if (!param || param->type != AST_VARIABLE_DECLARATION) {
                    continue;
                }
                ParsedType* paramTypes = param->varDecl.declaredTypes;
                for (size_t v = 0; v < param->varDecl.varCount; ++v) {
                    ParsedType* paramType = paramTypes
                        ? &paramTypes[v]
                        : &param->varDecl.declaredType;
                    freezeAggregateAliasesAtDeclaration(paramType, scope, 0);
                }
            }
            if (!validateFunctionParameters(params,
                                            paramCount,
                                            isVariadic,
                                            scope,
                                            funcLine,
                                            funcHint)) {
                break;
            }
            StorageClass storage = deduceStorageClass(node->type == AST_FUNCTION_DEFINITION
                                                          ? &node->functionDef.returnType
                                                          : &node->functionDecl.returnType);
            SymbolLinkage linkage = deduceLinkage(node->type == AST_FUNCTION_DEFINITION
                                                      ? &node->functionDef.returnType
                                                      : &node->functionDecl.returnType,
                                                  scopeIsFileScope(scope));
            bool isDefinition = (node->type == AST_FUNCTION_DEFINITION);

            Symbol* existing = lookupSymbol(&scope->table, funcName->valueNode.value);
            if (existing && existing->kind == SYMBOL_FUNCTION) {
                if (!existing->definition) {
                    ParsedType reboundType = parsedTypeClone(
                        node->type == AST_FUNCTION_DEFINITION
                            ? &node->functionDef.returnType
                            : &node->functionDecl.returnType);
                    if (reboundType.kind == TYPE_INVALID) {
                        parsedTypeFree(&reboundType);
                        break;
                    }
                    parsedTypeFree(&existing->type);
                    existing->type = reboundType;
                    invalidateSymbolTypeInfoCache(existing);
                    resetFunctionSignature(existing);
                    if (node->type == AST_FUNCTION_DEFINITION) {
                        assignFunctionSignature(existing,
                                                node->functionDef.parameters,
                                                node->functionDef.paramCount,
                                                node->functionDef.isVariadic,
                                                node->functionDef.hasPrototype,
                                                scope);
                    } else {
                        assignFunctionSignature(existing,
                                                node->functionDecl.parameters,
                                                node->functionDecl.paramCount,
                                                node->functionDecl.isVariadic,
                                                node->functionDecl.hasPrototype,
                                                scope);
                    }
                    existing->definition = node;
                    existing->storage = storage;
                    existing->linkage = linkage;
                    existing->hasDefinition = isDefinition;
                    primeSymbolTypeInfoCache(existing, scope);
                    applyInteropAttributes(existing, node, scope, true);
                    break;
                }
                if (node->type == AST_FUNCTION_DEFINITION &&
                    existing->definition &&
                    existing->definition->type == AST_FUNCTION_DEFINITION) {
                    addErrorWithRanges(funcName ? funcName->location : node->location,
                                       funcName ? funcName->macroCallSite : node->macroCallSite,
                                       funcName ? funcName->macroDefinition : node->macroDefinition,
                                       "Redefinition of function",
                                       funcName ? funcName->valueNode.value : NULL);
                    break;
                }

                if (existing->linkage != linkage) {
                    bool allowExternAfterStatic =
                        fileScope &&
                        storage == STORAGE_EXTERN &&
                        existing->linkage == LINKAGE_INTERNAL &&
                        linkage == LINKAGE_EXTERNAL;
                    bool conflict =
                        !allowExternAfterStatic &&
                        ((existing->linkage == LINKAGE_INTERNAL && linkage != LINKAGE_INTERNAL) ||
                         (linkage == LINKAGE_INTERNAL && existing->linkage != LINKAGE_INTERNAL));
                    if (conflict) {
                        addErrorWithRanges(funcName ? funcName->location : node->location,
                                           funcName ? funcName->macroCallSite : node->macroCallSite,
                                           funcName ? funcName->macroDefinition : node->macroDefinition,
                                           "Conflicting linkage for function",
                                           funcName ? funcName->valueNode.value : NULL);
                        break;
                    }
                    if (allowExternAfterStatic) {
                        linkage = LINKAGE_INTERNAL;
                    } else if (existing->linkage == LINKAGE_NONE) {
                        existing->linkage = linkage;
                    }
                }

                if (!parsedTypesLexicallyCompatibleInScope(
                        &existing->type,
                        node->type == AST_FUNCTION_DEFINITION
                            ? &node->functionDef.returnType
                            : &node->functionDecl.returnType,
                        scope)) {
                    addErrorWithRanges(funcName ? funcName->location : node->location,
                                       funcName ? funcName->macroCallSite : node->macroCallSite,
                                       funcName ? funcName->macroDefinition : node->macroDefinition,
                                       "Conflicting types for function",
                                       funcName ? funcName->valueNode.value : NULL);
                    break;
                }

                {
                    Symbol tmp = {0};
                    resetFunctionSignature(&tmp);
                    if (node->type == AST_FUNCTION_DEFINITION) {
                        assignFunctionSignature(&tmp,
                                                node->functionDef.parameters,
                                                node->functionDef.paramCount,
                                                node->functionDef.isVariadic,
                                                node->functionDef.hasPrototype,
                                                scope);
                    } else {
                        assignFunctionSignature(&tmp,
                                                node->functionDecl.parameters,
                                                node->functionDecl.paramCount,
                                                node->functionDecl.isVariadic,
                                                node->functionDecl.hasPrototype,
                                                scope);
                    }
                    if (!functionSignaturesCompatible(&existing->signature, &tmp.signature, scope)) {
                        addErrorWithRanges(funcName ? funcName->location : node->location,
                                           funcName ? funcName->macroCallSite : node->macroCallSite,
                                           funcName ? funcName->macroDefinition : node->macroDefinition,
                                           "Conflicting types for function",
                                           funcName ? funcName->valueNode.value : NULL);
                        freeFunctionSignatureParameters(&tmp.signature);
                        break;
                    }
                    mergeCompatibleParsedTypeDetailsInScope(
                        &existing->type,
                        node->type == AST_FUNCTION_DEFINITION
                            ? &node->functionDef.returnType
                            : &node->functionDecl.returnType,
                        scope);
                    mergeCompatibleFunctionSignatures(&existing->signature,
                                                      &tmp.signature);
                    freeFunctionSignatureParameters(&tmp.signature);
                }

                if (existing->hasDefinition && isDefinition) {
                    addErrorWithRanges(funcName ? funcName->location : node->location,
                                       funcName ? funcName->macroCallSite : node->macroCallSite,
                                       funcName ? funcName->macroDefinition : node->macroDefinition,
                                       "Redefinition of function",
                                       funcName ? funcName->valueNode.value : NULL);
                    break;
                }

                if (node->type == AST_FUNCTION_DEFINITION) {
                    /* mergeCompatibleParsedTypeDetailsInScope above installs an
                       owned canonical copy. Do not replace it with the
                       definition AST's borrowed return type: later compatible
                       redeclarations may free the symbol type while codegen
                       still consults it. */
                    invalidateSymbolTypeInfoCache(existing);
                    existing->definition = node;
                    if (node->functionDef.hasPrototype ||
                        !existing->signature.hasPrototype) {
                        resetFunctionSignature(existing);
                        assignFunctionSignature(existing,
                                                node->functionDef.parameters,
                                                node->functionDef.paramCount,
                                                node->functionDef.isVariadic,
                                                node->functionDef.hasPrototype,
                                                scope);
                    }
                    existing->hasDefinition = true;
                }
                primeSymbolTypeInfoCache(existing, scope);
                applyInteropAttributes(existing, node, scope, true);
                break;
            }

            if (!existing && !fileScope) {
                bool conflictingLinkedObject = false;
                Symbol* linkedFunction = NULL;
                for (Scope* current = scope->parent; current; current = current->parent) {
                    Symbol* linked = lookupSymbol(&current->table, funcName->valueNode.value);
                    if (!linked || linked->linkage == LINKAGE_NONE) {
                        continue;
                    }
                    if (linked->kind != SYMBOL_FUNCTION) {
                        addErrorWithRanges(funcName ? funcName->location : node->location,
                                           funcName ? funcName->macroCallSite : node->macroCallSite,
                                           funcName ? funcName->macroDefinition : node->macroDefinition,
                                           "Conflicting declaration kind",
                                           funcName ? funcName->valueNode.value : NULL);
                        conflictingLinkedObject = true;
                        break;
                    }
                    linkedFunction = linked;
                    break;
                }
                if (conflictingLinkedObject) {
                    break;
                }
                if (linkedFunction) {
                    const ParsedType* currentReturn =
                        node->type == AST_FUNCTION_DEFINITION
                            ? &node->functionDef.returnType
                            : &node->functionDecl.returnType;
                    bool compatible = parsedTypesLexicallyCompatibleInScope(
                        &linkedFunction->type, currentReturn, scope);

                    Symbol tmp = {0};
                    resetFunctionSignature(&tmp);
                    if (node->type == AST_FUNCTION_DEFINITION) {
                        assignFunctionSignature(&tmp,
                                                node->functionDef.parameters,
                                                node->functionDef.paramCount,
                                                node->functionDef.isVariadic,
                                                node->functionDef.hasPrototype,
                                                scope);
                    } else {
                        assignFunctionSignature(&tmp,
                                                node->functionDecl.parameters,
                                                node->functionDecl.paramCount,
                                                node->functionDecl.isVariadic,
                                                node->functionDecl.hasPrototype,
                                                scope);
                    }
                    if (compatible) {
                        compatible = functionSignaturesCompatible(
                            &linkedFunction->signature, &tmp.signature, scope);
                    }
                    if (compatible &&
                        linkedFunction->signature.hasPrototype &&
                        tmp.signature.hasPrototype &&
                        linkedFunction->signature.paramCount == tmp.signature.paramCount) {
                        for (size_t p = 0; p < tmp.signature.paramCount; ++p) {
                            if (!parsedTypesLexicallyCompatibleInScope(
                                    &linkedFunction->signature.params[p],
                                    &tmp.signature.params[p],
                                    scope)) {
                                compatible = false;
                                break;
                            }
                        }
                    }
                    if (!compatible) {
                        freeFunctionSignatureParameters(&tmp.signature);
                        addErrorWithRanges(
                            funcName ? funcName->location : node->location,
                            funcName ? funcName->macroCallSite : node->macroCallSite,
                            funcName ? funcName->macroDefinition : node->macroDefinition,
                            "Conflicting types for function",
                            funcName ? funcName->valueNode.value : NULL);
                        break;
                    }
                    mergeCompatibleFunctionSignatures(&linkedFunction->signature,
                                                      &tmp.signature);
                    freeFunctionSignatureParameters(&tmp.signature);
                    linkage = linkedFunction->linkage;
                }
            }

            Symbol* sym = calloc(1, sizeof(Symbol));
            if (!sym) break;
            sym->name = strdup(funcName->valueNode.value);
            sym->kind = SYMBOL_FUNCTION;
            sym->type = parsedTypeClone(
                node->type == AST_FUNCTION_DEFINITION
                    ? &node->functionDef.returnType
                    : &node->functionDecl.returnType);
            if (sym->type.kind == TYPE_INVALID) {
                parsedTypeFree(&sym->type);
                free(sym->name);
                free(sym);
                break;
            }
            sym->definition = node;
            sym->storage = storage;
            sym->linkage = linkage;
            sym->hasDefinition = isDefinition;
            sym->isTentative = false;
            sym->next = NULL;
            resetFunctionSignature(sym);
            primeSymbolTypeInfoCache(sym, scope);
            if (node->type == AST_FUNCTION_DEFINITION) {
                assignFunctionSignature(sym,
                                        node->functionDef.parameters,
                                        node->functionDef.paramCount,
                                        node->functionDef.isVariadic,
                                        node->functionDef.hasPrototype,
                                        scope);
            } else {
                assignFunctionSignature(sym,
                                        node->functionDecl.parameters,
                                        node->functionDecl.paramCount,
                                        node->functionDecl.isVariadic,
                                        node->functionDecl.hasPrototype,
                                        scope);
            }
            applyInteropAttributes(sym, node, scope, true);

            if (!addToScope(scope, sym)) {
                addErrorWithRanges(funcName ? funcName->location : node->location,
                                   funcName ? funcName->macroCallSite : node->macroCallSite,
                                   funcName ? funcName->macroDefinition : node->macroDefinition,
                                   "Redefinition of function",
                                   funcName ? funcName->valueNode.value : NULL);
            }
            break;
        }

        case AST_TYPEDEF: {
            analyzeInlineAggregateDefinition(&node->typedefStmt.baseType, scope);
            if (scopeIsFileScope(scope) && scope->ctx &&
                !node->typedefStmt.baseType.inlineStructOrUnionDef &&
                node->typedefStmt.baseType.userTypeName &&
                (node->typedefStmt.baseType.kind == TYPE_STRUCT ||
                 node->typedefStmt.baseType.kind == TYPE_UNION)) {
                CCTagKind tagKind = node->typedefStmt.baseType.kind == TYPE_UNION
                                        ? CC_TAG_UNION
                                        : CC_TAG_STRUCT;
                (void)cc_add_tag(scope->ctx,
                                 tagKind,
                                 node->typedefStmt.baseType.userTypeName);
            }
            if (!node->typedefStmt.baseType.inlineStructOrUnionDef &&
                (node->typedefStmt.baseType.kind == TYPE_STRUCT ||
                 node->typedefStmt.baseType.kind == TYPE_UNION) &&
                node->typedefStmt.baseType.userTypeName) {
                Symbol* tagSym = resolveTagInScopeChain(scope,
                                                        node->typedefStmt.baseType.userTypeName);
                if (tagSym && tagSym->definition) {
                    node->typedefStmt.baseType.inlineStructOrUnionDef = tagSym->definition;
                }
            }
            freezeAggregateAliasesAtDeclaration(&node->typedefStmt.baseType, scope, 0);
            const char* aliasName = node->typedefStmt.alias->valueNode.value;
            int aliasLine = node->typedefStmt.alias ? node->typedefStmt.alias->line : node->line;
            if (!validateStorageUsage(&node->typedefStmt.baseType,
                                      scopeIsFileScope(scope),
                                      false,
                                      true,
                                      aliasLine,
                                      aliasName)) {
                break;
            }
            if (!validateRestrictUsage(&node->typedefStmt.baseType, scope, aliasLine, aliasName)) {
                break;
            }
            if (!validatePrimitiveSpecifierUsage(&node->typedefStmt.baseType, aliasLine, aliasName)) {
                break;
            }
            evaluateArrayDerivations(&node->typedefStmt.baseType, scope);
            if (node->typedefStmt.baseType.kind == TYPE_NAMED) {
                ParsedType effectiveType = parsedTypeClone(&node->typedefStmt.baseType);
                canonicalizeParsedTypeInScope(&effectiveType, scope);
                if (parsedTypeHasVLA(&effectiveType)) {
                    node->typedefStmt.baseType.isVLA = true;
                }
                parsedTypeFree(&effectiveType);
            }
            Symbol* existing = lookupSymbol(&scope->table, aliasName);
            if (existing && existing->kind == SYMBOL_TYPEDEF) {
                if (parsedTypesStructurallyEqual(&existing->type, &node->typedefStmt.baseType)) {
                    break;
                }
                const ParsedType* existingBase = resolveTypedefBase(scope, &existing->type, 0);
                const ParsedType* newBase = resolveTypedefBase(scope, &node->typedefStmt.baseType, 0);
                if (parsedTypesStructurallyEqual(existingBase, newBase)) {
                    break;
                }
                if (typedefChainContainsName(scope, &existing->type, aliasName, 0) &&
                    typedefChainContainsName(scope, &node->typedefStmt.baseType, aliasName, 0)) {
                    break;
                }
                if (existing->definition == NULL) {
                    existing->type = node->typedefStmt.baseType;
                    existing->definition = node;
                    primeSymbolTypeInfoCache(existing, scope);
                    break;
                }
            }

            Symbol* sym = calloc(1, sizeof(Symbol));
            if (!sym) break;
            sym->name = strdup(aliasName);
            sym->kind = SYMBOL_TYPEDEF;
            sym->type = node->typedefStmt.baseType;
            sym->definition = node;
            sym->next = NULL;
            resetFunctionSignature(sym);
            primeSymbolTypeInfoCache(sym, scope);

            if (!addToScope(scope, sym)) {
                addErrorWithRanges(node ? node->location : (SourceRange){0},
                                   node ? node->macroCallSite : (SourceRange){0},
                                   node ? node->macroDefinition : (SourceRange){0},
                                   "Redefinition of typedef",
                                   sym->name);
            }
            if (scope->ctx) {
                cc_add_typedef(scope->ctx, aliasName);
            }
            break;
        }

        case AST_STRUCT_DEFINITION:
        case AST_UNION_DEFINITION:
        case AST_ENUM_DEFINITION: {
            ASTNode* nameNode = (node->type == AST_ENUM_DEFINITION)
                                    ? node->enumDef.enumName
                                    : node->structDef.structName;
            if (!nameNode || nameNode->type != AST_IDENTIFIER || !scope->ctx) {
                break;
            }

            Symbol* scopedTag = calloc(1, sizeof(Symbol));
            if (scopedTag) {
                scopedTag->name = strdup(nameNode->valueNode.value);
                scopedTag->kind = node->type == AST_ENUM_DEFINITION ? SYMBOL_ENUM : SYMBOL_STRUCT;
                scopedTag->definition = node;
                if (!scopedTag->name || !addTagToScope(scope, scopedTag)) {
                    free(scopedTag->name);
                    free(scopedTag);
                }
            }
            bool fileScope = scopeIsFileScope(scope);

            CCTagKind tagKind = CC_TAG_STRUCT;
            uint64_t fingerprint = 0;
            const char* kindLabel = "struct";
            bool isForward = false;

            if (node->type == AST_UNION_DEFINITION) {
                tagKind = CC_TAG_UNION;
                kindLabel = "union";
                fingerprint = fingerprintStructLike(node);
                isForward = (node->structDef.fieldCount == 0);
            } else if (node->type == AST_ENUM_DEFINITION) {
                tagKind = CC_TAG_ENUM;
                kindLabel = "enum";
                fingerprint = fingerprintEnumDefinition(node);
                isForward = false;
            } else {
                fingerprint = fingerprintStructLike(node);
                isForward = (node->structDef.fieldCount == 0);
            }

            if (fileScope) {
                bool crossKindConflict = false;
                if (tagKind != CC_TAG_STRUCT &&
                    cc_has_tag(scope->ctx, CC_TAG_STRUCT, nameNode->valueNode.value)) {
                    crossKindConflict = true;
                }
                if (tagKind != CC_TAG_UNION &&
                    cc_has_tag(scope->ctx, CC_TAG_UNION, nameNode->valueNode.value)) {
                    crossKindConflict = true;
                }
                if (tagKind != CC_TAG_ENUM &&
                    cc_has_tag(scope->ctx, CC_TAG_ENUM, nameNode->valueNode.value)) {
                    crossKindConflict = true;
                }
                if (crossKindConflict) {
                    char buffer[128];
                    snprintf(buffer,
                             sizeof(buffer),
                             "Conflicting tag name for %s '%s'",
                             kindLabel,
                             nameNode->valueNode.value);
                    addErrorWithRanges(nameNode ? nameNode->location : node->location,
                                       nameNode ? nameNode->macroCallSite : node->macroCallSite,
                                       nameNode ? nameNode->macroDefinition : node->macroDefinition,
                                       buffer,
                                       NULL);
                    break;
                }
            }

            if (isForward && tagKind != CC_TAG_ENUM) {
                if (fileScope) {
                    if (!cc_add_tag(scope->ctx, tagKind, nameNode->valueNode.value)) {
                        char buffer[128];
                        snprintf(buffer,
                                 sizeof(buffer),
                                 "Conflicting tag name for %s '%s'",
                                 kindLabel,
                                 nameNode->valueNode.value);
                        addErrorWithRanges(nameNode ? nameNode->location : node->location,
                                           nameNode ? nameNode->macroCallSite : node->macroCallSite,
                                           nameNode ? nameNode->macroDefinition : node->macroDefinition,
                                           buffer,
                                           NULL);
                    }
                }
            } else {
                if (!fileScope && tagKind != CC_TAG_ENUM) {
                    if (!cc_has_tag(scope->ctx, tagKind, nameNode->valueNode.value)) {
                        (void)cc_add_tag(scope->ctx, tagKind, nameNode->valueNode.value);
                    }
                    if (!cc_tag_is_defined(scope->ctx, tagKind, nameNode->valueNode.value)) {
                        CCTagDefineResult result = cc_define_tag(scope->ctx,
                                                                 tagKind,
                                                                 nameNode->valueNode.value,
                                                                 fingerprint,
                                                                 node);
                        if (result == CC_TAGDEF_CONFLICT) {
                            char buffer[128];
                            snprintf(buffer,
                                     sizeof(buffer),
                                     "Conflicting definition of %s '%s'",
                                     kindLabel,
                                     nameNode->valueNode.value);
                            addError(nameNode ? nameNode->line : node->line, 0, buffer, NULL);
                        }
                    }
                } else {
                    CCTagDefineResult result = cc_define_tag(scope->ctx,
                                                             tagKind,
                                                             nameNode->valueNode.value,
                                                             fingerprint,
                                                             node);
                    if (result == CC_TAGDEF_CONFLICT) {
                        char buffer[128];
                        snprintf(buffer,
                                 sizeof(buffer),
                                 "Conflicting definition of %s '%s'",
                                 kindLabel,
                                 nameNode->valueNode.value);
                        addError(nameNode ? nameNode->line : node->line, 0, buffer, NULL);
                    }
                }
            }

            if (node->type == AST_ENUM_DEFINITION) {
                ParsedType enumValueType = makeEnumValueParsedType();
                long long currentValue = 0;
                bool haveCurrent = false;
                for (size_t i = 0; i < node->enumDef.memberCount; ++i) {
                    ASTNode* member = node->enumDef.members ? node->enumDef.members[i] : NULL;
                    if (!member || member->type != AST_IDENTIFIER) {
                        continue;
                    }
                    long long enumVal = 0;
                    bool hasValue = false;
                    if (node->enumDef.values && node->enumDef.values[i]) {
                        ASTNode* valueExpr = node->enumDef.values[i];
                        if (enumExprHasOverflowingIntegerLiteral(valueExpr, scope)) {
                            addErrorWithRanges(member ? member->location : node->location,
                                               member ? member->macroCallSite : node->macroCallSite,
                                               member ? member->macroDefinition : node->macroDefinition,
                                               "Enumerator value contains an out-of-range integer literal",
                                               member ? member->valueNode.value : NULL);
                        }
                        ConstEvalResult val = constEvalIntegerResult(valueExpr, scope, true);
                        if (!val.isConst) {
                            addErrorWithRanges(member ? member->location : node->location,
                                               member ? member->macroCallSite : node->macroCallSite,
                                               member ? member->macroDefinition : node->macroDefinition,
                                               "Enumerator value is not a constant expression",
                                               member ? member->valueNode.value : NULL);
                        } else {
                            enumVal = val.value;
                            hasValue = true;
                            if (!enumValueFitsIntRange(enumVal, scope)) {
                                addErrorWithRanges(member ? member->location : node->location,
                                                   member ? member->macroCallSite : node->macroCallSite,
                                                   member ? member->macroDefinition : node->macroDefinition,
                                                   "Enumerator value is out of range for type 'int'",
                                                   member ? member->valueNode.value : NULL);
                            }
                        }
                    } else if (haveCurrent) {
                        enumVal = currentValue + 1;
                        hasValue = true;
                        if (!enumValueFitsIntRange(enumVal, scope)) {
                            addErrorWithRanges(member ? member->location : node->location,
                                               member ? member->macroCallSite : node->macroCallSite,
                                               member ? member->macroDefinition : node->macroDefinition,
                                               "Enumerator value is out of range for type 'int'",
                                               member ? member->valueNode.value : NULL);
                        }
                    } else {
                        enumVal = 0;
                        hasValue = true;
                    }
                    if (hasValue) {
                        currentValue = enumVal;
                        haveCurrent = true;
                    }
                    Symbol* enumSym = calloc(1, sizeof(Symbol));
                    if (!enumSym) {
                        continue;
                    }
                    resetFunctionSignature(enumSym);
                    enumSym->name = strdup(member->valueNode.value);
                    enumSym->kind = SYMBOL_ENUM;
                    enumSym->type = enumValueType;
                    enumSym->definition = node;
                    enumSym->hasConstValue = hasValue;
                    enumSym->constValue = enumVal;
                    enumSym->storage = STORAGE_NONE;
                    enumSym->linkage = LINKAGE_NONE;
                    enumSym->next = NULL;
                    if (!addToScope(scope, enumSym)) {
                        addErrorWithRanges(member ? member->location : node->location,
                                           member ? member->macroCallSite : node->macroCallSite,
                                           member ? member->macroDefinition : node->macroDefinition,
                                           "Redefinition of enum constant",
                                           member ? member->valueNode.value : NULL);
                    }
                }
            }

            if (node->type == AST_STRUCT_DEFINITION || node->type == AST_UNION_DEFINITION) {
                char** seenFieldNames = NULL;
                size_t seenFieldCount = 0;
                size_t seenFieldCap = 0;
                for (size_t i = 0; i < node->structDef.fieldCount; ++i) {
                    ASTNode* field = node->structDef.fields ? node->structDef.fields[i] : NULL;
                    if (!field || field->type != AST_VARIABLE_DECLARATION) continue;
                    ParsedType* fType = field->varDecl.declaredTypes
                                            ? &field->varDecl.declaredTypes[0]
                                            : &field->varDecl.declaredType;
                    bool isFlexible = parsedTypeIsFlexibleArray(fType);
                    const char* fieldName =
                        (field->varDecl.varNames && field->varDecl.varNames[0] &&
                         field->varDecl.varNames[0]->type == AST_IDENTIFIER)
                            ? field->varDecl.varNames[0]->valueNode.value
                            : NULL;
                    if (fieldName && fieldName[0]) {
                        bool isDuplicate = false;
                        for (size_t j = 0; j < seenFieldCount; ++j) {
                            if (seenFieldNames[j] && strcmp(seenFieldNames[j], fieldName) == 0) {
                                isDuplicate = true;
                                break;
                            }
                        }
                        if (isDuplicate) {
                            addErrorWithRanges(field->location,
                                               field->macroCallSite,
                                               field->macroDefinition,
                                               "Duplicate field name in aggregate type",
                                               fieldName);
                        } else {
                            if (seenFieldCount == seenFieldCap) {
                                size_t newCap = seenFieldCap ? seenFieldCap * 2 : 8;
                                char** grown = realloc(seenFieldNames, newCap * sizeof(char*));
                                if (grown) {
                                    seenFieldNames = grown;
                                    seenFieldCap = newCap;
                                }
                            }
                            if (seenFieldCount < seenFieldCap) {
                                seenFieldNames[seenFieldCount++] = strdup(fieldName);
                            }
                        }
                    }
                    if (isFlexible) {
                        SourceRange fieldSpelling = varDeclBestSpellingRange(field);
                        SourceRange fieldMacroCall = varDeclBestMacroCallSite(field);
                        SourceRange fieldMacroDef = varDeclBestMacroDefinition(field);
                        if (node->type == AST_UNION_DEFINITION) {
                            addErrorWithRanges(fieldSpelling,
                                               fieldMacroCall,
                                               fieldMacroDef,
                                               "Flexible array members are not allowed in unions",
                                               fieldName);
                        } else {
                            if (i + 1 != node->structDef.fieldCount) {
                                addErrorWithRanges(fieldSpelling,
                                                   fieldMacroCall,
                                                   fieldMacroDef,
                                                   "Flexible array member must be the last field in a struct",
                                                   fieldName);
                            }
                            if (field->varDecl.varCount > 1) {
                                addErrorWithRanges(fieldSpelling,
                                                   fieldMacroCall,
                                                   fieldMacroDef,
                                                   "Flexible array member cannot be declared with multiple declarators",
                                                   fieldName);
                            }
                            node->structDef.hasFlexibleArray = true;
                        }
                    }
                    validateBitField(field, fType, scope);
                    if (parsedTypeIsDirectArray(fType)) {
                        for (size_t d = 0; d < fType->derivationCount; ++d) {
                            TypeDerivation* deriv = parsedTypeGetMutableArrayDerivation(fType, d);
                            if (!deriv) break;
                            if (deriv->as.array.isFlexible) {
                                continue;
                            }
                            if (deriv->as.array.sizeExpr) {
                                size_t len = 0;
                                if (tryEvaluateArrayLength(deriv->as.array.sizeExpr, scope, &len)) {
                                    deriv->as.array.hasConstantSize = true;
                                    deriv->as.array.constantSize = (long long)len;
                                    deriv->as.array.isVLA = false;
                                } else {
                                    deriv->as.array.isVLA = true;
                                    addError(field->line,
                                             0,
                                             "Variable-length array not allowed in struct/union field",
                                             NULL);
                                }
                            }
                        }
                    }
                }
                for (size_t i = 0; i < seenFieldCount; ++i) {
                    free(seenFieldNames[i]);
                }
                free(seenFieldNames);
            }
            break;
        }

        default:
            addError(node ? node->line : 0,
                     0,
                     "Unknown declaration node",
                     "This node is not handled in analyzeDeclaration()");
            break;
    }
    profiler_end(declScope);
}
