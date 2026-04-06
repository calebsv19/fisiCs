// SPDX-License-Identifier: Apache-2.0

#include "type_checker.h"
#include "scope.h"
#include "symbol_table.h"
#include "Compiler/compiler_context.h"
#include "Syntax/target_layout.h"
#include "AST/ast_node.h"
#include "Parser/Helpers/parsed_type_format.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TypeInfo makeBaseInvalid(void) {
    TypeInfo info;
    memset(&info, 0, sizeof(info));
    info.category = TYPEINFO_INVALID;
    info.isComplete = false;
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
    info.isComplete = true;
    return info;
}

TypeInfo makeIntegerType(unsigned bitWidth, bool isSigned, TokenType primitive) {
    TypeInfo info = makeBaseInvalid();
    info.category = TYPEINFO_INTEGER;
    info.bitWidth = bitWidth;
    info.isSigned = isSigned;
    info.primitive = primitive;
    info.isComplete = true;
    return info;
}

static unsigned defaultIntBits(void) {
    return 32;
}

static unsigned longBits(void) {
    return 64;
}

static unsigned targetLongDoubleBits(Scope* scope) {
    const struct TargetLayout* tl = scope && scope->ctx ? cc_get_target_layout(scope->ctx) : tl_default();
    if (!tl) tl = tl_default();
    return (unsigned)tl->longDoubleBits;
}

static bool is_builtin_int128_name(const char* name, bool* outIsUnsigned) {
    if (!name) return false;
    if (strcmp(name, "__int128_t") == 0 || strcmp(name, "__int128") == 0) {
        if (outIsUnsigned) *outIsUnsigned = false;
        return true;
    }
    if (strcmp(name, "__uint128_t") == 0 || strcmp(name, "__uint128") == 0) {
        if (outIsUnsigned) *outIsUnsigned = true;
        return true;
    }
    return false;
}

TypeInfo makeFloatTypeInfo(FloatKind kind, bool isComplex, Scope* scope) {
    TypeInfo info = makeBaseInvalid();
    info.category = TYPEINFO_FLOAT;
    switch (kind) {
        case FLOAT_KIND_LONG_DOUBLE:
            info.primitive = TOKEN_DOUBLE;
            info.bitWidth = targetLongDoubleBits(scope);
            break;
        case FLOAT_KIND_DOUBLE:
            info.primitive = TOKEN_DOUBLE;
            info.bitWidth = 64;
            break;
        case FLOAT_KIND_FLOAT:
        default:
            info.primitive = TOKEN_FLOAT;
            info.bitWidth = 32;
            break;
    }
    info.isComplex = isComplex;
    info.isSigned = true;
    info.isComplete = true;
    return info;
}

