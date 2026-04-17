// SPDX-License-Identifier: Apache-2.0

#include "analyze_decls_internal.h"

static int countParameterDeclarators(ASTNode** params, size_t paramCount);
static bool isVoidParameterDecl(ASTNode* param);
static bool isSyntheticUnnamedParameterName(const char* name);
static bool adjustFunctionParameterType(ParsedType* type);
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

static bool adjustFunctionParameterType(ParsedType* type) {
    if (!type) return false;
    bool changed = false;
    if (parsedTypeAdjustArrayParameter(type)) {
        changed = true;
    }
    if (!parsedTypeIsDirectFunction(type)) {
        return changed;
    }
    TypeDerivation* grown = realloc(type->derivations,
                                    (type->derivationCount + 1) * sizeof(TypeDerivation));
    if (!grown) {
        return changed;
    }
    type->derivations = grown;
    memmove(type->derivations + 1,
            type->derivations,
            type->derivationCount * sizeof(TypeDerivation));
    memset(&type->derivations[0], 0, sizeof(TypeDerivation));
    type->derivations[0].kind = TYPE_DERIVATION_POINTER;
    type->derivations[0].as.pointer.isConst = false;
    type->derivations[0].as.pointer.isVolatile = false;
    type->derivations[0].as.pointer.isRestrict = false;
    type->derivationCount++;
    type->pointerDepth += 1;
    type->directlyDeclaresFunction = false;
    changed = true;
    return changed;
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
                             bool isVariadic) {
    if (!sym) return;
    free(sym->signature.params);
    sym->signature.params = NULL;
    sym->signature.paramCount = 0;
    sym->signature.isVariadic = isVariadic;
    sym->signature.hasPrototype = false;
    sym->signature.callConv = CALLCONV_DEFAULT;

    if (!params || paramCount == 0) {
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
                adjustFunctionParameterType(&adjusted);
                sym->signature.params[idx] = adjusted;
                idx++;
            }
        }
    }
    sym->signature.paramCount = idx;
    sym->signature.hasPrototype = true;
}

bool functionSignaturesCompatible(const FunctionSignature* lhs,
                                  const FunctionSignature* rhs,
                                  Scope* scope) {
    if (!lhs || !rhs) return true;
    if (!lhs->hasPrototype || !rhs->hasPrototype) {
        return true;
    }
    if ((lhs->paramCount > 0 && !lhs->params) || (rhs->paramCount > 0 && !rhs->params)) {
        return true;
    }
    if (lhs->paramCount != rhs->paramCount) return false;
    if (lhs->isVariadic != rhs->isVariadic) return false;
    for (size_t i = 0; i < lhs->paramCount; ++i) {
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
