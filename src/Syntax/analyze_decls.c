#include "analyze_decls.h"
#include "symbol_table.h"
#include "syntax_errors.h"
#include "scope.h"
#include "Compiler/compiler_context.h"
#include "Parser/Helpers/designated_init.h"
#include "analyze_expr.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

static const uint64_t FNV_OFFSET = 1469598103934665603ULL;
static const uint64_t FNV_PRIME  = 1099511628211ULL;

static void hash_bytes(uint64_t* hash, const void* data, size_t len) {
    if (!hash || !data) return;
    const unsigned char* bytes = (const unsigned char*)data;
    for (size_t i = 0; i < len; ++i) {
        *hash ^= bytes[i];
        *hash *= FNV_PRIME;
    }
}

static void hash_u64(uint64_t* hash, uint64_t value) {
    hash_bytes(hash, &value, sizeof(value));
}

static void hash_bool(uint64_t* hash, bool value) {
    uint64_t v = value ? 1u : 0u;
    hash_u64(hash, v);
}

static void hash_string(uint64_t* hash, const char* str) {
    if (!hash) return;
    if (!str) {
        hash_u64(hash, 0);
        return;
    }
    while (*str) {
        unsigned char c = (unsigned char)(*str++);
        *hash ^= c;
        *hash *= FNV_PRIME;
    }
    *hash ^= 0xffu;
    *hash *= FNV_PRIME;
}

static void hashParsedTypeFingerprint(uint64_t* hash, const ParsedType* type);
static void hashAstNode(uint64_t* hash, const ASTNode* node);


static void hashParsedTypeFingerprint(uint64_t* hash, const ParsedType* type) {
    if (!hash) return;
    if (!type) {
        hash_u64(hash, 0);
        return;
    }

    hash_u64(hash, (uint64_t)type->kind);
    hash_u64(hash, (uint64_t)type->tag);
    hash_u64(hash, (uint64_t)type->primitiveType);
    hash_bool (hash, type->isFunctionPointer);
    hash_u64  (hash, (uint64_t)type->fpParamCount);
    for (size_t i = 0; i < type->fpParamCount; ++i) {
        hashParsedTypeFingerprint(hash, &type->fpParams[i]);
    }
    hash_string(hash, type->userTypeName);
    hash_bool  (hash, type->isConst);
    hash_bool  (hash, type->isSigned);
    hash_bool  (hash, type->isUnsigned);
    hash_bool  (hash, type->isShort);
    hash_bool  (hash, type->isLong);
    hash_bool  (hash, type->isVolatile);
    hash_bool  (hash, type->isRestrict);
    hash_bool  (hash, type->isInline);
    hash_bool  (hash, type->isStatic);
    hash_bool  (hash, type->isExtern);
    hash_bool  (hash, type->isRegister);
    hash_bool  (hash, type->isAuto);
    hash_u64   (hash, (uint64_t)type->pointerDepth);
    hash_bool  (hash, type->isVLA);
}

static void hashAstNode(uint64_t* hash, const ASTNode* node) {
    hash_u64(hash, node ? (uint64_t)node->type : 0);
    if (!node) return;

    switch (node->type) {
        case AST_NUMBER_LITERAL:
        case AST_CHAR_LITERAL:
        case AST_STRING_LITERAL:
        case AST_IDENTIFIER:
            hash_string(hash, node->valueNode.value);
            break;
        case AST_BINARY_EXPRESSION:
            hash_string(hash, node->expr.op);
            hashAstNode(hash, node->expr.left);
            hashAstNode(hash, node->expr.right);
            break;
        case AST_UNARY_EXPRESSION:
            hash_string(hash, node->expr.op);
            hash_bool(hash, node->expr.isPostfix);
            hashAstNode(hash, node->expr.left);
            break;
        case AST_TERNARY_EXPRESSION:
            hashAstNode(hash, node->ternaryExpr.condition);
            hashAstNode(hash, node->ternaryExpr.trueExpr);
            hashAstNode(hash, node->ternaryExpr.falseExpr);
            break;
        case AST_ASSIGNMENT:
            hash_string(hash, node->assignment.op);
            hashAstNode(hash, node->assignment.target);
            hashAstNode(hash, node->assignment.value);
            break;
        case AST_COMMA_EXPRESSION:
            for (size_t i = 0; i < node->commaExpr.exprCount; ++i) {
                hashAstNode(hash, node->commaExpr.expressions[i]);
            }
            break;
        case AST_CAST_EXPRESSION:
            hashParsedTypeFingerprint(hash, &node->castExpr.castType);
            hashAstNode(hash, node->castExpr.expression);
            break;
        case AST_SIZEOF:
            hashAstNode(hash, node->expr.left);
            break;
        case AST_FUNCTION_CALL:
            hashAstNode(hash, node->functionCall.callee);
            for (size_t i = 0; i < node->functionCall.argumentCount; ++i) {
                hashAstNode(hash, node->functionCall.arguments[i]);
            }
            break;
        case AST_ARRAY_ACCESS:
            hashAstNode(hash, node->arrayAccess.array);
            hashAstNode(hash, node->arrayAccess.index);
            break;
        case AST_POINTER_ACCESS:
        case AST_DOT_ACCESS:
            hashAstNode(hash, node->memberAccess.base);
            hash_string(hash, node->memberAccess.field);
            break;
        case AST_POINTER_DEREFERENCE:
            hashAstNode(hash, node->pointerDeref.pointer);
            break;
        default:
            break;
    }
}