static void applyQualifiers(TypeInfo* info, const ParsedType* type) {
    if (!info || !type) return;
    if (info->category == TYPEINFO_POINTER) {
        // Base qualifiers on a pointer type apply to the pointee, not the pointer itself.
        return;
    }
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
                case TOKEN_COMPLEX: {
                    TypeInfo info = makeFloatTypeInfo(FLOAT_KIND_DOUBLE, true, scope);
                    return info;
                }
                case TOKEN_IMAGINARY: {
                    TypeInfo info = makeFloatTypeInfo(FLOAT_KIND_DOUBLE, true, scope);
                    info.isImaginary = true;
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
                    size_t bits = defaultIntBits();
                    TokenType prim = TOKEN_INT;
                    if (type->isShort) {
                        bits = 16;
                        prim = TOKEN_SHORT;
                    } else if (type->isLong) {
                        bits = longBits();
                        prim = TOKEN_LONG;
                    }
                    bool isSigned = !type->isUnsigned;
                    TypeInfo info = makeIntegerType(bits, isSigned, prim);
                    return info;
                }
                case TOKEN_LONG: {
                    TypeInfo info = makeIntegerType(longBits(), !type->isUnsigned, TOKEN_LONG);
                    return info;
                }
                case TOKEN_FLOAT: {
                    TypeInfo info = makeFloatTypeInfo(FLOAT_KIND_FLOAT, type->isComplex || type->isImaginary, scope);
                    info.isImaginary = type->isImaginary;
                    return info;
                }
                case TOKEN_DOUBLE: {
                    FloatKind fk = FLOAT_KIND_DOUBLE;
                    if (type->isLong) {
                        fk = FLOAT_KIND_LONG_DOUBLE;
                    }
                    TypeInfo info = makeFloatTypeInfo(fk, type->isComplex || type->isImaginary, scope);
                    info.isImaginary = type->isImaginary;
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
            info.isComplete = true;
            return info;
        }
        case TYPE_STRUCT:
        case TYPE_UNION: {
            TypeInfo info = makeBaseInvalid();
            info.category = (type->kind == TYPE_STRUCT) ? TYPEINFO_STRUCT : TYPEINFO_UNION;
            info.tag = (type->kind == TYPE_STRUCT) ? TAG_STRUCT : TAG_UNION;
            info.userTypeName = type->userTypeName;
            info.isComplete = (type->inlineStructOrUnionDef != NULL);
            if (!info.isComplete && scope && scope->ctx && type->userTypeName) {
                CCTagKind k = (type->kind == TYPE_STRUCT) ? CC_TAG_STRUCT : CC_TAG_UNION;
                info.isComplete = cc_tag_is_defined(scope->ctx, k, type->userTypeName);
            }
            return info;
        }
        case TYPE_NAMED: {
            bool isUnsignedBuiltin128 = false;
            if (is_builtin_int128_name(type->userTypeName, &isUnsignedBuiltin128)) {
                TypeInfo info = makeIntegerType(128, !isUnsignedBuiltin128, isUnsignedBuiltin128 ? TOKEN_UNSIGNED : TOKEN_LONG);
                applyQualifiers(&info, type);
                return info;
            }
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
            info.isComplex = target.isComplex;
            info.isImaginary = target.isImaginary;
            info.originalType = type;
            info.isVLA = target.isVLA;
            info.isComplete = true;
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
            arrayInfo.isComplete = element.isComplete && !deriv->as.array.isFlexible;
            arrayInfo.isComplex = element.isComplex;
            arrayInfo.isImaginary = element.isImaginary;
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
        info.isComplex = base.isComplex;
        info.isImaginary = base.isImaginary;
        info.originalType = type;
        clearPointerLevels(&info);
        int stored = info.pointerDepth < TYPEINFO_MAX_POINTER_DEPTH
            ? info.pointerDepth
            : TYPEINFO_MAX_POINTER_DEPTH;
        if (stored > 0) {
            info.pointerLevels[0] = makePointerQual(base.isConst, base.isVolatile, base.isRestrict);
        }
        info.isComplete = true; // pointer itself is complete regardless of target
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
        TypeInfo promoted = makeFloatTypeInfo(FLOAT_KIND_DOUBLE, info.isComplex, NULL);
        promoted.isImaginary = info.isImaginary && !info.isComplex;
        promoted.isLValue = false;
        return promoted;
    }
    info.isLValue = false;
    return info;
}

static int integerRank(const TypeInfo* info) {
    if (!info || !typeInfoIsInteger(info)) return -1;
    unsigned bits = info->bitWidth ? info->bitWidth : 32;
    if (bits <= 8) return 1;
    if (bits <= 16) return 2;
    if (bits <= 32) return 3;
    if (bits <= 64) return 4;
    if (bits <= 128) return 5;
    return 6;
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

    /* Floating / complex rules: choose the widest FP kind among operands,
       preserving complex/imag flags. */
    if (typeInfoIsFloating(&left) || typeInfoIsFloating(&right)) {
        FloatKind fk = FLOAT_KIND_FLOAT;
        bool isComplex = left.isComplex || right.isComplex || left.isImaginary || right.isImaginary;
        bool isImag = left.isImaginary || right.isImaginary;
        unsigned maxBits = left.bitWidth > right.bitWidth ? left.bitWidth : right.bitWidth;
        if (maxBits >= 128) {
            fk = FLOAT_KIND_LONG_DOUBLE;
        } else if (maxBits >= 64) {
            fk = FLOAT_KIND_DOUBLE;
        }
        TypeInfo r = makeFloatTypeInfo(fk, isComplex, NULL);
        r.isImaginary = isImag && !isComplex;
        return r;
    }

    /* Integer path: promote then pick rank/unsigned following C usual conversions. */
    TypeInfo a = integerPromote(left);
    TypeInfo b = integerPromote(right);
    return pickIntegerResult(a, b);
}

static bool sameString(const char* a, const char* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    return strcmp(a, b) == 0;
}

static const char* canonicalUserTypeName(const TypeInfo* t) {
    if (!t) return NULL;
    if (t->userTypeName && t->userTypeName[0]) {
        return t->userTypeName;
    }
    if (t->originalType && t->originalType->userTypeName &&
        t->originalType->userTypeName[0]) {
        return t->originalType->userTypeName;
    }
    return NULL;
}

