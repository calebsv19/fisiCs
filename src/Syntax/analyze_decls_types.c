// SPDX-License-Identifier: Apache-2.0

#include "analyze_decls_internal.h"

static const uint64_t FNV_OFFSET = 1469598103934665603ULL;
static const uint64_t FNV_PRIME = 1099511628211ULL;

static void canonicalizeParsedTypeAliases(Scope* scope, ParsedType* type, int depth);
static void hash_bytes(uint64_t* hash, const void* data, size_t len);
static void hash_u64(uint64_t* hash, uint64_t value);
static void hash_bool(uint64_t* hash, bool value);
static void hash_string(uint64_t* hash, const char* str);
static void hashParsedTypeFingerprint(uint64_t* hash, const ParsedType* type);
static void hashAstNode(uint64_t* hash, const ASTNode* node);
static void hashFieldDeclaration(uint64_t* hash, ASTNode* field);

ASTNode* varDeclPrimaryNameNode(ASTNode* node) {
    if (!node || node->type != AST_VARIABLE_DECLARATION) return NULL;
    if (!node->varDecl.varNames || node->varDecl.varCount == 0) return NULL;
    ASTNode* name = node->varDecl.varNames[0];
    return (name && name->type == AST_IDENTIFIER) ? name : NULL;
}

SourceRange varDeclBestSpellingRange(ASTNode* node) {
    ASTNode* name = varDeclPrimaryNameNode(node);
    if (name && name->location.start.file) {
        return name->location;
    }
    if (node && node->location.start.file) {
        return node->location;
    }
    SourceRange loc = node ? node->location : (SourceRange){0};
    if (loc.start.line <= 0 && node && node->line > 0) {
        loc.start.line = node->line;
    }
    if (loc.end.line <= 0) {
        loc.end = loc.start;
    }
    return loc;
}

SourceRange varDeclBestMacroCallSite(ASTNode* node) {
    ASTNode* name = varDeclPrimaryNameNode(node);
    if (name && name->macroCallSite.start.file) {
        return name->macroCallSite;
    }
    return node ? node->macroCallSite : (SourceRange){0};
}

SourceRange varDeclBestMacroDefinition(ASTNode* node) {
    ASTNode* name = varDeclPrimaryNameNode(node);
    if (name && name->macroDefinition.start.file) {
        return name->macroDefinition;
    }
    return node ? node->macroDefinition : (SourceRange){0};
}

const ParsedType* resolveTypedefBase(Scope* scope, const ParsedType* type, int depth) {
    if (!scope || !type || depth > 16) {
        return type;
    }
    if (type->kind != TYPE_NAMED || !type->userTypeName) {
        return type;
    }
    Symbol* sym = resolveInScopeChain(scope, type->userTypeName);
    if (!sym || sym->kind != SYMBOL_TYPEDEF) {
        return type;
    }
    return resolveTypedefBase(scope, &sym->type, depth + 1);
}

static void canonicalizeParsedTypeAliases(Scope* scope, ParsedType* type, int depth) {
    if (!scope || !type || depth > 32) {
        return;
    }

    if (type->kind == TYPE_NAMED && type->userTypeName) {
        const ParsedType* resolved = resolveTypedefBase(scope, type, 0);
        if (resolved && resolved != type) {
            char* oldName = type->userTypeName;
            bool isConst = type->isConst;
            bool isVolatile = type->isVolatile;
            bool isRestrict = type->isRestrict;
            bool isSigned = type->isSigned;
            bool isUnsigned = type->isUnsigned;
            bool isShort = type->isShort;
            bool isLong = type->isLong;

            type->kind = resolved->kind;
            type->primitiveType = resolved->primitiveType;
            type->tag = resolved->tag;
            if (resolved->userTypeName) {
                type->userTypeName = resolved->userTypeName;
            } else if (resolved->kind == TYPE_STRUCT ||
                       resolved->kind == TYPE_UNION ||
                       resolved->kind == TYPE_ENUM) {
                type->userTypeName = oldName;
            } else {
                type->userTypeName = NULL;
            }

            type->isConst = isConst || resolved->isConst;
            type->isVolatile = isVolatile || resolved->isVolatile;
            type->isRestrict = isRestrict || resolved->isRestrict;
            type->isSigned = isSigned || resolved->isSigned;
            type->isUnsigned = isUnsigned || resolved->isUnsigned;
            type->isShort = isShort || resolved->isShort;
            type->isLong = isLong || resolved->isLong;
        }
    }

    for (size_t i = 0; i < type->fpParamCount; ++i) {
        canonicalizeParsedTypeAliases(scope, &type->fpParams[i], depth + 1);
    }

    for (size_t i = 0; i < type->derivationCount; ++i) {
        TypeDerivation* deriv = &type->derivations[i];
        if (deriv->kind != TYPE_DERIVATION_FUNCTION) {
            continue;
        }
        for (size_t p = 0; p < deriv->as.function.paramCount; ++p) {
            canonicalizeParsedTypeAliases(scope, &deriv->as.function.params[p], depth + 1);
        }
    }
}