static void hashFieldDeclaration(uint64_t* hash, ASTNode* field) {
    if (!hash || !field) return;
    hash_u64(hash, (uint64_t)field->type);

    if (field->type == AST_VARIABLE_DECLARATION) {
        hashParsedTypeFingerprint(hash, &field->varDecl.declaredType);
        hash_u64(hash, field->varDecl.varCount);
        for (size_t i = 0; i < field->varDecl.varCount; ++i) {
            ASTNode* name = field->varDecl.varNames ? field->varDecl.varNames[i] : NULL;
            if (name && name->type == AST_IDENTIFIER) {
                hash_string(hash, name->valueNode.value);
            } else {
                hash_string(hash, "");
            }
        }
        if (field->varDecl.bitFieldWidth) {
            hash_bool(hash, true);
            hashAstNode(hash, field->varDecl.bitFieldWidth);
        } else {
            hash_bool(hash, false);
        }
    }
}

static uint64_t fingerprintStructLike(const ASTNode* node) {
    uint64_t hash = FNV_OFFSET;
    if (!node) return hash;

    hash_u64(&hash, node->type == AST_UNION_DEFINITION ? 2u : 1u);
    for (size_t i = 0; i < node->structDef.fieldCount; ++i) {
        hash_u64(&hash, i + 1);
        hashFieldDeclaration(&hash, node->structDef.fields[i]);
    }
    hash_u64(&hash, node->structDef.fieldCount);
    return hash;
}

static uint64_t fingerprintEnumDefinition(const ASTNode* node) {
    uint64_t hash = FNV_OFFSET;
    if (!node) return hash;
    hash_u64(&hash, node->enumDef.memberCount);
    for (size_t i = 0; i < node->enumDef.memberCount; ++i) {
        ASTNode* member = node->enumDef.members ? node->enumDef.members[i] : NULL;
        if (member && member->type == AST_IDENTIFIER) {
            hash_string(&hash, member->valueNode.value);
        } else {
            hash_string(&hash, "");
        }
        ASTNode* value = node->enumDef.values ? node->enumDef.values[i] : NULL;
        if (value) {
            hash_bool(&hash, true);
            hashAstNode(&hash, value);
        } else {
            hash_bool(&hash, false);
        }
    }
    return hash;
}

static ParsedType makeEnumValueParsedType(void) {
    ParsedType t;
    memset(&t, 0, sizeof(t));
    t.kind = TYPE_PRIMITIVE;
    t.primitiveType = TOKEN_INT;
    t.isConst = true;
    return t;
}

