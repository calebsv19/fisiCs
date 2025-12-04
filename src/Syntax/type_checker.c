#include "type_checker.h"
#include "scope.h"
#include "symbol_table.h"

#include <string.h>

static TypeInfo makeBaseInvalid(void) {
    TypeInfo info;
    memset(&info, 0, sizeof(info));
    info.category = TYPEINFO_INVALID;
    return info;
}

static void propagateVLAFlag(TypeInfo* info, const ParsedType* type);
static TypeInfo typeInfoFromDerivationIndex(const ParsedType* type, size_t index, Scope* scope);
static PointerQualifier makePointerQual(bool isConst, bool isVolatile, bool isRestrict) {
    PointerQualifier q = { isConst, isVolatile, isRestrict };
    return q;
}

static void clearPointerLevels(TypeInfo* info) {
    if (!info) return;
    for (int i = 0; i < TYPEINFO_MAX_POINTER_DEPTH; ++i) {
        info->pointerLevels[i].isConst = false;
        info->pointerLevels[i].isVolatile = false;
        info->pointerLevels[i].isRestrict = false;
    }
}

void typeInfoPrependPointerLevel(TypeInfo* info, PointerQualifier q) {
    if (!info) return;
    int stored = info->pointerDepth;
    if (stored >= TYPEINFO_MAX_POINTER_DEPTH) {
        stored = TYPEINFO_MAX_POINTER_DEPTH - 1;
    }
    for (int i = stored; i > 0; --i) {
        info->pointerLevels[i] = info->pointerLevels[i - 1];
    }
    if (TYPEINFO_MAX_POINTER_DEPTH > 0) {
        info->pointerLevels[0] = q;
    }
    info->pointerDepth += 1;
    info->category = TYPEINFO_POINTER;
    info->isConst = false;
    info->isVolatile = false;
    info->isRestrict = false;
}

void typeInfoDropPointerLevel(TypeInfo* info) {
    if (!info || info->pointerDepth <= 0) return;
    PointerQualifier pointed = info->pointerLevels[0];
    int stored = info->pointerDepth;
    if (stored > TYPEINFO_MAX_POINTER_DEPTH) {
        stored = TYPEINFO_MAX_POINTER_DEPTH;
    }
    for (int i = 0; i + 1 < stored; ++i) {
        info->pointerLevels[i] = info->pointerLevels[i + 1];
    }
    if (stored > 0) {
        info->pointerLevels[stored - 1] = makePointerQual(false, false, false);
    }
    info->pointerDepth -= 1;
    if (info->pointerDepth > 0) {
        info->isConst = info->pointerLevels[0].isConst;
        info->isVolatile = info->pointerLevels[0].isVolatile;
        info->isRestrict = info->pointerLevels[0].isRestrict;
    } else {
        info->isConst = pointed.isConst;
        info->isVolatile = pointed.isVolatile;
        info->isRestrict = pointed.isRestrict;
    }
}

TypeInfo makeInvalidType(void) {
    return makeBaseInvalid();
}

TypeInfo makeBoolType(void) {
    TypeInfo info = makeBaseInvalid();
    info.category = TYPEINFO_INTEGER;
    info.primitive = TOKEN_BOOL;
    info.bitWidth = 1;
    info.isSigned = false;
    return info;
}

TypeInfo makeIntegerType(unsigned bitWidth, bool isSigned, TokenType primitive) {
    TypeInfo info = makeBaseInvalid();
    info.category = TYPEINFO_INTEGER;
    info.bitWidth = bitWidth;
    info.isSigned = isSigned;
    info.primitive = primitive;
    return info;
}

static unsigned defaultIntBits(void) {
    return 32;
}

static unsigned longBits(void) {
    return 64;
}

TypeInfo makeFloatTypeInfo(bool isDouble) {
    TypeInfo info = makeBaseInvalid();
    info.category = TYPEINFO_FLOAT;
    info.primitive = isDouble ? TOKEN_DOUBLE : TOKEN_FLOAT;
    info.bitWidth = isDouble ? 64 : 32;
    info.isSigned = true;
    return info;
}

static void applyQualifiers(TypeInfo* info, const ParsedType* type) {
    if (!info || !type) return;
    info->isConst    |= type->isConst;
    info->isVolatile |= type->isVolatile;
    info->isRestrict |= type->isRestrict;
}

static const ParsedType* resolveTypedef(const ParsedType* type, Scope* scope) {
    if (!type || !scope || !type->userTypeName) return NULL;
    Scope* current = scope;
    while (current) {
        Symbol* sym = lookupSymbol(&current->table, type->userTypeName);
        if (sym && sym->kind == SYMBOL_TYPEDEF) {
            return &sym->type;
        }
        current = current->parent;
    }
    return NULL;
}

