// SPDX-License-Identifier: Apache-2.0

#include "type_checker.h"

static unsigned defaultIntBits(void) {
    return 32;
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
    {
        unsigned bits = info->bitWidth ? info->bitWidth : 32;
        if (bits <= 8) return 1;
        if (bits <= 16) return 2;
        if (bits <= 32) return 3;
        if (bits <= 64) return 4;
        if (bits <= 128) return 5;
        return 6;
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
        FloatKind fk = FLOAT_KIND_FLOAT;
        bool isComplex = left.isComplex || right.isComplex || left.isImaginary || right.isImaginary;
        bool isImag = left.isImaginary || right.isImaginary;
        unsigned maxBits = left.bitWidth > right.bitWidth ? left.bitWidth : right.bitWidth;
        if (maxBits >= 128) {
            fk = FLOAT_KIND_LONG_DOUBLE;
        } else if (maxBits >= 64) {
            fk = FLOAT_KIND_DOUBLE;
        }
        {
            TypeInfo result = makeFloatTypeInfo(fk, isComplex, NULL);
            result.isImaginary = isImag && !isComplex;
            return result;
        }
    }

    {
        TypeInfo a = integerPromote(left);
        TypeInfo b = integerPromote(right);
        return pickIntegerResult(a, b);
    }
}