static void resetFunctionSignature(Symbol* sym) {
    if (!sym) return;
    sym->signature.params = NULL;
    sym->signature.paramCount = 0;
    sym->signature.isVariadic = false;
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

static void assignFunctionSignature(Symbol* sym, ASTNode** params, size_t paramCount, bool isVariadic) {
    if (!sym) return;
    free(sym->signature.params);
    sym->signature.params = NULL;
    sym->signature.paramCount = 0;
    sym->signature.isVariadic = isVariadic;

    if (!params || paramCount == 0) {
        return;
    }

    int totalDecls = countParameterDeclarators(params, paramCount);
    if (totalDecls <= 0) {
        return;
    }

    if (totalDecls == 1 && isVoidParameterDecl(params[0]) && !isVariadic) {
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
                sym->signature.params[idx] = *srcType;
                idx++;
            }
        }
    }
    sym->signature.paramCount = idx;
}

static const char* safeIdentifierName(ASTNode* node) {
    if (node && node->type == AST_IDENTIFIER && node->valueNode.value) {
        return node->valueNode.value;
    }
    return "<unnamed>";
}

static bool parseIntegerLiteral(const char* text, long long* out) {
    if (!text || !out) return false;
    char* end = NULL;
    long long value = strtoll(text, &end, 0);
    if (text == end) return false;
    *out = value;
    return true;
}

static bool evaluateConstantIntegral(ASTNode* expr, long long* out) {
    if (!expr || !out) return false;
    switch (expr->type) {
        case AST_NUMBER_LITERAL:
            return parseIntegerLiteral(expr->valueNode.value, out);
        case AST_CHAR_LITERAL:
            if (expr->valueNode.value && expr->valueNode.value[0]) {
                *out = (unsigned char)expr->valueNode.value[0];
                return true;
            }
            return false;
        case AST_UNARY_EXPRESSION:
            if (!expr->expr.left || !expr->expr.op) return false;
            if (evaluateConstantIntegral(expr->expr.left, out)) {
                if (strcmp(expr->expr.op, "-") == 0) {
                    *out = -*out;
                    return true;
                }
                if (strcmp(expr->expr.op, "+") == 0) {
                    return true;
                }
            }
            return false;
        default:
            return false;
    }
}

static bool tryEvaluateArrayLength(ASTNode* sizeExpr, size_t* outLen) {
    if (!sizeExpr || !outLen) return false;
    long long value = 0;
    if (!evaluateConstantIntegral(sizeExpr, &value) || value < 0) {
        return false;
    }
    *outLen = (size_t)value;
    return true;
}

static bool scopeIsFileScope(Scope* scope) {
    return scope && scope->parent == NULL;
}

static void analyzeDesignatedInitializer(DesignatedInit* init, Scope* scope) {
    if (!init || !scope) return;
    if (init->indexExpr) {
        (void)analyzeExpression(init->indexExpr, scope);
    }
    if (init->expression) {
        (void)analyzeExpression(init->expression, scope);
    }
}

static void validateVariableInitializer(ParsedType* type, DesignatedInit* init, ASTNode* nameNode, Scope* scope, bool staticStorage);
static void validateVariableArrayInitializer(ParsedType* type, DesignatedInit* init, ASTNode* nameNode, Scope* scope);
static void validateArrayInitializerEntries(ParsedType* type,
                                            const char* arrayName,
                                            DesignatedInit** values,
                                            size_t valueCount,
                                            Scope* scope,
                                            ASTNode* contextNode,
                                            long long* outInferredLength);