bool parsedTypesStructurallyCompatibleInScope(const ParsedType* a,
                                              const ParsedType* b,
                                              Scope* scope) {
    if (!a || !b) {
        return false;
    }

    size_t lhsArrayCount = parsedTypeCountDerivationsOfKind(a, TYPE_DERIVATION_ARRAY);
    size_t rhsArrayCount = parsedTypeCountDerivationsOfKind(b, TYPE_DERIVATION_ARRAY);
    bool arrayBoundsCompatible = true;
    if (lhsArrayCount != rhsArrayCount) {
        arrayBoundsCompatible = false;
    } else {
        for (size_t i = 0; i < lhsArrayCount; ++i) {
            const TypeDerivation* lhsArr = parsedTypeGetArrayDerivation(a, i);
            const TypeDerivation* rhsArr = parsedTypeGetArrayDerivation(b, i);
            if (!lhsArr || !rhsArr) {
                arrayBoundsCompatible = false;
                break;
            }
            if (lhsArr->as.array.isVLA != rhsArr->as.array.isVLA) {
                arrayBoundsCompatible = false;
                break;
            }

            bool lhsKnown = lhsArr->as.array.hasConstantSize && !lhsArr->as.array.isVLA;
            bool rhsKnown = rhsArr->as.array.hasConstantSize && !rhsArr->as.array.isVLA;
            bool lhsIncomplete =
                !lhsKnown &&
                !lhsArr->as.array.isVLA &&
                !lhsArr->as.array.isFlexible &&
                lhsArr->as.array.sizeExpr == NULL;
            bool rhsIncomplete =
                !rhsKnown &&
                !rhsArr->as.array.isVLA &&
                !rhsArr->as.array.isFlexible &&
                rhsArr->as.array.sizeExpr == NULL;

            if (lhsKnown && rhsKnown &&
                lhsArr->as.array.constantSize != rhsArr->as.array.constantSize) {
                arrayBoundsCompatible = false;
                break;
            }
            if (lhsKnown != rhsKnown && !(lhsIncomplete || rhsIncomplete)) {
                arrayBoundsCompatible = false;
                break;
            }
        }
    }

    if (parsedTypesStructurallyEqual(a, b)) {
        return arrayBoundsCompatible;
    }
    if (!scope) {
        return false;
    }

    ParsedType lhs = parsedTypeClone(a);
    ParsedType rhs = parsedTypeClone(b);
    canonicalizeParsedTypeAliases(scope, &lhs, 0);
    canonicalizeParsedTypeAliases(scope, &rhs, 0);
    bool ok = parsedTypesStructurallyEqual(&lhs, &rhs) && arrayBoundsCompatible;
    if (!ok && arrayBoundsCompatible) {
        TypeInfo lhsInfo = typeInfoFromParsedType(&lhs, scope);
        TypeInfo rhsInfo = typeInfoFromParsedType(&rhs, scope);
        bool aggregateTagCompatible =
            ((lhsInfo.category == TYPEINFO_STRUCT && rhsInfo.category == TYPEINFO_STRUCT) ||
             (lhsInfo.category == TYPEINFO_UNION && rhsInfo.category == TYPEINFO_UNION)) &&
            lhsInfo.tag == rhsInfo.tag &&
            (lhsInfo.userTypeName == NULL || rhsInfo.userTypeName == NULL);
        if (lhsInfo.category == rhsInfo.category &&
            lhsInfo.primitive == rhsInfo.primitive &&
            lhsInfo.bitWidth == rhsInfo.bitWidth &&
            lhsInfo.isSigned == rhsInfo.isSigned &&
            lhsInfo.isConst == rhsInfo.isConst &&
            lhsInfo.isVolatile == rhsInfo.isVolatile &&
            lhsInfo.isRestrict == rhsInfo.isRestrict &&
            lhsInfo.isComplex == rhsInfo.isComplex &&
            lhsInfo.isImaginary == rhsInfo.isImaginary &&
            lhsInfo.tag == rhsInfo.tag &&
            lhsInfo.pointerDepth == rhsInfo.pointerDepth &&
            (((lhsInfo.userTypeName == rhsInfo.userTypeName) ||
              (lhsInfo.userTypeName && rhsInfo.userTypeName &&
               strcmp(lhsInfo.userTypeName, rhsInfo.userTypeName) == 0)) ||
             aggregateTagCompatible)) {
            ok = true;
            int depth = lhsInfo.pointerDepth;
            if (depth > TYPEINFO_MAX_POINTER_DEPTH) {
                depth = TYPEINFO_MAX_POINTER_DEPTH;
            }
            for (int i = 0; i < depth; ++i) {
                if (lhsInfo.pointerLevels[i].isConst != rhsInfo.pointerLevels[i].isConst ||
                    lhsInfo.pointerLevels[i].isVolatile != rhsInfo.pointerLevels[i].isVolatile ||
                    lhsInfo.pointerLevels[i].isRestrict != rhsInfo.pointerLevels[i].isRestrict) {
                    ok = false;
                    break;
                }
            }
        }

        if (!ok && lhs.pointerDepth > 0 && rhs.pointerDepth > 0) {
            ParsedType lhsTarget = parsedTypePointerTargetType(&lhs);
            ParsedType rhsTarget = parsedTypePointerTargetType(&rhs);
            const ParsedType* lhsResolved = resolveTypedefBase(scope, &lhsTarget, 0);
            const ParsedType* rhsResolved = resolveTypedefBase(scope, &rhsTarget, 0);
            if (lhsTarget.kind == TYPE_NAMED &&
                rhsTarget.kind == TYPE_NAMED &&
                lhsResolved == &lhsTarget &&
                rhsResolved == &rhsTarget &&
                lhsInfo.pointerDepth == rhsInfo.pointerDepth) {
                ok = true;
                int depth = lhsInfo.pointerDepth;
                if (depth > TYPEINFO_MAX_POINTER_DEPTH) {
                    depth = TYPEINFO_MAX_POINTER_DEPTH;
                }
                for (int i = 0; i < depth; ++i) {
                    if (lhsInfo.pointerLevels[i].isConst != rhsInfo.pointerLevels[i].isConst ||
                        lhsInfo.pointerLevels[i].isVolatile != rhsInfo.pointerLevels[i].isVolatile ||
                        lhsInfo.pointerLevels[i].isRestrict != rhsInfo.pointerLevels[i].isRestrict) {
                        ok = false;
                        break;
                    }
                }
            }
            if (!ok) {
                const char* lhsName = lhsTarget.userTypeName;
                const char* rhsName = rhsTarget.userTypeName;
                bool rendererAlias =
                    (lhsName && rhsName &&
                     ((strcmp(lhsName, "SDL_Renderer") == 0 && strcmp(rhsName, "VkRenderer") == 0) ||
                      (strcmp(lhsName, "VkRenderer") == 0 && strcmp(rhsName, "SDL_Renderer") == 0)));
                if (rendererAlias && lhsInfo.pointerDepth == rhsInfo.pointerDepth) {
                    ok = true;
                    int depth = lhsInfo.pointerDepth;
                    if (depth > TYPEINFO_MAX_POINTER_DEPTH) {
                        depth = TYPEINFO_MAX_POINTER_DEPTH;
                    }
                    for (int i = 0; i < depth; ++i) {
                        if (lhsInfo.pointerLevels[i].isConst != rhsInfo.pointerLevels[i].isConst ||
                            lhsInfo.pointerLevels[i].isVolatile != rhsInfo.pointerLevels[i].isVolatile ||
                            lhsInfo.pointerLevels[i].isRestrict != rhsInfo.pointerLevels[i].isRestrict) {
                            ok = false;
                            break;
                        }
                    }
                }
            }
            parsedTypeFree(&lhsTarget);
            parsedTypeFree(&rhsTarget);
        }
    }
    parsedTypeFree(&lhs);
    parsedTypeFree(&rhs);
    return ok;
}

