// SPDX-License-Identifier: Apache-2.0

#include "Parser/Helpers/parsed_type_format.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* data;
    size_t len;
    size_t cap;
} StrBuf;

static bool sb_reserve(StrBuf* b, size_t extra) {
    if (!b) return false;
    size_t need = b->len + extra + 1;
    if (need <= b->cap) return true;
    size_t newCap = b->cap ? b->cap * 2 : 64;
    while (newCap < need) newCap *= 2;
    char* grown = (char*)realloc(b->data, newCap);
    if (!grown) return false;
    b->data = grown;
    b->cap = newCap;
    return true;
}

static bool sb_append(StrBuf* b, const char* s) {
    if (!b || !s) return false;
    size_t n = strlen(s);
    if (!sb_reserve(b, n)) return false;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
    return true;
}

static bool sb_appendf(StrBuf* b, const char* fmt, ...) {
    if (!b || !fmt) return false;
    va_list args;
    va_start(args, fmt);
    char tmp[256];
    int n = vsnprintf(tmp, sizeof(tmp), fmt, args);
    va_end(args);
    if (n < 0) return false;
    if ((size_t)n >= sizeof(tmp)) {
        char* dyn = (char*)malloc((size_t)n + 1);
        if (!dyn) return false;
        va_start(args, fmt);
        vsnprintf(dyn, (size_t)n + 1, fmt, args);
        va_end(args);
        bool ok = sb_append(b, dyn);
        free(dyn);
        return ok;
    }
    return sb_append(b, tmp);
}

static bool append_parsed_type(StrBuf* b, const ParsedType* pt) {
    if (!pt) return sb_append(b, "<null-type>");

    if (pt->isStatic)   if (!sb_append(b, "static ")) return false;
    if (pt->isExtern)   if (!sb_append(b, "extern ")) return false;
    if (pt->isRegister) if (!sb_append(b, "register ")) return false;
    if (pt->isAuto)     if (!sb_append(b, "auto ")) return false;

    if (pt->isConst)    if (!sb_append(b, "const ")) return false;
    if (pt->isSigned)   if (!sb_append(b, "signed ")) return false;
    if (pt->isUnsigned) if (!sb_append(b, "unsigned ")) return false;
    if (pt->isShort)    if (!sb_append(b, "short ")) return false;
    if (pt->isLong)     if (!sb_append(b, "long ")) return false;
    if (pt->isComplex)  if (!sb_append(b, "_Complex ")) return false;
    if (pt->isImaginary) if (!sb_append(b, "_Imaginary ")) return false;
    if (pt->isVolatile) if (!sb_append(b, "volatile ")) return false;
    if (pt->isRestrict) if (!sb_append(b, "restrict ")) return false;
    if (pt->isInline)   if (!sb_append(b, "inline ")) return false;

    if (pt->kind == TYPE_PRIMITIVE) {
        if (!sb_append(b, getTokenTypeName(pt->primitiveType))) return false;
    } else {
        if (pt->tag == TAG_STRUCT) if (!sb_append(b, "struct ")) return false;
        if (pt->tag == TAG_UNION)  if (!sb_append(b, "union ")) return false;
        if (!sb_append(b, pt->userTypeName ? pt->userTypeName : "<anon>")) return false;
    }

    if (pt->isFunctionPointer) {
        if (!sb_append(b, " (")) return false;
        bool sawPointerDerivation = false;
        for (size_t i = pt->derivationCount; i > 0; --i) {
            const TypeDerivation* deriv = parsedTypeGetDerivation(pt, i - 1);
            if (!deriv || deriv->kind != TYPE_DERIVATION_POINTER) {
                continue;
            }
            sawPointerDerivation = true;
            if (!sb_append(b, "*")) return false;
            if (deriv->as.pointer.isConst) if (!sb_append(b, " const")) return false;
            if (deriv->as.pointer.isVolatile) if (!sb_append(b, " volatile")) return false;
            if (deriv->as.pointer.isRestrict) if (!sb_append(b, " restrict")) return false;
        }
        if (!sawPointerDerivation) {
            if (pt->pointerDepth > 0) {
                for (int i = 0; i < pt->pointerDepth; i++) {
                    if (!sb_append(b, "*")) return false;
                }
            } else {
                if (!sb_append(b, "*")) return false;
            }
        }
        if (!sb_append(b, ")(")) return false;
        for (size_t i = 0; i < pt->fpParamCount; i++) {
            if (i && !sb_append(b, ", ")) return false;
            if (!append_parsed_type(b, &pt->fpParams[i])) return false;
        }
        if (!sb_append(b, ")")) return false;
    } else {
        bool sawPointerDerivation = false;
        for (size_t i = pt->derivationCount; i > 0; --i) {
            const TypeDerivation* deriv = parsedTypeGetDerivation(pt, i - 1);
            if (!deriv || deriv->kind != TYPE_DERIVATION_POINTER) {
                continue;
            }
            sawPointerDerivation = true;
            if (!sb_append(b, "*")) return false;
            if (deriv->as.pointer.isConst) if (!sb_append(b, " const")) return false;
            if (deriv->as.pointer.isVolatile) if (!sb_append(b, " volatile")) return false;
            if (deriv->as.pointer.isRestrict) if (!sb_append(b, " restrict")) return false;
        }
        if (!sawPointerDerivation) {
            for (int i = 0; i < pt->pointerDepth; i++) {
                if (!sb_append(b, "*")) return false;
            }
        }
    }

    for (size_t i = 0; i < pt->derivationCount; ++i) {
        const TypeDerivation* deriv = parsedTypeGetDerivation(pt, i);
        if (!deriv) continue;
        switch (deriv->kind) {
            case TYPE_DERIVATION_ARRAY:
                if (!sb_append(b, " [")) return false;
                if (deriv->as.array.isVLA) {
                    if (!sb_append(b, "VLA")) return false;
                } else if (deriv->as.array.isFlexible) {
                    if (!sb_append(b, "flex")) return false;
                } else if (deriv->as.array.hasConstantSize) {
                    if (!sb_appendf(b, "%lld", deriv->as.array.constantSize)) return false;
                } else if (deriv->as.array.sizeExpr) {
                    if (!sb_append(b, "expr")) return false;
                } else {
                    if (!sb_append(b, "?")) return false;
                }
                if (!sb_append(b, "]")) return false;
                break;
            case TYPE_DERIVATION_FUNCTION:
                if (pt->isFunctionPointer) {
                    break;
                }
                if (!sb_append(b, " (")) return false;
                for (size_t p = 0; p < deriv->as.function.paramCount; ++p) {
                    if (p && !sb_append(b, ", ")) return false;
                    if (!append_parsed_type(b, &deriv->as.function.params[p])) return false;
                }
                if (deriv->as.function.isVariadic) {
                    if (deriv->as.function.paramCount > 0) {
                        if (!sb_append(b, ", ")) return false;
                    }
                    if (!sb_append(b, "...")) return false;
                }
                if (!sb_append(b, ")")) return false;
                break;
            case TYPE_DERIVATION_POINTER:
                break;
            default:
                break;
        }
    }
    return true;
}

char* parsed_type_to_string(const ParsedType* type) {
    StrBuf b = {0};
    if (!append_parsed_type(&b, type)) {
        free(b.data);
        return NULL;
    }
    if (!b.data) {
        b.data = (char*)malloc(1);
        if (!b.data) return NULL;
        b.data[0] = '\0';
    }
    return b.data;
}