void analyzeDeclaration(ASTNode* node, Scope* scope) {
    switch (node->type) {
        case AST_VARIABLE_DECLARATION: {
            ParsedType* declaredTypes = node->varDecl.declaredTypes;
            for (size_t i = 0; i < node->varDecl.varCount; i++) {
                ASTNode* ident = node->varDecl.varNames[i];
                ParsedType* varType = declaredTypes ? &declaredTypes[i]
                                                     : &node->varDecl.declaredType;
                Symbol* sym = malloc(sizeof(Symbol));
                sym->name = strdup(ident->valueNode.value);
                sym->kind = SYMBOL_VARIABLE;
                sym->type = *varType;
                sym->definition = node;
                sym->next = NULL;
                resetFunctionSignature(sym);

                if (!addToScope(scope, sym)) {
                    addErrorWithRanges(ident ? ident->location : node->location,
                                       ident ? ident->macroCallSite : node->macroCallSite,
                                       ident ? ident->macroDefinition : node->macroDefinition,
                                       "Redefinition of variable",
                                       ident ? ident->valueNode.value : NULL);
                }

                bool staticStorage = scopeIsFileScope(scope) ||
                                     (varType && (varType->isStatic || varType->isExtern));
                bool isArrayVar = varType && parsedTypeIsDirectArray(varType);
                if (isArrayVar) {
                    for (size_t d = 0; d < varType->derivationCount; ++d) {
                        TypeDerivation* deriv = parsedTypeGetMutableArrayDerivation(varType, d);
                        if (!deriv) break;
                        if (deriv->as.array.sizeExpr) {
                            size_t len = 0;
                            if (tryEvaluateArrayLength(deriv->as.array.sizeExpr, &len)) {
                                deriv->as.array.hasConstantSize = true;
                                deriv->as.array.constantSize = (long long)len;
                                deriv->as.array.isVLA = false;
                            } else {
                                deriv->as.array.isVLA = true;
                            }
                        }
                    }
                    if (parsedTypeHasVLA(varType) && staticStorage) {
                        char buffer[256];
                        snprintf(buffer,
                                 sizeof(buffer),
                                 "Variable-length array '%s' is not allowed at static storage duration",
                                 ident && ident->valueNode.value ? ident->valueNode.value : "<unnamed>");
                        addError(ident ? ident->line : node->line, 0, buffer, NULL);
                    }
                }

                if (i < node->varDecl.varCount && node->varDecl.initializers) {
                    DesignatedInit* init = node->varDecl.initializers[i];
                    analyzeDesignatedInitializer(init, scope);
                    if (parsedTypeIsDirectArray(varType)) {
                        validateVariableArrayInitializer(varType, init, ident, scope);
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
            if (node->type == AST_FUNCTION_DEFINITION) {
                fprintf(stderr, "analyze func %s paramCount=%zu\n",
                        funcName && funcName->type == AST_IDENTIFIER ? funcName->valueNode.value : "<anon>",
                        node->functionDef.paramCount);
            }

            Symbol* existing = lookupSymbol(&scope->table, funcName->valueNode.value);
            if (existing && existing->kind == SYMBOL_FUNCTION) {
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

                if (node->type == AST_FUNCTION_DEFINITION) {
                    existing->type = node->functionDef.returnType;
                    existing->definition = node;
                    resetFunctionSignature(existing);
                    assignFunctionSignature(existing,
                                            node->functionDef.parameters,
                                            node->functionDef.paramCount,
                                            node->functionDef.isVariadic);
                }
                break;
            }

            Symbol* sym = malloc(sizeof(Symbol));
            sym->name = strdup(funcName->valueNode.value);
            sym->kind = SYMBOL_FUNCTION;
            sym->type = node->type == AST_FUNCTION_DEFINITION
                ? node->functionDef.returnType
                : node->functionDecl.returnType;
            sym->definition = node;
            sym->next = NULL;
            resetFunctionSignature(sym);
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
            Symbol* sym = malloc(sizeof(Symbol));
            sym->name = strdup(node->typedefStmt.alias->valueNode.value);
            sym->kind = SYMBOL_TYPEDEF;
            sym->type = node->typedefStmt.baseType;
            sym->definition = node;
            sym->next = NULL;
            resetFunctionSignature(sym);

            if (!addToScope(scope, sym)) {
                addErrorWithRanges(node ? node->location : (SourceRange){0},
                                   node ? node->macroCallSite : (SourceRange){0},
                                   node ? node->macroDefinition : (SourceRange){0},
                                   "Redefinition of typedef",
                                   sym->name);
            }
            if (scope->ctx) {
                cc_add_typedef(scope->ctx, node->typedefStmt.alias->valueNode.value);
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

            CCTagKind tagKind = CC_TAG_STRUCT;
            uint64_t fingerprint = 0;
            const char* kindLabel = "struct";

            if (node->type == AST_UNION_DEFINITION) {
                tagKind = CC_TAG_UNION;
                kindLabel = "union";
                fingerprint = fingerprintStructLike(node);
            } else if (node->type == AST_ENUM_DEFINITION) {
                tagKind = CC_TAG_ENUM;
                kindLabel = "enum";
                fingerprint = fingerprintEnumDefinition(node);
            } else {
                fingerprint = fingerprintStructLike(node);
            }

            CCTagDefineResult result = cc_define_tag(scope->ctx, tagKind, nameNode->valueNode.value, fingerprint);
            if (result == CC_TAGDEF_CONFLICT) {
                char buffer[128];
                snprintf(buffer, sizeof(buffer), "Conflicting definition of %s '%s'", kindLabel, nameNode->valueNode.value);
                addError(nameNode ? nameNode->line : node->line, 0, buffer, NULL);
            }

            if (node->type == AST_ENUM_DEFINITION) {
                ParsedType enumValueType = makeEnumValueParsedType();
                for (size_t i = 0; i < node->enumDef.memberCount; ++i) {
                    ASTNode* member = node->enumDef.members ? node->enumDef.members[i] : NULL;
                    if (!member || member->type != AST_IDENTIFIER) {
                        continue;
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
            break;
        }

        default:
            addError(node ? node->line : 0, 0, "Unknown declaration node", "This node is not handled in analyzeDeclaration()");
            break;
    }
}
static bool typeInfoIsStructLike(const TypeInfo* info) {
    return info && (info->category == TYPEINFO_STRUCT || info->category == TYPEINFO_UNION);
}

static void reportScalarInitializerIssue(ASTNode* context, const char* name, const char* detail) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s for '%s'", detail ? detail : "Invalid scalar initializer", name ? name : "<unnamed>");
    addError(context ? context->line : 0, 0, buffer, NULL);
}

static void validateScalarCompoundLiteral(ASTNode* compound, ASTNode* context, const char* name) {
    if (!compound || compound->type != AST_COMPOUND_LITERAL) return;
    size_t count = compound->compoundLiteral.entryCount;
    if (count == 0) {
        reportScalarInitializerIssue(context, name, "Empty scalar initializer");
    } else if (count > 1) {
        reportScalarInitializerIssue(context, name, "Excess elements in scalar initializer");
    }
}

static void validateVariableInitializer(ParsedType* type,
                                        DesignatedInit* init,
                                        ASTNode* nameNode,
                                        Scope* scope,
                                        bool staticStorage) {
    if (!type || !scope) return;

    if (parsedTypeIsDirectArray(type)) {
        validateVariableArrayInitializer(type, init, nameNode, scope);
        return;
    }

    if (!init) return;
    const char* name = safeIdentifierName(nameNode);
    TypeInfo info = typeInfoFromParsedType(type, scope);
    if (typeInfoIsStructLike(&info)) {
        if (!init->expression || init->expression->type != AST_COMPOUND_LITERAL) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "Initializer for struct variable '%s' must be brace-enclosed", name);
            addError(nameNode ? nameNode->line : 0, 0, buffer, NULL);
        } else if (init->expression->compoundLiteral.entryCount == 0) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "Empty initializer for struct variable '%s'", name);
            addError(nameNode ? nameNode->line : 0, 0, buffer, NULL);
        }
        return;
    }

    if (init->expression && init->expression->type == AST_COMPOUND_LITERAL) {
        validateScalarCompoundLiteral(init->expression, nameNode, name);
    }

    if (staticStorage && init->expression && init->expression->type == AST_COMPOUND_LITERAL) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "Compound literal at static storage must be constant");
        addError(nameNode ? nameNode->line : 0, 0, buffer, NULL);
    }
}