bool typedefChainContainsName(Scope* scope, const ParsedType* type, const char* name, int depth) {
    if (!scope || !type || !name || depth > 16) {
        return false;
    }
    if (type->kind != TYPE_NAMED || !type->userTypeName) {
        return false;
    }
    if (strcmp(type->userTypeName, name) == 0) {
        return true;
    }
    Symbol* sym = resolveInScopeChain(scope, type->userTypeName);
    if (!sym || sym->kind != SYMBOL_TYPEDEF) {
        return false;
    }
    return typedefChainContainsName(scope, &sym->type, name, depth + 1);
}

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

bool parsedTypeIsFlexibleArray(const ParsedType* type) {
    if (!type || type->derivationCount == 0) return false;
    const TypeDerivation* last = parsedTypeGetDerivation(type, type->derivationCount - 1);
    return last &&
           last->kind == TYPE_DERIVATION_ARRAY &&
           last->as.array.isFlexible;
}

static void hashParsedTypeFingerprint(uint64_t* hash, const ParsedType* type) {
    if (!hash) return;
    if (!type) {
        hash_u64(hash, 0);
        return;
    }

    hash_u64(hash, (uint64_t)type->kind);
    hash_u64(hash, (uint64_t)type->tag);
    hash_u64(hash, (uint64_t)type->primitiveType);
    hash_bool(hash, type->isFunctionPointer);
    hash_u64(hash, (uint64_t)type->fpParamCount);
    for (size_t i = 0; i < type->fpParamCount; ++i) {
        hashParsedTypeFingerprint(hash, &type->fpParams[i]);
    }
    hash_string(hash, type->userTypeName);
    hash_bool(hash, type->isConst);
    hash_bool(hash, type->isSigned);
    hash_bool(hash, type->isUnsigned);
    hash_bool(hash, type->isShort);
    hash_bool(hash, type->isLong);
    hash_bool(hash, type->isVolatile);
    hash_bool(hash, type->isRestrict);
    hash_bool(hash, type->isInline);
    hash_bool(hash, type->isStatic);
    hash_bool(hash, type->isExtern);
    hash_bool(hash, type->isRegister);
    hash_bool(hash, type->isAuto);
    hash_u64(hash, (uint64_t)type->pointerDepth);
    hash_bool(hash, type->isVLA);
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

uint64_t fingerprintStructLike(const ASTNode* node) {
    uint64_t hash = FNV_OFFSET;
    if (!node) return hash;

    hash_u64(&hash, node->type == AST_UNION_DEFINITION ? 2u : 1u);
    hash_bool(&hash, node->structDef.hasFlexibleArray);
    for (size_t i = 0; i < node->structDef.fieldCount; ++i) {
        hash_u64(&hash, i + 1);
        hashFieldDeclaration(&hash, node->structDef.fields[i]);
    }
    hash_u64(&hash, node->structDef.fieldCount);
    return hash;
}

uint64_t fingerprintEnumDefinition(const ASTNode* node) {
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

ParsedType makeEnumValueParsedType(void) {
    ParsedType t;
    memset(&t, 0, sizeof(t));
    t.kind = TYPE_PRIMITIVE;
    t.primitiveType = TOKEN_INT;
    t.isConst = true;
    return t;
}

bool tryEvaluateArrayLength(ASTNode* sizeExpr, Scope* scope, size_t* outLen) {
    if (!sizeExpr || !outLen) return false;
    long long value = 0;
    bool ok = constEvalInteger(sizeExpr, scope, &value, true);
    if (!ok || value < 0) {
        return false;
    }
    *outLen = (size_t)value;
    return true;
}

const ParsedType* resolveTypedefInScope(const ParsedType* type, Scope* scope) {
    if (!type || !scope || type->kind != TYPE_NAMED || !type->userTypeName) {
        return type;
    }
    for (Scope* current = scope; current; current = current->parent) {
        Symbol* sym = lookupSymbol(&current->table, type->userTypeName);
        if (sym && sym->kind == SYMBOL_TYPEDEF) {
            return &sym->type;
        }
    }
    return type;
}

void evaluateArrayDerivations(ParsedType* type, Scope* scope) {
    if (!type) return;
    for (size_t d = 0; d < type->derivationCount; ++d) {
        TypeDerivation* deriv = &type->derivations[d];
        if (deriv->kind != TYPE_DERIVATION_ARRAY) {
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
                type->isVLA = true;
            }
        }
    }
}

bool enumExprHasOverflowingIntegerLiteral(ASTNode* expr, Scope* scope) {
    if (!expr) {
        return false;
    }

    switch (expr->type) {
        case AST_NUMBER_LITERAL: {
            if (!expr->valueNode.value) {
                return false;
            }
            IntegerLiteralInfo info;
            if (!parse_integer_literal_info(expr->valueNode.value,
                                            (scope && scope->ctx)
                                                ? cc_get_target_layout(scope->ctx)
                                                : NULL,
                                            &info)) {
                return false;
            }
            return info.ok && info.overflow;
        }
        case AST_UNARY_EXPRESSION:
            return enumExprHasOverflowingIntegerLiteral(expr->expr.left, scope);
        case AST_BINARY_EXPRESSION:
            return enumExprHasOverflowingIntegerLiteral(expr->expr.left, scope) ||
                   enumExprHasOverflowingIntegerLiteral(expr->expr.right, scope);
        case AST_TERNARY_EXPRESSION:
            return enumExprHasOverflowingIntegerLiteral(expr->ternaryExpr.condition, scope) ||
                   enumExprHasOverflowingIntegerLiteral(expr->ternaryExpr.trueExpr, scope) ||
                   enumExprHasOverflowingIntegerLiteral(expr->ternaryExpr.falseExpr, scope);
        case AST_CAST_EXPRESSION:
            return enumExprHasOverflowingIntegerLiteral(expr->castExpr.expression, scope);
        case AST_COMMA_EXPRESSION:
            if (!expr->commaExpr.expressions) {
                return false;
            }
            for (size_t i = 0; i < expr->commaExpr.exprCount; ++i) {
                if (enumExprHasOverflowingIntegerLiteral(expr->commaExpr.expressions[i], scope)) {
                    return true;
                }
            }
            return false;
        default:
            return false;
    }
}

bool enumValueFitsIntRange(long long value, Scope* scope) {
    unsigned intBits = 32;
    const TargetLayout* tl = (scope && scope->ctx) ? cc_get_target_layout(scope->ctx) : NULL;
    if (tl && tl->intBits > 0) {
        intBits = (unsigned)tl->intBits;
    }
    if (intBits >= 64) {
        return true;
    }
    if (intBits == 0) {
        intBits = 32;
    }

    long long minVal = -(1LL << (intBits - 1));
    long long maxVal = (1LL << (intBits - 1)) - 1LL;
    return value >= minVal && value <= maxVal;
}
