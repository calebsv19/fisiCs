// SPDX-License-Identifier: Apache-2.0

#include "analyze_decls_internal.h"
#include "Utils/profiler.h"

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

                if (!validateStorageUsage(varType,
                                          scopeIsFileScope(scope),
                                          false,
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
                evaluateArrayDerivations(varType, scope);
                profiler_record_value("semantic_count_type_info_site_decl", 1);
                TypeInfo varInfo = typeInfoFromParsedType(varType, scope);
                StorageClass storage = deduceStorageClass(varType);
                SymbolLinkage linkage = deduceLinkage(varType, scopeIsFileScope(scope));
                bool hasInitializer = node->varDecl.initializers &&
                                      i < node->varDecl.varCount &&
                                      node->varDecl.initializers[i] != NULL;
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
                        addErrorWithRanges(ident ? ident->location : node->location,
                                           ident ? ident->macroCallSite : node->macroCallSite,
                                           ident ? ident->macroDefinition : node->macroDefinition,
                                           "Conflicting types for variable",
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
                        existing->hasDefinition = true;
                        existing->isTentative = false;
                        existing->definition = node;
                    }
                    primeSymbolTypeInfoCache(existing, scope);
                    boundSym = existing;
                } else {
                    if (!fileScope && storage == STORAGE_EXTERN) {
                        Symbol* linked = resolveInScopeChain(scope, ident->valueNode.value);
                        if (linked &&
                            linked->kind == SYMBOL_VARIABLE &&
                            linked->linkage != LINKAGE_NONE &&
                            !parsedTypesStructurallyCompatibleInScope(&linked->type, varType, scope)) {
                            addErrorWithRanges(ident ? ident->location : node->location,
                                               ident ? ident->macroCallSite : node->macroCallSite,
                                               ident ? ident->macroDefinition : node->macroDefinition,
                                               "Conflicting types for variable",
                                               ident ? ident->valueNode.value : NULL);
                            continue;
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
                    sym->next = NULL;
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
                const ParsedType* arrayType =
                    (resolvedVar && parsedTypeIsDirectArray(resolvedVar)) ? resolvedVar : varType;
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
            ASTNode** params = node->type == AST_FUNCTION_DEFINITION
                                   ? node->functionDef.parameters
                                   : node->functionDecl.parameters;
            size_t paramCount = node->type == AST_FUNCTION_DEFINITION
                                    ? node->functionDef.paramCount
                                    : node->functionDecl.paramCount;
            bool isVariadic = node->type == AST_FUNCTION_DEFINITION
                                  ? node->functionDef.isVariadic
                                  : node->functionDecl.isVariadic;
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
                    existing->type = node->type == AST_FUNCTION_DEFINITION
                                         ? node->functionDef.returnType
                                         : node->functionDecl.returnType;
                    invalidateSymbolTypeInfoCache(existing);
                    resetFunctionSignature(existing);
                    if (node->type == AST_FUNCTION_DEFINITION) {
                        assignFunctionSignature(existing,
                                                node->functionDef.parameters,
                                                node->functionDef.paramCount,
                                                node->functionDef.isVariadic);
                    } else {
                        assignFunctionSignature(existing,
                                                node->functionDecl.parameters,
                                                node->functionDecl.paramCount,
                                                node->functionDecl.isVariadic);
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

                if (!parsedTypesStructurallyCompatibleInScope(
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
                                                node->functionDef.isVariadic);
                    } else {
                        assignFunctionSignature(&tmp,
                                                node->functionDecl.parameters,
                                                node->functionDecl.paramCount,
                                                node->functionDecl.isVariadic);
                    }
                    if (!functionSignaturesCompatible(&existing->signature, &tmp.signature, scope)) {
                        addErrorWithRanges(funcName ? funcName->location : node->location,
                                           funcName ? funcName->macroCallSite : node->macroCallSite,
                                           funcName ? funcName->macroDefinition : node->macroDefinition,
                                           "Conflicting types for function",
                                           funcName ? funcName->valueNode.value : NULL);
                        free(tmp.signature.params);
                        break;
                    }
                    free(tmp.signature.params);
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
                    existing->type = node->functionDef.returnType;
                    invalidateSymbolTypeInfoCache(existing);
                    existing->definition = node;
                    resetFunctionSignature(existing);
                    assignFunctionSignature(existing,
                                            node->functionDef.parameters,
                                            node->functionDef.paramCount,
                                            node->functionDef.isVariadic);
                    existing->hasDefinition = true;
                }
                primeSymbolTypeInfoCache(existing, scope);
                applyInteropAttributes(existing, node, scope, true);
                break;
            }

            Symbol* sym = calloc(1, sizeof(Symbol));
            if (!sym) break;
            sym->name = strdup(funcName->valueNode.value);
            sym->kind = SYMBOL_FUNCTION;
            sym->type = node->type == AST_FUNCTION_DEFINITION
                            ? node->functionDef.returnType
                            : node->functionDecl.returnType;
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
                                        node->functionDef.isVariadic);
            } else {
                assignFunctionSignature(sym,
                                        node->functionDecl.parameters,
                                        node->functionDecl.paramCount,
                                        node->functionDecl.isVariadic);
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
                    addError(nameNode ? nameNode->line : node->line, 0, buffer, NULL);
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
                        addError(nameNode ? nameNode->line : node->line, 0, buffer, NULL);
                    }
                }
            } else {
                if (!fileScope && tagKind != CC_TAG_ENUM) {
                    if (!cc_has_tag(scope->ctx, tagKind, nameNode->valueNode.value)) {
                        (void)cc_add_tag(scope->ctx, tagKind, nameNode->valueNode.value);
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
                        ConstEvalResult val = constEval(valueExpr, scope, true);
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