static bool typeInfoIsCharLike(const TypeInfo* info) {
    return info && info->category == TYPEINFO_INTEGER && info->bitWidth == 8;
}

static bool isStringLiteralInitializer(DesignatedInit* init) {
    return init && !init->fieldName && !init->indexExpr &&
           init->expression && init->expression->type == AST_STRING_LITERAL;
}

static void validateArrayInitializerEntries(ParsedType* type,
                                            const char* arrayName,
                                            DesignatedInit** values,
                                            size_t valueCount,
                                            Scope* scope,
                                            ASTNode* contextNode,
                                            long long* outInferredLength) {
    if (!type || !scope || !values || valueCount == 0) {
        if (outInferredLength) *outInferredLength = -1;
        return;
    }

    size_t declaredLen = 0;
    bool hasDeclaredLen = false;
    TypeDerivation* topArray = parsedTypeGetMutableArrayDerivation(type, 0);
    ASTNode* sizeExpr = NULL;
    if (topArray) {
        if (topArray->as.array.hasConstantSize && !topArray->as.array.isVLA) {
            hasDeclaredLen = true;
            declaredLen = (size_t)(topArray->as.array.constantSize >= 0 ? topArray->as.array.constantSize : 0);
        } else {
            sizeExpr = topArray->as.array.sizeExpr;
            if (sizeExpr && tryEvaluateArrayLength(sizeExpr, &declaredLen)) {
                hasDeclaredLen = true;
                topArray->as.array.hasConstantSize = true;
                topArray->as.array.constantSize = (long long)declaredLen;
                topArray->as.array.isVLA = false;
            }
        }
    }
    if (outInferredLength) {
        *outInferredLength = -1;
    }

    TypeInfo elementInfo = typeInfoFromParsedType(type, scope);
    if (parsedTypeIsDirectArray(type)) {
        ParsedType elementType = parsedTypeArrayElementType(type);
        elementInfo = typeInfoFromParsedType(&elementType, scope);
        parsedTypeFree(&elementType);
    }
    bool treatAsChar = typeInfoIsCharLike(&elementInfo);

    if (valueCount == 1 && isStringLiteralInitializer(values[0]) && treatAsChar) {
        size_t literalLen = values[0]->expression && values[0]->expression->valueNode.value
            ? strlen(values[0]->expression->valueNode.value)
            : 0;
        size_t needed = literalLen + 1;
        if (hasDeclaredLen && needed > declaredLen) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer),
                     "String literal for array '%s' is too long (needs %zu, size %zu)",
                     arrayName, needed, declaredLen);
            addError(contextNode ? contextNode->line : 0, 0, buffer, NULL);
        } else if (!hasDeclaredLen && outInferredLength) {
            *outInferredLength = (long long)needed;
        }
        return;
    }

    size_t sequentialCount = 0;
    size_t highestIndex = 0;
    bool usedDesignators = false;
    bool sawAny = false;
    for (size_t i = 0; i < valueCount; ++i) {
        DesignatedInit* init = values[i];
        if (!init) continue;
        if (init->indexExpr) {
            usedDesignators = true;
            long long indexValue = 0;
            if (!evaluateConstantIntegral(init->indexExpr, &indexValue)) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "Array '%s' designator index must be a constant expression", arrayName);
                addError(init->indexExpr->line, 0, buffer, NULL);
            } else if (indexValue < 0) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "Array '%s' designator index %lld is negative", arrayName, indexValue);
                addError(init->indexExpr->line, 0, buffer, NULL);
            } else if (hasDeclaredLen && (size_t)indexValue >= declaredLen) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer),
                         "Array '%s' designator index %lld is out of bounds (size %zu)",
                         arrayName, indexValue, declaredLen);
                addError(init->indexExpr->line, 0, buffer, NULL);
            }
            if (indexValue >= 0) {
                size_t candidate = (size_t)indexValue + 1;
                if (candidate > highestIndex) {
                    highestIndex = candidate;
                }
                sawAny = true;
            }
        } else {
            if (hasDeclaredLen && sequentialCount >= declaredLen) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer),
                         "Too many initializers for array '%s' (size %zu)",
                         arrayName, declaredLen);
                addError(contextNode ? contextNode->line : 0, 0, buffer, NULL);
            }
            sequentialCount++;
            if (sequentialCount > highestIndex) {
                highestIndex = sequentialCount;
            }
            sawAny = true;
        }

        if (init->expression && init->expression->type == AST_COMPOUND_LITERAL && !typeInfoIsStructLike(&elementInfo)) {
            validateScalarCompoundLiteral(init->expression, contextNode, arrayName);
        }
    }

    if (hasDeclaredLen && !usedDesignators && sequentialCount < declaredLen) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "Not enough initializers for array '%s' (have %zu, expected %zu)",
                 arrayName, sequentialCount, declaredLen);
        addError(contextNode ? contextNode->line : 0, 0, buffer, NULL);
    }

    if (!hasDeclaredLen && outInferredLength && sawAny) {
        *outInferredLength = (long long)highestIndex;
    }
}

static void validateVariableArrayInitializer(ParsedType* type,
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
        validateArrayInitializerEntries(type, arrayName, single, 1, scope, nameNode, &inferredLen);
    } else {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
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
