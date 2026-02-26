#include "analyze_decls.h"
#include "symbol_table.h"
#include "syntax_errors.h"
#include "scope.h"
#include "Compiler/compiler_context.h"
#include "Parser/Helpers/designated_init.h"
#include "const_eval.h"
#include "analyze_expr.h"
#include "type_checker.h"
#include "literal_utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

static const uint64_t FNV_OFFSET = 1469598103934665603ULL;
static const uint64_t FNV_PRIME  = 1099511628211ULL;

static bool isFunctionAddressConstant(ASTNode* expr, Scope* scope);

static const ParsedType* resolveTypedefBase(Scope* scope, const ParsedType* type, int depth) {
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
                /*
                 * Some typedef declarations resolve to aggregate/enum bases without
                 * preserving a userTypeName in the parsed type node. Reuse the alias
                 * spelling so redeclaration checks still match equivalent declarations
                 * like `struct Foo*` and `Foo*`.
                 */
                type->userTypeName = oldName;
            } else {
                type->userTypeName = NULL;
            }

            // Keep explicit qualifiers/spelling from the alias use-site.
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

static bool parsedTypesStructurallyCompatibleInScope(const ParsedType* a,
                                                     const ParsedType* b,
                                                     Scope* scope) {
    if (parsedTypesStructurallyEqual(a, b)) {
        return true;
    }
    if (!a || !b || !scope) {
        return false;
    }

    ParsedType lhs = parsedTypeClone(a);
    ParsedType rhs = parsedTypeClone(b);
    canonicalizeParsedTypeAliases(scope, &lhs, 0);
    canonicalizeParsedTypeAliases(scope, &rhs, 0);
    bool ok = parsedTypesStructurallyEqual(&lhs, &rhs);
    if (!ok) {
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

static bool typedefChainContainsName(Scope* scope, const ParsedType* type, const char* name, int depth) {
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

static void hashParsedTypeFingerprint(uint64_t* hash, const ParsedType* type);
static void hashAstNode(uint64_t* hash, const ASTNode* node);

static bool parsedTypeIsFlexibleArray(const ParsedType* type) {
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
    hash_bool(&hash, node->structDef.hasFlexibleArray);
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

static void assignFunctionSignature(Symbol* sym, ASTNode** params, size_t paramCount, bool isVariadic) {
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
                sym->signature.params[idx] = *srcType;
                idx++;
            }
        }
    }
    sym->signature.paramCount = idx;
    sym->signature.hasPrototype = true;
}

static bool functionSignaturesCompatible(const FunctionSignature* lhs,
                                         const FunctionSignature* rhs,
                                         Scope* scope) {
    if (!lhs || !rhs) return true;
    if (!lhs->hasPrototype || !rhs->hasPrototype) {
        return true;
    }
    if ((lhs->paramCount > 0 && !lhs->params) ||
        (rhs->paramCount > 0 && !rhs->params)) {
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

static const char* safeIdentifierName(ASTNode* node) {
    if (node && node->type == AST_IDENTIFIER && node->valueNode.value) {
        return node->valueNode.value;
    }
    return "<unnamed>";
}

static bool tryEvaluateArrayLength(ASTNode* sizeExpr, Scope* scope, size_t* outLen) {
    if (!sizeExpr || !outLen) return false;
    long long value = 0;
    bool ok = constEvalInteger(sizeExpr, scope, &value, true);
    if (!ok || value < 0) {
        return false;
    }
    *outLen = (size_t)value;
    return true;
}

static const ParsedType* resolveTypedefInScope(const ParsedType* type, Scope* scope) {
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

static void evaluateArrayDerivations(ParsedType* type, Scope* scope) {
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

static bool scopeIsFileScope(Scope* scope) {
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

static void applyInteropAttributes(Symbol* sym, ASTNode* node, Scope* scope, bool allowWarn) {
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
        } else if ((isGnu && strstr(tmp, "fastcall")) || (isDeclspec && strstr(tmp, "fastcall"))) {
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

static StorageClass deduceStorageClass(const ParsedType* type) {
    if (!type) return STORAGE_NONE;
    if (type->isExtern) return STORAGE_EXTERN;
    if (type->isStatic) return STORAGE_STATIC;
    if (type->isRegister) return STORAGE_REGISTER;
    if (type->isAuto) return STORAGE_AUTO;
    return STORAGE_NONE;
}

static SymbolLinkage deduceLinkage(const ParsedType* type, bool fileScope) {
    StorageClass sc = deduceStorageClass(type);
    if (sc == STORAGE_STATIC) return LINKAGE_INTERNAL;
    if (sc == STORAGE_EXTERN || fileScope) return LINKAGE_EXTERNAL;
    return LINKAGE_NONE;
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
static void validateBitField(ASTNode* field, ParsedType* fieldType, Scope* scope);
static void maybeRecordConstIntegerValue(Symbol* sym,
                                         const ParsedType* varType,
                                         DesignatedInit* init,
                                         Scope* scope);

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
    switch (node->type) {
        case AST_VARIABLE_DECLARATION: {
            ParsedType* declaredTypes = node->varDecl.declaredTypes;
            for (size_t i = 0; i < node->varDecl.varCount; i++) {
                ASTNode* ident = node->varDecl.varNames[i];
                Symbol* boundSym = NULL;
                ParsedType* varType = declaredTypes ? &declaredTypes[i]
                                                     : &node->varDecl.declaredType;
                analyzeInlineAggregateDefinition(varType, scope);
                TypeInfo varInfo = typeInfoFromParsedType(varType, scope);
                StorageClass storage = deduceStorageClass(varType);
                SymbolLinkage linkage = deduceLinkage(varType, scopeIsFileScope(scope));
                bool hasInitializer = node->varDecl.initializers &&
                                      i < node->varDecl.varCount &&
                                      node->varDecl.initializers[i] != NULL;
                bool fileScope = scopeIsFileScope(scope);
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

                if ((varInfo.category == TYPEINFO_STRUCT || varInfo.category == TYPEINFO_UNION) && !varInfo.isComplete) {
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
                    bool conflict = (existing->linkage == LINKAGE_INTERNAL && linkage != LINKAGE_INTERNAL) ||
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
                    boundSym = existing;
                } else {
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
                const ParsedType* arrayType = (resolvedVar && parsedTypeIsDirectArray(resolvedVar))
                    ? resolvedVar
                    : varType;
                bool isArrayVar = arrayType && parsedTypeIsDirectArray(arrayType);
                if (isArrayVar) {
                    if (parsedTypeHasVLA(arrayType) && staticStorage) {
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
                    bool conflict = (existing->linkage == LINKAGE_INTERNAL && linkage != LINKAGE_INTERNAL) ||
                                    (linkage == LINKAGE_INTERNAL && existing->linkage != LINKAGE_INTERNAL);
                    if (conflict) {
                        addErrorWithRanges(funcName ? funcName->location : node->location,
                                           funcName ? funcName->macroCallSite : node->macroCallSite,
                                           funcName ? funcName->macroDefinition : node->macroDefinition,
                                           "Conflicting linkage for function",
                                           funcName ? funcName->valueNode.value : NULL);
                        break;
                    }
                    if (existing->linkage == LINKAGE_NONE) {
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
                    existing->definition = node;
                    resetFunctionSignature(existing);
                    assignFunctionSignature(existing,
                                            node->functionDef.parameters,
                                            node->functionDef.paramCount,
                                            node->functionDef.isVariadic);
                    existing->hasDefinition = true;
                }
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
            evaluateArrayDerivations(&node->typedefStmt.baseType, scope);
            Symbol* existing = lookupSymbol(&scope->table, aliasName);
            if (existing && existing->kind == SYMBOL_TYPEDEF) {
                if (parsedTypesStructurallyEqual(&existing->type, &node->typedefStmt.baseType)) {
                    // Compatible redeclaration — permitted by C; keep the first definition.
                    break;
                }
                const ParsedType* existingBase = resolveTypedefBase(scope, &existing->type, 0);
                const ParsedType* newBase = resolveTypedefBase(scope, &node->typedefStmt.baseType, 0);
                if (parsedTypesStructurallyEqual(existingBase, newBase)) {
                    // Allow typedef chains that resolve to the same base type (e.g. va_list).
                    break;
                }
                if (typedefChainContainsName(scope, &existing->type, aliasName, 0) &&
                    typedefChainContainsName(scope, &node->typedefStmt.baseType, aliasName, 0)) {
                    // Allow mutually-referential typedef chains (common in system headers).
                    break;
                }
                if (existing->definition == NULL) {
                    // Built-in / seeded typedef being replaced by a real one: update in-place.
                    existing->type = node->typedefStmt.baseType;
                    existing->definition = node;
                    break;
                }
            }

            Symbol* sym = calloc(1, sizeof(Symbol));
            sym->name = strdup(aliasName);
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

            if (isForward && tagKind != CC_TAG_ENUM) {
                if (!cc_add_tag(scope->ctx, tagKind, nameNode->valueNode.value)) {
                    char buffer[128];
                    snprintf(buffer, sizeof(buffer), "Conflicting tag name for %s '%s'", kindLabel, nameNode->valueNode.value);
                    addError(nameNode ? nameNode->line : node->line, 0, buffer, NULL);
                }
            } else {
                CCTagDefineResult result = cc_define_tag(scope->ctx,
                                                         tagKind,
                                                         nameNode->valueNode.value,
                                                         fingerprint,
                                                         node);
                if (result == CC_TAGDEF_CONFLICT) {
                    char buffer[128];
                    snprintf(buffer, sizeof(buffer), "Conflicting definition of %s '%s'", kindLabel, nameNode->valueNode.value);
                    addError(nameNode ? nameNode->line : node->line, 0, buffer, NULL);
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
                        ConstEvalResult val = constEval(node->enumDef.values[i], scope, true);
                        if (!val.isConst) {
                            addErrorWithRanges(member ? member->location : node->location,
                                               member ? member->macroCallSite : node->macroCallSite,
                                               member ? member->macroDefinition : node->macroDefinition,
                                               "Enumerator value is not a constant expression",
                                               member ? member->valueNode.value : NULL);
                        } else {
                            enumVal = val.value;
                            hasValue = true;
                        }
                    } else if (haveCurrent) {
                        enumVal = currentValue + 1;
                        hasValue = true;
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
                    if (isFlexible) {
                        if (node->type == AST_UNION_DEFINITION) {
                            addErrorWithRanges(field->location,
                                               field->macroCallSite,
                                               field->macroDefinition,
                                               "Flexible array members are not allowed in unions",
                                               fieldName);
                        } else {
                            if (i + 1 != node->structDef.fieldCount) {
                                addErrorWithRanges(field->location,
                                                   field->macroCallSite,
                                                   field->macroDefinition,
                                                   "Flexible array member must be the last field in a struct",
                                                   fieldName);
                            }
                            if (field->varDecl.varCount > 1) {
                                addErrorWithRanges(field->location,
                                                   field->macroCallSite,
                                                   field->macroDefinition,
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

static ASTNode* symbolConstInitializerExpr(const Symbol* sym) {
    if (!sym || sym->kind != SYMBOL_VARIABLE || !sym->name || !sym->definition) return NULL;
    ASTNode* def = sym->definition;
    if (def->type != AST_VARIABLE_DECLARATION ||
        !def->varDecl.varNames ||
        !def->varDecl.initializers) {
        return NULL;
    }
    for (size_t i = 0; i < def->varDecl.varCount; ++i) {
        ASTNode* ident = def->varDecl.varNames[i];
        if (!ident || ident->type != AST_IDENTIFIER || !ident->valueNode.value) {
            continue;
        }
        if (strcmp(ident->valueNode.value, sym->name) == 0) {
            DesignatedInit* init = def->varDecl.initializers[i];
            return init ? init->expression : NULL;
        }
    }
    return NULL;
}

static bool isSimpleFloatingConstExpr(ASTNode* expr, Scope* scope, int depth) {
    if (!expr) return false;
    if (depth > 8) return false;
    switch (expr->type) {
        case AST_NUMBER_LITERAL: {
            const char* text = expr->valueNode.value;
            if (!text || !text[0]) return false;
            for (const char* p = text; *p; ++p) {
                if (*p == '.' || *p == 'e' || *p == 'E' || *p == 'p' || *p == 'P' ||
                    *p == 'f' || *p == 'F' || *p == 'i' || *p == 'I' ||
                    *p == 'j' || *p == 'J') {
                    return true;
                }
            }
            return false;
        }
        case AST_IDENTIFIER: {
            if (!scope || !expr->valueNode.value) return false;
            Symbol* sym = resolveInScopeChain(scope, expr->valueNode.value);
            if (!sym || sym->kind != SYMBOL_VARIABLE || !sym->type.isConst) {
                return false;
            }
            ASTNode* initExpr = symbolConstInitializerExpr(sym);
            if (!initExpr) return false;
            return isSimpleFloatingConstExpr(initExpr, scope, depth + 1);
        }
        case AST_CAST_EXPRESSION:
            return isSimpleFloatingConstExpr(expr->castExpr.expression, scope, depth + 1);
        case AST_UNARY_EXPRESSION:
            if (expr->expr.op &&
                (strcmp(expr->expr.op, "+") == 0 || strcmp(expr->expr.op, "-") == 0)) {
                return isSimpleFloatingConstExpr(expr->expr.left, scope, depth + 1);
            }
            return false;
        case AST_BINARY_EXPRESSION:
            if (!expr->expr.op) return false;
            if (strcmp(expr->expr.op, "+") == 0 ||
                strcmp(expr->expr.op, "-") == 0 ||
                strcmp(expr->expr.op, "*") == 0 ||
                strcmp(expr->expr.op, "/") == 0) {
                return isSimpleFloatingConstExpr(expr->expr.left, scope, depth + 1) &&
                       isSimpleFloatingConstExpr(expr->expr.right, scope, depth + 1);
            }
            return false;
        default:
            return false;
    }
}

static void maybeRecordConstIntegerValue(Symbol* sym,
                                         const ParsedType* varType,
                                         DesignatedInit* init,
                                         Scope* scope) {
    if (!sym || !varType || !init || !init->expression || !scope) return;
    if (!varType->isConst) return;

    TypeInfo info = typeInfoFromParsedType(varType, scope);
    if (info.category != TYPEINFO_INTEGER && info.category != TYPEINFO_ENUM) {
        return;
    }

    long long value = 0;
    if (constEvalInteger(init->expression, scope, &value, true)) {
        sym->hasConstValue = true;
        sym->constValue = value;
    }
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
    if (init->expression &&
        typeInfoIsPointerLike(&info) &&
        (init->expression->type == AST_COMPOUND_LITERAL ||
         (init->expression->type == AST_UNARY_EXPRESSION &&
          init->expression->expr.op &&
          strcmp(init->expression->expr.op, "&") == 0 &&
          init->expression->expr.left &&
          init->expression->expr.left->type == AST_COMPOUND_LITERAL))) {
        // Allow pointer initialized from any compound literal (array decays; struct becomes pointer to first field unsupported but not here).
        return;
    }
    if (typeInfoIsStructLike(&info)) {
        if (!init->expression) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "Initializer for struct variable '%s' must not be empty", name);
            addError(nameNode ? nameNode->line : 0, 0, buffer, NULL);
        } else if (init->expression->type == AST_COMPOUND_LITERAL &&
                   init->expression->compoundLiteral.entryCount == 0) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "Empty initializer for struct variable '%s'", name);
            addError(nameNode ? nameNode->line : 0, 0, buffer, NULL);
        }
        return;
    }

    if (init->expression && init->expression->type == AST_COMPOUND_LITERAL) {
        validateScalarCompoundLiteral(init->expression, nameNode, name);
    }

    if (staticStorage && init->expression && init->expression->type == AST_STRING_LITERAL) {
        // String literals are load-time constants for pointers/arrays at static storage.
        return;
    }

    if (staticStorage && init->expression && init->expression->type != AST_COMPOUND_LITERAL) {
        if (typeInfoIsPointerLike(&info) && isFunctionAddressConstant(init->expression, scope)) {
            return;
        }
        if (isSimpleFloatingConstExpr(init->expression, scope, 0)) {
            return;
        }
        long long ignored = 0;
        if (!constEvalInteger(init->expression, scope, &ignored, true)) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer),
                     "Initializer for static variable '%s' is not a constant expression",
                     name ? name : "<unnamed>");
            addError(nameNode ? nameNode->line : 0, 0, buffer, NULL);
        }
    }
}

static bool typeInfoIsCharLike(const TypeInfo* info) {
    return info && info->category == TYPEINFO_INTEGER && info->bitWidth == 8;
}

static bool isFunctionAddressConstant(ASTNode* expr, Scope* scope) {
    if (!expr || !scope) return false;
    ASTNode* target = expr;
    if (expr->type == AST_UNARY_EXPRESSION &&
        expr->expr.op &&
        strcmp(expr->expr.op, "&") == 0 &&
        expr->expr.left) {
        target = expr->expr.left;
    }
    if (!target || target->type != AST_IDENTIFIER) {
        return false;
    }
    Symbol* sym = resolveInScopeChain(scope, target->valueNode.value);
    return sym && sym->kind == SYMBOL_FUNCTION;
}

static bool isStringLiteralInitializer(DesignatedInit* init) {
    return init && !init->fieldName && !init->indexExpr &&
           init->expression && init->expression->type == AST_STRING_LITERAL;
}

static bool array_inner_block_size(const ParsedType* type, size_t* outSize) {
    if (!type || !outSize) return false;
    size_t block = 1;
    bool saw = false;
    for (size_t dim = 1; ; ++dim) {
        const TypeDerivation* arr = parsedTypeGetArrayDerivation(type, dim);
        if (!arr) break;
        if (arr->as.array.isVLA || !arr->as.array.hasConstantSize) {
            return false;
        }
        if (arr->as.array.constantSize < 0) {
            return false;
        }
        block *= (size_t)arr->as.array.constantSize;
        saw = true;
    }
    if (!saw) {
        *outSize = 1;
        return true;
    }
    *outSize = block;
    return true;
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
            if (sizeExpr && tryEvaluateArrayLength(sizeExpr, scope, &declaredLen)) {
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
    ParsedType elementParsed = parsedTypeClone(type);
    if (parsedTypeIsDirectArray(type)) {
        ParsedType elementType = parsedTypeArrayElementType(type);
        parsedTypeFree(&elementParsed);
        elementParsed = parsedTypeClone(&elementType);
        elementInfo = typeInfoFromParsedType(&elementType, scope);
        parsedTypeFree(&elementType);
    }
    bool elementIsArray = parsedTypeIsDirectArray(&elementParsed);
    bool treatAsChar = typeInfoIsCharLike(&elementInfo);
    bool treatAsWideChar = elementInfo.category == TYPEINFO_INTEGER && elementInfo.bitWidth > 8;

    if (valueCount == 1 && isStringLiteralInitializer(values[0])) {
        const char* payload = NULL;
        LiteralEncoding enc = ast_literal_encoding(values[0]->expression->valueNode.value, &payload);
        int charWidth = treatAsWideChar ? (elementInfo.bitWidth ? elementInfo.bitWidth : 32) : 8;
        LiteralDecodeResult res = decode_c_string_literal(payload ? payload : "", charWidth, NULL, NULL);
        size_t needed = res.length + 1;
        bool okType = (treatAsChar && enc != LIT_ENC_WIDE) ||
                      (treatAsWideChar && enc == LIT_ENC_WIDE);
        if (!okType) {
            // fall through to generic checks for incompatible literal/type
        } else if (!res.ok) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer),
                     "Invalid escape sequence in string literal for array '%s'",
                     arrayName);
            addError(contextNode ? contextNode->line : 0, 0, buffer, NULL);
            return;
        } else if (res.overflow) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer),
                     "String literal for array '%s' contains code points not representable in element type",
                     arrayName);
            addError(contextNode ? contextNode->line : 0, 0, buffer, NULL);
            return;
        } else if (hasDeclaredLen && needed > declaredLen) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer),
                     "String literal for array '%s' is too long (needs %zu, size %zu)",
                     arrayName, needed, declaredLen);
            addError(contextNode ? contextNode->line : 0, 0, buffer, NULL);
            return;
        } else if (!hasDeclaredLen && outInferredLength) {
            *outInferredLength = (long long)needed;
            return;
        } else {
            return;
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
        if (array_inner_block_size(type, &block) && block > 0) {
            if (hasDeclaredLen) {
                size_t capacity = declaredLen * block;
                if (scalarCount > capacity) {
                    char buffer[256];
                    snprintf(buffer, sizeof(buffer),
                             "Too many initializers for array '%s' (size %zu)",
                             arrayName,
                             capacity);
                    addError(contextNode ? contextNode->line : 0, 0, buffer, NULL);
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
                sequentialCount = candidate;
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
        }
    }

    parsedTypeFree(&elementParsed);


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

static void validateBitField(ASTNode* field, ParsedType* fieldType, Scope* scope) {
    if (!field || !fieldType || !field->varDecl.bitFieldWidth) return;
    long long width = 0;
    if (!constEvalInteger(field->varDecl.bitFieldWidth, scope, &width, true)) {
        addErrorWithRanges(field->location,
                           field->macroCallSite,
                           field->macroDefinition,
                           "Bitfield width must be an integer constant expression",
                           NULL);
        return;
    }
    if (width < 0) {
        addErrorWithRanges(field->location,
                           field->macroCallSite,
                           field->macroDefinition,
                           "Bitfield width must be non-negative",
                           NULL);
        return;
    }

    ASTNode* nameNode = field->varDecl.varNames ? field->varDecl.varNames[0] : NULL;
    const char* name = safeIdentifierName(nameNode);
    bool hasExplicitName =
        nameNode &&
        nameNode->type == AST_IDENTIFIER &&
        nameNode->valueNode.value != NULL;
    if (width == 0 && hasExplicitName) {
        addErrorWithRanges(field->location,
                           field->macroCallSite,
                           field->macroDefinition,
                           "Zero-width bitfield must be unnamed",
                           name);
        return;
    }

    TypeInfo info = typeInfoFromParsedType(fieldType, scope);
    if (!typeInfoIsInteger(&info) && info.category != TYPEINFO_ENUM) {
        addErrorWithRanges(field->location,
                           field->macroCallSite,
                           field->macroDefinition,
                           "Bitfield requires integral or enum type",
                           name);
        return;
    }
    unsigned bitWidth = info.bitWidth ? info.bitWidth : 0;
    if (bitWidth == 0 || (unsigned long long)width > bitWidth) {
        addErrorWithRanges(field->location,
                           field->macroCallSite,
                           field->macroDefinition,
                           "Bitfield width exceeds type width",
                           name);
    }
}