static bool pointerTargetsEqual(const TypeInfo* a, const TypeInfo* b) {
    if (!a || !b) return false;
    const char* aName = canonicalUserTypeName(a);
    const char* bName = canonicalUserTypeName(b);
    if (a->tag != b->tag) {
        return false;
    }
    if (a->tag != TAG_NONE) {
        return sameString(aName, bName);
    }
    if (a->primitive != b->primitive) {
        // Fallback for unresolved named aliases that carry no primitive/tag info.
        return aName && bName && strcmp(aName, bName) == 0;
    }
    return true;
}

static bool parsedTypeIsPointerish(const ParsedType* t) {
    if (!t) return false;
    if (t->pointerDepth > 0) return true;
    for (size_t i = 0; i < t->derivationCount; ++i) {
        if (t->derivations[i].kind == TYPE_DERIVATION_POINTER) {
            return true;
        }
    }
    return false;
}

static void normalizePointerDerivations(ParsedType* t) {
    if (!t || t->pointerDepth <= 0) {
        return;
    }
    size_t ptrCount = parsedTypeCountDerivationsOfKind(t, TYPE_DERIVATION_POINTER);
    while (ptrCount < (size_t)t->pointerDepth) {
        if (!parsedTypeAppendPointer(t)) {
            break;
        }
        ptrCount++;
    }
    parsedTypeNormalizeFunctionPointer(t);
}