static TypeInfo typeInfoFromBaseKind(const ParsedType* type, Scope* scope) {
    if (!type) {
        return makeInvalidType();
    }
    switch (type->kind) {
        case TYPE_PRIMITIVE: {
            switch (type->primitiveType) {
                case TOKEN_BOOL: {
                    TypeInfo info = makeBoolType();
                    return info;
                }
                case TOKEN_CHAR: {
                    TypeInfo info = makeIntegerType(8, !type->isUnsigned, TOKEN_CHAR);
                    return info;
                }
                case TOKEN_SHORT: {
                    TypeInfo info = makeIntegerType(16, !type->isUnsigned, TOKEN_SHORT);
                    return info;
                }
                case TOKEN_INT:
                case TOKEN_SIGNED:
                case TOKEN_UNSIGNED: {
                    bool isSigned = !type->isUnsigned;
                    TypeInfo info = makeIntegerType(defaultIntBits(), isSigned, TOKEN_INT);
                    return info;
                }
                case TOKEN_LONG: {
                    TypeInfo info = makeIntegerType(longBits(), !type->isUnsigned, TOKEN_LONG);
                    return info;
                }
                case TOKEN_FLOAT: {
                    TypeInfo info = makeFloatTypeInfo(false);
                    return info;
                }
                case TOKEN_DOUBLE: {
                    TypeInfo info = makeFloatTypeInfo(true);
                    return info;
                }
                case TOKEN_VOID: {
                    TypeInfo info = makeBaseInvalid();
                    info.category = TYPEINFO_VOID;
                    info.primitive = TOKEN_VOID;
                    return info;
                }
                default:
                    break;
            }
            break;
        }
        case TYPE_ENUM: {
            TypeInfo info = makeIntegerType(defaultIntBits(), true, TOKEN_INT);
            info.category = TYPEINFO_ENUM;
            info.tag = TAG_ENUM;
            info.userTypeName = type->userTypeName;
            return info;
        }
        case TYPE_STRUCT:
        case TYPE_UNION: {
            TypeInfo info = makeBaseInvalid();
            info.category = (type->kind == TYPE_STRUCT) ? TYPEINFO_STRUCT : TYPEINFO_UNION;
            info.tag = (type->kind == TYPE_STRUCT) ? TAG_STRUCT : TAG_UNION;
            info.userTypeName = type->userTypeName;
            return info;
        }
        case TYPE_NAMED: {
            const ParsedType* resolved = resolveTypedef(type, scope);
            if (!resolved) {
                TypeInfo info = makeInvalidType();
                info.userTypeName = type->userTypeName;
                return info;
            }
            TypeInfo base = typeInfoFromParsedType(resolved, scope);
            applyQualifiers(&base, type);
            return base;
        }
        case TYPE_INVALID:
        default:
            break;
    }

    return makeInvalidType();
}

static void propagateVLAFlag(TypeInfo* info, const ParsedType* type) {
    if (!info || !type) return;
    if (type->isVLA || parsedTypeHasVLA(type)) {
        info->isVLA = true;
    }
}

