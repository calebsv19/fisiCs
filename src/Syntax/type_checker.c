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

TypeInfo typeInfoFromParsedType(const ParsedType* type, Scope* scope) {
    if (!type) return makeInvalidType();

    if (type->pointerDepth > 0) {
        TypeInfo info = makeBaseInvalid();
        info.category = TYPEINFO_POINTER;
        info.pointerDepth = type->pointerDepth;
        info.primitive = type->primitiveType;
        info.tag = type->tag;
        info.userTypeName = type->userTypeName;
        info.bitWidth = defaultIntBits();
        info.isSigned = false;
        info.originalType = type;
        applyQualifiers(&info, type);
        return info;
    }

    switch (type->kind) {
        case TYPE_PRIMITIVE: {
            switch (type->primitiveType) {
                case TOKEN_BOOL:
                    return makeBoolType();
                case TOKEN_CHAR: {
                    TypeInfo info = makeIntegerType(8, !type->isUnsigned, TOKEN_CHAR);
                    applyQualifiers(&info, type);
                    info.originalType = type;
                    return info;
                }
                case TOKEN_SHORT: {
                    TypeInfo info = makeIntegerType(16, !type->isUnsigned, TOKEN_SHORT);
                    applyQualifiers(&info, type);
                    info.originalType = type;
                    return info;
                }
                case TOKEN_INT:
                case TOKEN_SIGNED:
                case TOKEN_UNSIGNED: {
                    bool isSigned = !type->isUnsigned;
                    TypeInfo info = makeIntegerType(defaultIntBits(), isSigned, TOKEN_INT);
                    applyQualifiers(&info, type);
                    info.originalType = type;
                    return info;
                }
                case TOKEN_LONG: {
                    TypeInfo info = makeIntegerType(longBits(), !type->isUnsigned, TOKEN_LONG);
                    applyQualifiers(&info, type);
                    info.originalType = type;
                    return info;
                }
                case TOKEN_FLOAT: {
                    TypeInfo info = makeFloatTypeInfo(false);
                    applyQualifiers(&info, type);
                    info.originalType = type;
                    return info;
                }
                case TOKEN_DOUBLE: {
                    TypeInfo info = makeFloatTypeInfo(true);
                    applyQualifiers(&info, type);
                    info.originalType = type;
                    return info;
                }
                case TOKEN_VOID: {
                    TypeInfo info = makeBaseInvalid();
                    info.category = TYPEINFO_VOID;
                    info.primitive = TOKEN_VOID;
                    applyQualifiers(&info, type);
                    info.originalType = type;
                    return info;
                }
                default:
                    break;
            }
            break;
        }
        case TYPE_ENUM: {
            TypeInfo info = makeIntegerType(defaultIntBits(), true, TOKEN_INT);
            applyQualifiers(&info, type);
            info.category = TYPEINFO_ENUM;
            info.tag = TAG_ENUM;
            info.userTypeName = type->userTypeName;
            info.originalType = type;
            return info;
        }
        case TYPE_STRUCT:
        case TYPE_UNION: {
            TypeInfo info = makeBaseInvalid();
            info.category = (type->kind == TYPE_STRUCT) ? TYPEINFO_STRUCT : TYPEINFO_UNION;
            info.tag = (type->kind == TYPE_STRUCT) ? TAG_STRUCT : TAG_UNION;
            info.userTypeName = type->userTypeName;
            info.originalType = type;
            applyQualifiers(&info, type);
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
            base.originalType = resolved;
            return base;
        }
        case TYPE_INVALID:
        default:
            break;
    }

    return makeInvalidType();
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
    return info && (info->category == TYPEINFO_POINTER || info->category == TYPEINFO_ARRAY);
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
        if (src->isConst && !dest->isConst) return ASSIGN_QUALIFIER_LOSS;
        if (src->isVolatile && !dest->isVolatile) return ASSIGN_QUALIFIER_LOSS;
        if (src->isRestrict && !dest->isRestrict) return ASSIGN_QUALIFIER_LOSS;
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