bool typesAreEqual(const TypeInfo* a, const TypeInfo* b) {
    if (!a || !b) return false;
    if (typeInfoIsPointerLike(a) && typeInfoIsPointerLike(b)) {
        if (a->originalType && b->originalType &&
            parsedTypeIsPointerish(a->originalType) &&
            parsedTypeIsPointerish(b->originalType)) {
            ParsedType lhs = parsedTypeClone(a->originalType);
            ParsedType rhs = parsedTypeClone(b->originalType);
            normalizePointerDerivations(&lhs);
            normalizePointerDerivations(&rhs);
            bool equal = parsedTypesStructurallyEqual(&lhs, &rhs);
            parsedTypeFree(&lhs);
            parsedTypeFree(&rhs);
            if (equal) {
                return true;
            }
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
            return a->bitWidth == b->bitWidth &&
                   a->isComplex == b->isComplex &&
                   a->isImaginary == b->isImaginary;
        case TYPEINFO_STRUCT:
        case TYPEINFO_UNION:
            return a->tag == b->tag &&
                   sameString(canonicalUserTypeName(a), canonicalUserTypeName(b));
        case TYPEINFO_ENUM:
            return sameString(canonicalUserTypeName(a), canonicalUserTypeName(b));
        case TYPEINFO_VOID:
            return true;
        case TYPEINFO_FUNCTION:
            return true;
        case TYPEINFO_INVALID:
        default:
            return false;
    }
}

static const TypeDerivation* findFunctionDerivation(const ParsedType* type) {
    if (!type) return NULL;
    for (size_t i = 0; i < type->derivationCount; ++i) {
        const TypeDerivation* deriv = parsedTypeGetDerivation(type, i);
        if (deriv && deriv->kind == TYPE_DERIVATION_FUNCTION) {
            return deriv;
        }
    }
    return NULL;
}

static bool typeInfoIsFunctionPointer(const TypeInfo* info) {
    if (!info || !info->originalType) return false;
    if (info->originalType->isFunctionPointer) return true;
    if (info->pointerDepth <= 0) return false;
    ParsedType target = parsedTypePointerTargetType(info->originalType);
    bool hasFunction = findFunctionDerivation(&target) != NULL;
    parsedTypeFree(&target);
    return hasFunction;
}

static bool functionPointerTargetsCompatible(const ParsedType* destType, const ParsedType* srcType) {
    if (!destType || !srcType) return true;

    ParsedType destTarget = parsedTypePointerTargetType(destType);
    ParsedType srcTarget = parsedTypePointerTargetType(srcType);

    const TypeDerivation* destFn = findFunctionDerivation(&destTarget);
    const TypeDerivation* srcFn = findFunctionDerivation(&srcTarget);
    if (!destFn || !srcFn) {
        parsedTypeFree(&destTarget);
        parsedTypeFree(&srcTarget);
        return true;
    }

    ParsedType destRet = parsedTypeFunctionReturnType(&destTarget);
    ParsedType srcRet = parsedTypeFunctionReturnType(&srcTarget);
    bool retCompat = parsedTypesStructurallyEqual(&destRet, &srcRet);
    parsedTypeFree(&destRet);
    parsedTypeFree(&srcRet);
    if (!retCompat) {
        parsedTypeFree(&destTarget);
        parsedTypeFree(&srcTarget);
        return false;
    }

    bool destHasParams = destFn->as.function.paramCount > 0 || destFn->as.function.isVariadic;
    bool srcHasParams = srcFn->as.function.paramCount > 0 || srcFn->as.function.isVariadic;
    if (!destHasParams || !srcHasParams) {
        parsedTypeFree(&destTarget);
        parsedTypeFree(&srcTarget);
        return true;
    }

    if (destFn->as.function.isVariadic != srcFn->as.function.isVariadic) {
        parsedTypeFree(&destTarget);
        parsedTypeFree(&srcTarget);
        return false;
    }

    if (destFn->as.function.paramCount != srcFn->as.function.paramCount) {
        parsedTypeFree(&destTarget);
        parsedTypeFree(&srcTarget);
        return false;
    }

    for (size_t i = 0; i < destFn->as.function.paramCount; ++i) {
        if (!parsedTypesStructurallyEqual(&destFn->as.function.params[i],
                                          &srcFn->as.function.params[i])) {
            parsedTypeFree(&destTarget);
            parsedTypeFree(&srcTarget);
            return false;
        }
    }

    parsedTypeFree(&destTarget);
    parsedTypeFree(&srcTarget);
    return true;
}

static bool typeInfoIsVoidPointer(const TypeInfo* info) {
    if (!info || info->category != TYPEINFO_POINTER) return false;
    if (info->pointerDepth != 1) return false;
    if (info->originalType) {
        ParsedType target = parsedTypePointerTargetType(info->originalType);
        bool isVoid = target.kind == TYPE_PRIMITIVE && target.primitiveType == TOKEN_VOID;
        parsedTypeFree(&target);
        if (isVoid) return true;
    }
    return info->primitive == TOKEN_VOID;
}

static size_t collectPointerDerivations(const ParsedType* type,
                                        const TypeDerivation** out,
                                        size_t maxOut) {
    if (!type || !out || maxOut == 0) return 0;
    size_t count = 0;
    for (size_t i = 0; i < type->derivationCount; ++i) {
        const TypeDerivation* d = parsedTypeGetDerivation(type, i);
        if (!d || d->kind != TYPE_DERIVATION_POINTER) {
            continue;
        }
        if (count < maxOut) {
            out[count] = d;
        }
        count++;
    }
    return count;
}

static void clearParsedTypeQualifiers(ParsedType* type) {
    if (!type) return;
    type->isConst = false;
    type->isVolatile = false;
    type->isRestrict = false;
    for (size_t i = 0; i < type->derivationCount; ++i) {
        TypeDerivation* deriv = &type->derivations[i];
        if (deriv->kind == TYPE_DERIVATION_POINTER) {
            deriv->as.pointer.isConst = false;
            deriv->as.pointer.isVolatile = false;
            deriv->as.pointer.isRestrict = false;
        } else if (deriv->kind == TYPE_DERIVATION_ARRAY) {
            bool vlaLike = deriv->as.array.isVLA ||
                           (!deriv->as.array.hasConstantSize &&
                            deriv->as.array.sizeExpr &&
                            !deriv->as.array.isFlexible);
            deriv->as.array.isVLA = vlaLike;
        }
    }
}

static bool parsedTypesEqualIgnoringQualifiers(const ParsedType* lhs, const ParsedType* rhs) {
    if (!lhs || !rhs) return false;
    ParsedType lhsCopy = parsedTypeClone(lhs);
    ParsedType rhsCopy = parsedTypeClone(rhs);
    normalizePointerDerivations(&lhsCopy);
    normalizePointerDerivations(&rhsCopy);
    clearParsedTypeQualifiers(&lhsCopy);
    clearParsedTypeQualifiers(&rhsCopy);
    bool equal = parsedTypesStructurallyEqual(&lhsCopy, &rhsCopy);
    parsedTypeFree(&lhsCopy);
    parsedTypeFree(&rhsCopy);
    return equal;
}

static bool functionPointerQualifierLoss(const ParsedType* destType, const ParsedType* srcType) {
    if (!destType || !srcType) return false;
    const TypeDerivation* destPtrs[TYPEINFO_MAX_POINTER_DEPTH] = {0};
    const TypeDerivation* srcPtrs[TYPEINFO_MAX_POINTER_DEPTH] = {0};
    size_t destCount = collectPointerDerivations(destType, destPtrs, TYPEINFO_MAX_POINTER_DEPTH);
    size_t srcCount = collectPointerDerivations(srcType, srcPtrs, TYPEINFO_MAX_POINTER_DEPTH);
    size_t pairCount = destCount < srcCount ? destCount : srcCount;

    /* Ignore top-level pointer qualifiers; compare nested pointed-to levels only. */
    if (pairCount <= 1) {
        return false;
    }

    for (size_t i = 0; i + 1 < pairCount; ++i) {
        const TypeDerivation* d = destPtrs[i];
        const TypeDerivation* s = srcPtrs[i];
        if (!d || !s) continue;
        if (s->as.pointer.isConst && !d->as.pointer.isConst) return true;
        if (s->as.pointer.isVolatile && !d->as.pointer.isVolatile) return true;
        if (s->as.pointer.isRestrict && !d->as.pointer.isRestrict) return true;
    }
    return false;
}

static void debugAssignmentMismatch(const TypeInfo* dest, const TypeInfo* src, const char* reason) {
    const char* dbg = getenv("FISICS_DEBUG_ASSIGN");
    if (!dbg || dbg[0] == '\0' || strcmp(dbg, "0") == 0) {
        return;
    }
    char* destType = parsed_type_to_string(dest ? dest->originalType : NULL);
    char* srcType = parsed_type_to_string(src ? src->originalType : NULL);
    fprintf(stderr,
            "[assign-debug] %s | dest(cat=%d tag=%d depth=%d name=%s parsed=%s) "
            "src(cat=%d tag=%d depth=%d name=%s parsed=%s)\n",
            reason ? reason : "mismatch",
            dest ? (int)dest->category : -1,
            dest ? (int)dest->tag : -1,
            dest ? dest->pointerDepth : -1,
            (dest && canonicalUserTypeName(dest)) ? canonicalUserTypeName(dest) : "<null>",
            destType ? destType : "<null>",
            src ? (int)src->category : -1,
            src ? (int)src->tag : -1,
            src ? src->pointerDepth : -1,
            (src && canonicalUserTypeName(src)) ? canonicalUserTypeName(src) : "<null>",
            srcType ? srcType : "<null>");
    free(destType);
    free(srcType);
}

AssignmentCheckResult canAssignTypes(const TypeInfo* dest, const TypeInfo* src) {
    if (!dest || !src) return ASSIGN_INCOMPATIBLE;

    if (typeInfoIsPointerLike(dest) && typeInfoIsPointerLike(src)) {
        bool destVoidPtr = typeInfoIsVoidPointer(dest);
        bool srcVoidPtr = typeInfoIsVoidPointer(src);
        if ((destVoidPtr && !typeInfoIsFunctionPointer(src)) ||
            (srcVoidPtr && !typeInfoIsFunctionPointer(dest))) {
            PointerQualifier s = { 0 };
            PointerQualifier d = { 0 };
            if (src->pointerDepth > 0) s = src->pointerLevels[0];
            if (dest->pointerDepth > 0) d = dest->pointerLevels[0];
            if (s.isConst && !d.isConst) return ASSIGN_QUALIFIER_LOSS;
            if (s.isVolatile && !d.isVolatile) return ASSIGN_QUALIFIER_LOSS;
            if (s.isRestrict && !d.isRestrict) return ASSIGN_QUALIFIER_LOSS;
            return ASSIGN_OK;
        }
        if (dest->pointerDepth != src->pointerDepth) {
            debugAssignmentMismatch(dest, src, "pointer depth mismatch");
            return ASSIGN_INCOMPATIBLE;
        }
        if (functionPointerQualifierLoss(dest->originalType, src->originalType)) {
            return ASSIGN_QUALIFIER_LOSS;
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
        if (typeInfoIsFunctionPointer(dest) && typeInfoIsFunctionPointer(src)) {
            if (!functionPointerTargetsCompatible(dest->originalType, src->originalType)) {
                debugAssignmentMismatch(dest, src, "function pointer target mismatch");
                return ASSIGN_INCOMPATIBLE;
            }
            return ASSIGN_OK;
        }
        if (parsedTypesEqualIgnoringQualifiers(dest->originalType, src->originalType)) {
            return ASSIGN_OK;
        }
        if (typesAreEqual(dest, src)) {
            return ASSIGN_OK;
        }
        debugAssignmentMismatch(dest, src, "pointer targets mismatch");
        return ASSIGN_INCOMPATIBLE;
    }

    if (typeInfoIsArithmetic(dest) && typeInfoIsArithmetic(src)) {
        return ASSIGN_OK;
    }

    if (dest->category == src->category) {
        if (dest->category == TYPEINFO_STRUCT ||
            dest->category == TYPEINFO_UNION ||
            dest->category == TYPEINFO_ENUM) {
            return typesAreEqual(dest, src) ? ASSIGN_OK : ASSIGN_INCOMPATIBLE;
        }
        return ASSIGN_OK;
    }

    debugAssignmentMismatch(dest, src, "category mismatch");
    return ASSIGN_INCOMPATIBLE;
}