static TypeInfo typeInfoFromDerivationIndex(const ParsedType* type, size_t index, Scope* scope) {
    if (!type) {
        return makeInvalidType();
    }
    if (index >= type->derivationCount) {
        TypeInfo base = typeInfoFromBaseKind(type, scope);
        applyQualifiers(&base, type);
        base.originalType = type;
        return base;
    }

    const TypeDerivation* deriv = parsedTypeGetDerivation(type, index);
    if (!deriv) {
        return makeInvalidType();
    }

    switch (deriv->kind) {
        case TYPE_DERIVATION_POINTER: {
            TypeInfo target = typeInfoFromDerivationIndex(type, index + 1, scope);
            TypeInfo info = makeBaseInvalid();
            info.category = TYPEINFO_POINTER;
            info.pointerDepth = target.pointerDepth + 1;
            info.primitive = target.primitive;
            info.tag = target.tag;
            info.userTypeName = target.userTypeName;
            info.bitWidth = target.bitWidth;
            info.isSigned = target.isSigned;
            info.originalType = type;
            info.isVLA = target.isVLA;
            clearPointerLevels(&info);
            PointerQualifier targetQual = makePointerQual(target.isConst,
                                                          target.isVolatile,
                                                          target.isRestrict);
            PointerQualifier pointerSelf = makePointerQual(deriv->as.pointer.isConst,
                                                           deriv->as.pointer.isVolatile,
                                                           deriv->as.pointer.isRestrict);
            int stored = info.pointerDepth;
            if (stored > TYPEINFO_MAX_POINTER_DEPTH) {
                stored = TYPEINFO_MAX_POINTER_DEPTH;
            }
            if (stored > 0) {
                info.pointerLevels[0] = targetQual;
            }
            int copyDepth = stored - 1;
            if (copyDepth > 0) {
                int available = target.pointerDepth;
                if (available > TYPEINFO_MAX_POINTER_DEPTH) {
                    available = TYPEINFO_MAX_POINTER_DEPTH;
                }
                int toCopy = copyDepth < available ? copyDepth : available;
                for (int i = 0; i < toCopy; ++i) {
                    info.pointerLevels[i + 1] = target.pointerLevels[i];
                }
            }
            info.isConst = pointerSelf.isConst;
            info.isVolatile = pointerSelf.isVolatile;
            info.isRestrict = pointerSelf.isRestrict;
            return info;
        }
        case TYPE_DERIVATION_ARRAY: {
            TypeInfo element = typeInfoFromDerivationIndex(type, index + 1, scope);
            TypeInfo arrayInfo = element;
            arrayInfo.category = TYPEINFO_ARRAY;
            arrayInfo.isArray = true;
            arrayInfo.isLValue = true;
            arrayInfo.isVLA = element.isVLA || deriv->as.array.isVLA;
            arrayInfo.originalType = type;
            return arrayInfo;
        }
        case TYPE_DERIVATION_FUNCTION: {
            TypeInfo info = makeBaseInvalid();
            info.category = TYPEINFO_FUNCTION;
            info.isFunction = true;
            info.originalType = type;
            info.isLValue = false;
            return info;
        }
        default:
            break;
    }
    return makeInvalidType();
}

TypeInfo typeInfoFromParsedType(const ParsedType* type, Scope* scope) {
    if (!type) return makeInvalidType();

    TypeInfo info;
    if (type->derivationCount > 0) {
        info = typeInfoFromDerivationIndex(type, 0, scope);
    } else if (type->pointerDepth > 0) {
        TypeInfo base = typeInfoFromBaseKind(type, scope);
        info = makeBaseInvalid();
        info.category = TYPEINFO_POINTER;
        info.pointerDepth = type->pointerDepth;
        info.primitive = base.primitive;
        info.tag = base.tag;
        info.userTypeName = base.userTypeName;
        info.bitWidth = base.bitWidth ? base.bitWidth : defaultIntBits();
        info.isSigned = base.isSigned;
        info.originalType = type;
        clearPointerLevels(&info);
        int stored = info.pointerDepth < TYPEINFO_MAX_POINTER_DEPTH
            ? info.pointerDepth
            : TYPEINFO_MAX_POINTER_DEPTH;
        if (stored > 0) {
            info.pointerLevels[0] = makePointerQual(base.isConst, base.isVolatile, base.isRestrict);
        }
    } else {
        info = typeInfoFromBaseKind(type, scope);
        info.originalType = type;
    }

    applyQualifiers(&info, type);
    propagateVLAFlag(&info, type);
    return info;
}

bool typeInfoIsInteger(const TypeInfo* info) {
    if (!info) return false;
    return info->category == TYPEINFO_INTEGER || info->category == TYPEINFO_ENUM;
}

bool typeInfoIsFloating(const TypeInfo* info) {
    return info && info->category == TYPEINFO_FLOAT;
}

bool typeInfoIsArithmetic(const TypeInfo* info) {
    return typeInfoIsInteger(info) || typeInfoIsFloating(info);
}

bool typeInfoIsPointerLike(const TypeInfo* info) {
    if (!info) return false;
    return info->category == TYPEINFO_POINTER || info->category == TYPEINFO_ARRAY || info->isArray;
}

TypeInfo integerPromote(TypeInfo info) {
    if (!typeInfoIsInteger(&info)) {
        return info;
    }

    if (info.bitWidth >= defaultIntBits()) {
        return info;
    }

    info.bitWidth = defaultIntBits();
    info.primitive = TOKEN_INT;
    return info;
}

TypeInfo defaultArgumentPromotion(TypeInfo info) {
    if (typeInfoIsInteger(&info)) {
        info = integerPromote(info);
        info.isLValue = false;
        return info;
    }
    if (typeInfoIsFloating(&info) && info.bitWidth < 64) {
        TypeInfo promoted = makeFloatTypeInfo(true);
        promoted.isLValue = false;
        return promoted;
    }
    info.isLValue = false;
    return info;
}

static int integerRank(const TypeInfo* info) {
    if (!info || !typeInfoIsInteger(info)) return -1;
    switch (info->bitWidth) {
        case 8:  return 1;
        case 16: return 2;
        case 32: return 3;
        case 64: return 4;
        default: return 0;
    }
}

static TypeInfo pickIntegerResult(TypeInfo a, TypeInfo b) {
    int rankA = integerRank(&a);
    int rankB = integerRank(&b);
    if (rankA > rankB) return a;
    if (rankB > rankA) return b;
    if (a.isSigned == b.isSigned) {
        return a.isSigned ? a : b;
    }
    if (!a.isSigned && a.bitWidth >= b.bitWidth) return a;
    if (!b.isSigned && b.bitWidth >= a.bitWidth) return b;
    a.isSigned = false;
    return a;
}

TypeInfo usualArithmeticConversion(TypeInfo left, TypeInfo right, bool* ok) {
    if (ok) *ok = true;

    if (!typeInfoIsArithmetic(&left) || !typeInfoIsArithmetic(&right)) {
        if (ok) *ok = false;
        return makeInvalidType();
    }

    if (typeInfoIsFloating(&left) || typeInfoIsFloating(&right)) {
        if (left.bitWidth == 64 || right.bitWidth == 64) {
            return makeFloatTypeInfo(true);
        }
        return makeFloatTypeInfo(false);
    }

    TypeInfo a = integerPromote(left);
    TypeInfo b = integerPromote(right);
    return pickIntegerResult(a, b);
}

static bool sameString(const char* a, const char* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    return strcmp(a, b) == 0;
}

static bool pointerTargetsEqual(const TypeInfo* a, const TypeInfo* b) {
    return a->primitive == b->primitive &&
           a->tag == b->tag &&
           sameString(a->userTypeName, b->userTypeName);
}

bool typesAreEqual(const TypeInfo* a, const TypeInfo* b) {
    if (!a || !b) return false;
    if (typeInfoIsPointerLike(a) && typeInfoIsPointerLike(b)) {
        if (a->originalType && b->originalType) {
            return parsedTypesStructurallyEqual(a->originalType, b->originalType);
        }
        if (a->pointerDepth != b->pointerDepth) return false;
        return pointerTargetsEqual(a, b);
    }
    if (a->category != b->category) {
        return false;
    }

    switch (a->category) {
        case TYPEINFO_INTEGER:
            return a->bitWidth == b->bitWidth && a->isSigned == b->isSigned;
        case TYPEINFO_FLOAT:
            return a->bitWidth == b->bitWidth;
        case TYPEINFO_STRUCT:
        case TYPEINFO_UNION:
            return a->tag == b->tag && sameString(a->userTypeName, b->userTypeName);
        case TYPEINFO_ENUM:
            return sameString(a->userTypeName, b->userTypeName);
        case TYPEINFO_VOID:
            return true;
        case TYPEINFO_FUNCTION:
            return true;
        case TYPEINFO_INVALID:
        default:
            return false;
    }
}

AssignmentCheckResult canAssignTypes(const TypeInfo* dest, const TypeInfo* src) {
    if (!dest || !src) return ASSIGN_INCOMPATIBLE;

    if (typeInfoIsPointerLike(dest) && typeInfoIsPointerLike(src)) {
        if (dest->pointerDepth != src->pointerDepth) {
            return ASSIGN_INCOMPATIBLE;
        }
        int depth = dest->pointerDepth;
        int limit = depth < TYPEINFO_MAX_POINTER_DEPTH ? depth : TYPEINFO_MAX_POINTER_DEPTH;
        for (int i = 0; i < limit; ++i) {
            PointerQualifier s = src->pointerLevels[i];
            PointerQualifier d = dest->pointerLevels[i];
            if (s.isConst && !d.isConst) return ASSIGN_QUALIFIER_LOSS;
            if (s.isVolatile && !d.isVolatile) return ASSIGN_QUALIFIER_LOSS;
            if (s.isRestrict && !d.isRestrict) return ASSIGN_QUALIFIER_LOSS;
        }
        return typesAreEqual(dest, src) ? ASSIGN_OK : ASSIGN_INCOMPATIBLE;
    }

    if (typeInfoIsArithmetic(dest) && typeInfoIsArithmetic(src)) {
        return ASSIGN_OK;
    }

    if (dest->category == src->category) {
        return ASSIGN_OK;
    }

    return ASSIGN_INCOMPATIBLE;
}
