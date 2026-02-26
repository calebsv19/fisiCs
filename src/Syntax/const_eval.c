#include "const_eval.h"

#include "analyze_expr.h"
#include "layout.h"
#include "literal_utils.h"
#include "scope.h"
#include "symbol_table.h"
#include "type_checker.h"
#include "syntax_errors.h"
#include "target_layout.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

static const TargetLayout* currentLayout(Scope* scope) {
    const TargetLayout* tl = (scope && scope->ctx) ? cc_get_target_layout(scope->ctx) : NULL;
    return tl ? tl : tl_default();
}

static unsigned defaultIntBits(Scope* scope) {
    const TargetLayout* tl = currentLayout(scope);
    return (unsigned)((tl && tl->intBits) ? tl->intBits : 32);
}

static unsigned defaultPointerBits(Scope* scope) {
    const TargetLayout* tl = currentLayout(scope);
    return (unsigned)((tl && tl->pointerBits) ? tl->pointerBits : (sizeof(void*) * 8));
}

static bool evalOffsetofField(Scope* scope,
                              const ParsedType* type,
                              const char* fieldName,
                              size_t* offsetOut) {
    if (!scope || !scope->ctx || !type || !fieldName || !offsetOut) return false;
    TypeInfo info = typeInfoFromParsedType(type, scope);
    if (info.category != TYPEINFO_STRUCT && info.category != TYPEINFO_UNION) return false;
    CCTagKind kind = (info.category == TYPEINFO_STRUCT) ? CC_TAG_STRUCT : CC_TAG_UNION;
    const CCTagFieldLayout* layouts = NULL;
    size_t count = 0;
    if (!cc_get_tag_field_layouts(scope->ctx, kind, info.userTypeName, &layouts, &count) || !layouts) {
        size_t sz = 0, al = 0;
        if (!layout_struct_union(scope->ctx, scope, kind, info.userTypeName, &sz, &al)) return false;
        if (!cc_get_tag_field_layouts(scope->ctx, kind, info.userTypeName, &layouts, &count) || !layouts) {
            return false;
        }
    }
    for (size_t i = 0; i < count; ++i) {
        const CCTagFieldLayout* lay = &layouts[i];
        if (!lay->name) continue;
        if (strcmp(lay->name, fieldName) == 0) {
            if (lay->isBitfield && !lay->isZeroWidth) return false;
            *offsetOut = lay->byteOffset;
            return true;
        }
    }
    return false;
}

static long long truncateToWidth(long long value, unsigned bits, bool isUnsigned) {
    if (bits == 0 || bits >= 64) {
        return value;
    }
    uint64_t mask = (bits == 64) ? ~0ULL : ((1ULL << bits) - 1ULL);
    uint64_t v = ((uint64_t)value) & mask;
    if (!isUnsigned && bits > 0) {
        uint64_t sign = 1ULL << (bits - 1);
        if (v & sign) {
            v |= ~mask;
        }
    }
    return (long long)v;
}

static ConstEvalResult makeConstTyped(long long value, unsigned bits, bool isUnsigned) {
    ConstEvalResult r = { true, truncateToWidth(value, bits, isUnsigned), bits, isUnsigned };
    return r;
}

static ConstEvalResult makeConst(long long value) {
    return makeConstTyped(value, 64, false);
}

static ConstEvalResult makeNonConst(void) {
    ConstEvalResult r = { false, 0, 0, false };
    return r;
}

static bool parseIntegerLiteral(const char* text,
                                Scope* scope,
                                long long* out,
                                unsigned* bitsOut,
                                bool* isUnsignedOut) {
    if (!text || !out) return false;
    IntegerLiteralInfo info;
    if (!parse_integer_literal_info(text, currentLayout(scope), &info) || !info.ok) {
        return false;
    }
    long long val = (long long)info.value;
    if (out) *out = val;
    if (bitsOut) *bitsOut = info.bits;
    if (isUnsignedOut) *isUnsignedOut = info.isUnsigned;
    return true;
}

static bool sizeFromTypeInfo(const TypeInfo* info, Scope* scope, long long* out) {
    (void)scope;
    if (!info || !out) return false;
    switch (info->category) {
        case TYPEINFO_INTEGER:
        case TYPEINFO_ENUM: {
            unsigned bits = info->bitWidth ? info->bitWidth : 32;
            unsigned bytes = (bits + 7) / 8;
            if (bytes == 0) bytes = 1;
            *out = (long long)bytes;
            return true;
        }
        case TYPEINFO_FLOAT: {
            unsigned bits = info->bitWidth ? info->bitWidth : 32;
            unsigned bytes = (bits + 7) / 8;
            if (bytes == 0) bytes = 4;
            if (info->isComplex) {
                bytes *= 2;
            }
            *out = (long long)bytes;
            return true;
        }
        case TYPEINFO_POINTER:
            *out = (long long)sizeof(void*);
            return true;
        case TYPEINFO_ARRAY:
            // Without element count preserved in TypeInfo, caller must handle arrays via ParsedType path.
            return false;
        case TYPEINFO_STRUCT:
        case TYPEINFO_UNION: {
            if (!scope || !scope->ctx || !info->userTypeName) return false;
            size_t sz = 0, al = 0;
            CCTagKind kind = (info->category == TYPEINFO_STRUCT) ? CC_TAG_STRUCT : CC_TAG_UNION;
            if (!layout_struct_union(scope->ctx, scope, kind, info->userTypeName, &sz, &al)) {
                return false;
            }
            *out = (long long)sz;
            return true;
        }
        default:
            return false;
    }
}

static ConstEvalResult constEvalInternal(ASTNode* expr,
                                         Scope* scope,
                                         bool allowEnumRefs);

static bool sizeFromParsedType(const ParsedType* type, Scope* scope, long long* out) {
    if (!type || !out) return false;

    size_t sz = 0, al = 0;
    ParsedType mutableType = *type;
    if (!size_align_of_parsed_type(&mutableType, scope, &sz, &al)) {
        return false;
    }
    *out = (long long)sz;
    return true;
}

static bool alignFromParsedType(const ParsedType* type, Scope* scope, long long* out) {
    if (!type || !out) return false;
    size_t sz = 0, al = 0;
    ParsedType mutableType = *type;
    if (!size_align_of_parsed_type(&mutableType, scope, &sz, &al)) {
        return false;
    }
    *out = (long long)al;
    return true;
}

static bool alignFromTypeInfo(const TypeInfo* info, Scope* scope, long long* out) {
    if (!info || !scope || !out) return false;
    const TargetLayout* tl = currentLayout(scope);
    switch (info->category) {
        case TYPEINFO_INTEGER:
        case TYPEINFO_ENUM: {
            unsigned bits = info->bitWidth ? info->bitWidth : 32;
            size_t align = 0;
            if (bits <= tl->charBits) align = tl->charAlign;
            else if (bits <= tl->shortBits) align = tl->shortAlign;
            else if (bits <= tl->intBits) align = tl->intAlign;
            else if (bits <= tl->longBits) align = tl->longAlign;
            else align = tl->longLongAlign;
            *out = (long long)align;
            return true;
        }
        case TYPEINFO_FLOAT: {
            unsigned bits = info->bitWidth ? info->bitWidth : 32;
            size_t align = tl->floatAlign;
            if (bits >= tl->doubleBits) align = tl->doubleAlign;
            if (bits >= tl->longDoubleBits) align = tl->longDoubleAlign;
            *out = (long long)align;
            return true;
        }
        case TYPEINFO_POINTER:
            *out = (long long)tl->pointerAlign;
            return true;
        case TYPEINFO_STRUCT:
        case TYPEINFO_UNION: {
            if (!scope->ctx || !info->userTypeName) return false;
            size_t sz = 0, al = 0;
            CCTagKind kind = (info->category == TYPEINFO_STRUCT) ? CC_TAG_STRUCT : CC_TAG_UNION;
            if (!layout_struct_union(scope->ctx, scope, kind, info->userTypeName, &sz, &al)) return false;
            *out = (long long)al;
            return true;
        }
        default:
            return false;
    }
}

static bool bitsFromTypeInfo(const TypeInfo* info, Scope* scope, unsigned* bits, bool* isUnsigned) {
    if (!info || !bits || !isUnsigned) return false;
    switch (info->category) {
        case TYPEINFO_INTEGER:
        case TYPEINFO_ENUM:
            *bits = info->bitWidth ? info->bitWidth : defaultIntBits(scope);
            *isUnsigned = !info->isSigned;
            return true;
        case TYPEINFO_POINTER:
            *bits = defaultPointerBits(scope);
            *isUnsigned = true;
            return true;
        default:
            return false;
    }
}

static void commonIntegerType(ConstEvalResult lhs,
                              ConstEvalResult rhs,
                              Scope* scope,
                              unsigned* bits,
                              bool* isUnsigned) {
    unsigned leftBits = lhs.bitWidth ? lhs.bitWidth : defaultIntBits(scope);
    unsigned rightBits = rhs.bitWidth ? rhs.bitWidth : defaultIntBits(scope);
    TypeInfo leftInfo = makeIntegerType(leftBits, !lhs.isUnsigned, TOKEN_INT);
    TypeInfo rightInfo = makeIntegerType(rightBits, !rhs.isUnsigned, TOKEN_INT);
    bool ok = true;
    TypeInfo common = usualArithmeticConversion(leftInfo, rightInfo, &ok);
    unsigned outBits = defaultIntBits(scope);
    bool outUnsigned = false;
    if (ok) {
        outBits = common.bitWidth ? common.bitWidth : defaultIntBits(scope);
        outUnsigned = !common.isSigned;
    }
    if (bits) *bits = outBits;
    if (isUnsigned) *isUnsigned = outUnsigned;
}

static ConstEvalResult finalizeWithExprType(ConstEvalResult res, ASTNode* expr, Scope* scope) {
    (void)expr;
    if (!res.isConst) {
        return res;
    }
    if (res.bitWidth == 0) {
        res.bitWidth = defaultIntBits(scope);
    }
    res.value = truncateToWidth(res.value, res.bitWidth, res.isUnsigned);
    return res;
}

static TypeInfo lookupWCharType(Scope* scope) {
    TypeInfo info = makeIntegerType(defaultIntBits(scope), true, TOKEN_INT);
    if (!scope) return info;
    Symbol* w = resolveInScopeChain(scope, "wchar_t");
    if (w && w->kind == SYMBOL_TYPEDEF) {
        info = typeInfoFromParsedType(&w->type, scope);
    }
    return info;
}

static bool evalSizeof(ASTNode* target, Scope* scope, long long* out) {
    if (!target || !scope || !out) return false;

    if (target->type == AST_PARSED_TYPE) {
        return sizeFromParsedType(&target->parsedTypeNode.parsed, scope, out);
    }

    if (target->type == AST_COMPOUND_LITERAL) {
        return sizeFromParsedType(&target->compoundLiteral.literalType, scope, out);
    }

    if (target->type == AST_STRING_LITERAL) {
        const char* payload = NULL;
        LiteralEncoding enc = ast_literal_encoding(target->valueNode.value, &payload);
        const TargetLayout* tl = currentLayout(scope);
        unsigned charBits = (unsigned)((tl && tl->charBits) ? tl->charBits : 8);
        if (enc == LIT_ENC_WIDE) {
            TypeInfo wcharInfo = lookupWCharType(scope);
            unsigned wcharBits = wcharInfo.bitWidth ? wcharInfo.bitWidth : defaultIntBits(scope);
            LiteralDecodeResult res = decode_c_string_literal_wide(payload ? payload : "",
                                                                   (int)wcharBits,
                                                                   NULL,
                                                                   NULL);
            if (!res.ok) return false;
            long long wcharSize = 0;
            if (!sizeFromTypeInfo(&wcharInfo, scope, &wcharSize)) {
                wcharSize = (long long)((wcharBits + 7) / 8);
            }
            *out = (long long)(res.length + 1) * wcharSize;
            return true;
        }

        LiteralDecodeResult res = decode_c_string_literal(payload ? payload : "",
                                                          (int)charBits,
                                                          NULL,
                                                          NULL);
        if (!res.ok) return false;
        long long charSize = (long long)((charBits + 7) / 8);
        if (charSize == 0) charSize = 1;
        *out = (long long)(res.length + 1) * charSize;
        return true;
    }

    if (target->type == AST_IDENTIFIER) {
        Symbol* sym = resolveInScopeChain(scope, target->valueNode.value);
        if (sym) {
            return sizeFromParsedType(&sym->type, scope, out);
        }
    }

    TypeInfo info = analyzeExpression(target, scope);
    if ((info.category == TYPEINFO_STRUCT || info.category == TYPEINFO_UNION) && !info.isComplete) {
        return false;
    }
    return sizeFromTypeInfo(&info, scope, out);
}

static bool evalAlignof(ASTNode* target, Scope* scope, long long* out) {
    if (!target || !scope || !out) return false;
    if (target->type == AST_PARSED_TYPE) {
        return alignFromParsedType(&target->parsedTypeNode.parsed, scope, out);
    }
    if (target->type == AST_IDENTIFIER) {
        Symbol* sym = resolveInScopeChain(scope, target->valueNode.value);
        if (sym) {
            return alignFromParsedType(&sym->type, scope, out);
        }
    }
    TypeInfo info = analyzeExpression(target, scope);
    if ((info.category == TYPEINFO_STRUCT || info.category == TYPEINFO_UNION) && !info.isComplete) {
        return false;
    }
    return alignFromTypeInfo(&info, scope, out);
}

static bool evalCast(long long value,
                     const ParsedType* castType,
                     Scope* scope,
                     long long* out,
                     unsigned* bitsOut,
                     bool* isUnsignedOut) {
    if (!castType || !out) return false;
    TypeInfo info = typeInfoFromParsedType(castType, scope);
    unsigned bits = 0;
    bool isUnsigned = false;
    if (!bitsFromTypeInfo(&info, scope, &bits, &isUnsigned)) {
        return false;
    }
    *out = truncateToWidth(value, bits, isUnsigned);
    if (bitsOut) *bitsOut = bits;
    if (isUnsignedOut) *isUnsignedOut = isUnsigned;
    return true;
}

static ConstEvalResult evalBinaryOp(const char* op,
                                    ConstEvalResult lhs,
                                    ConstEvalResult rhs,
                                    ASTNode* expr,
                                    Scope* scope) {
    if (!op) return makeNonConst();

    unsigned resultBits = defaultIntBits(scope);
    bool resultUnsigned = false;

    if (strcmp(op, "|") == 0 || strcmp(op, "&") == 0 || strcmp(op, "^") == 0) {
        unsigned opBits = defaultIntBits(scope);
        bool opUnsigned = false;
        commonIntegerType(lhs, rhs, scope, &opBits, &opUnsigned);
        uint64_t l = (uint64_t)truncateToWidth(lhs.value, opBits, opUnsigned);
        uint64_t r = (uint64_t)truncateToWidth(rhs.value, opBits, opUnsigned);
        uint64_t v = 0;
        if (strcmp(op, "|") == 0) v = l | r;
        else if (strcmp(op, "&") == 0) v = l & r;
        else v = l ^ r;
        ConstEvalResult res = makeConstTyped((long long)v, opBits, opUnsigned);
        return finalizeWithExprType(res, expr, scope);
    }

    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
        strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
        strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
        unsigned opBits = defaultIntBits(scope);
        bool opUnsigned = false;
        commonIntegerType(lhs, rhs, scope, &opBits, &opUnsigned);
        long long l = truncateToWidth(lhs.value, opBits, false);
        long long r = truncateToWidth(rhs.value, opBits, false);
        uint64_t lu = (uint64_t)truncateToWidth(lhs.value, opBits, true);
        uint64_t ru = (uint64_t)truncateToWidth(rhs.value, opBits, true);
        int cmp = 0;
        if (opUnsigned) {
            if (strcmp(op, "==") == 0) cmp = (lu == ru);
            else if (strcmp(op, "!=") == 0) cmp = (lu != ru);
            else if (strcmp(op, "<") == 0) cmp = (lu < ru);
            else if (strcmp(op, "<=") == 0) cmp = (lu <= ru);
            else if (strcmp(op, ">") == 0) cmp = (lu > ru);
            else cmp = (lu >= ru);
        } else {
            if (strcmp(op, "==") == 0) cmp = (l == r);
            else if (strcmp(op, "!=") == 0) cmp = (l != r);
            else if (strcmp(op, "<") == 0) cmp = (l < r);
            else if (strcmp(op, "<=") == 0) cmp = (l <= r);
            else if (strcmp(op, ">") == 0) cmp = (l > r);
            else cmp = (l >= r);
        }
        ConstEvalResult res = makeConstTyped(cmp ? 1 : 0, defaultIntBits(scope), false);
        return finalizeWithExprType(res, expr, scope);
    }

    if (strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) {
        unsigned opBits = lhs.bitWidth ? lhs.bitWidth : defaultIntBits(scope);
        bool opUnsigned = lhs.isUnsigned;
        long long shiftAmt = rhs.value;
        if (shiftAmt < 0 || shiftAmt >= (long long)opBits) {
            return makeNonConst();
        }
        uint64_t l = (uint64_t)truncateToWidth(lhs.value, opBits, opUnsigned);
        uint64_t v = 0;
        if (strcmp(op, "<<") == 0) {
            v = l << shiftAmt;
        } else {
            if (opUnsigned) {
                v = l >> shiftAmt;
            } else {
                long long sval = truncateToWidth(lhs.value, opBits, false);
                v = (uint64_t)(sval >> shiftAmt);
            }
        }
        ConstEvalResult res = makeConstTyped((long long)v, opBits, opUnsigned);
        return finalizeWithExprType(res, expr, scope);
    }

    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
        strcmp(op, "*") == 0 || strcmp(op, "/") == 0 ||
        strcmp(op, "%") == 0) {
        if ((strcmp(op, "/") == 0 || strcmp(op, "%") == 0) && rhs.value == 0) {
            return makeNonConst();
        }
        commonIntegerType(lhs, rhs, scope, &resultBits, &resultUnsigned);
        if (resultUnsigned) {
            uint64_t l = (uint64_t)truncateToWidth(lhs.value, resultBits, true);
            uint64_t r = (uint64_t)truncateToWidth(rhs.value, resultBits, true);
            uint64_t v = 0;
            if (strcmp(op, "+") == 0) v = l + r;
            else if (strcmp(op, "-") == 0) v = l - r;
            else if (strcmp(op, "*") == 0) v = l * r;
            else if (strcmp(op, "/") == 0) v = r == 0 ? 0 : (l / r);
            else v = r == 0 ? 0 : (l % r);
            ConstEvalResult res = makeConstTyped((long long)v, resultBits, true);
            return finalizeWithExprType(res, expr, scope);
        }
        long long l = truncateToWidth(lhs.value, resultBits, false);
        long long r = truncateToWidth(rhs.value, resultBits, false);
        long long v = 0;
        if (strcmp(op, "+") == 0) v = l + r;
        else if (strcmp(op, "-") == 0) v = l - r;
        else if (strcmp(op, "*") == 0) v = l * r;
        else if (strcmp(op, "/") == 0) v = r == 0 ? 0 : (l / r);
        else v = r == 0 ? 0 : (l % r);
        ConstEvalResult res = makeConstTyped(v, resultBits, resultUnsigned);
        return finalizeWithExprType(res, expr, scope);
    }

    return makeNonConst();
}

static bool evalIdentifier(ASTNode* expr, Scope* scope, long long* out, bool allowEnumRefs) {
    if (!expr || !scope || !out) return false;
    Symbol* sym = resolveInScopeChain(scope, expr->valueNode.value);
    if (!sym) {
        return false;
    }

    if (sym->kind == SYMBOL_VARIABLE && sym->hasConstValue) {
        *out = sym->constValue;
        return true;
    }

    if (!allowEnumRefs || sym->kind != SYMBOL_ENUM || !sym->definition) {
        return false;
    }

    if (sym->hasConstValue) {
        *out = sym->constValue;
        return true;
    }

    // Find the enumerator inside the defining enum AST to derive its ordinal.
    ASTNode* enumNode = sym->definition;
    if (enumNode->type != AST_ENUM_DEFINITION) {
        return false;
    }

    long long current = 0;
    bool haveValue = false;
    for (size_t i = 0; i < enumNode->enumDef.memberCount; ++i) {
        ASTNode* member = enumNode->enumDef.members ? enumNode->enumDef.members[i] : NULL;
        if (!member || member->type != AST_IDENTIFIER) {
            continue;
        }

        if (enumNode->enumDef.values && enumNode->enumDef.values[i]) {
            long long explicitVal = 0;
            if (constEvalInteger(enumNode->enumDef.values[i], scope, &explicitVal, allowEnumRefs)) {
                current = explicitVal;
                haveValue = true;
            }
        } else if (!haveValue) {
            current = 0;
            haveValue = true;
        }

        if (strcmp(member->valueNode.value, expr->valueNode.value) == 0) {
            *out = current;
            return true;
        }
        ++current;
    }

    return false;
}

static ConstEvalResult constEvalInternal(ASTNode* expr,
                                         Scope* scope,
                                         bool allowEnumRefs) {
    if (!expr) return makeNonConst();
    switch (expr->type) {
        case AST_NUMBER_LITERAL:
        {
            long long value = 0;
            unsigned bits = defaultIntBits(scope);
            bool isUnsigned = false;
            if (!parseIntegerLiteral(expr->valueNode.value, scope, &value, &bits, &isUnsigned)) {
                return makeNonConst();
            }
            return makeConstTyped(value, bits, isUnsigned);
        }
        case AST_CHAR_LITERAL:
            if (!expr->valueNode.value) return makeNonConst();
            const char* payload = NULL;
            ast_literal_encoding(expr->valueNode.value, &payload);
            if (!payload) return makeNonConst();
            long long chVal = 0;
            unsigned bits = defaultIntBits(scope);
            bool isUnsigned = false;
            if (!parseIntegerLiteral(payload, scope, &chVal, &bits, &isUnsigned)) {
                chVal = (unsigned char)payload[0];
            }
            // Character literals have type int; wide literals use wchar_t.
            TypeInfo litType = makeIntegerType(defaultIntBits(scope), true, TOKEN_INT);
            LiteralEncoding enc = ast_literal_encoding(expr->valueNode.value, NULL);
            if (enc == LIT_ENC_WIDE && scope) {
                litType = lookupWCharType(scope);
            }
            if (enc == LIT_ENC_WIDE && (uint64_t)chVal > 0x10FFFFULL) {
                addError(expr->line, 0, "Character literal value is out of range", NULL);
                return makeNonConst();
            }
            if (litType.bitWidth) bits = litType.bitWidth;
            isUnsigned = !litType.isSigned;
            if (bits < 63) {
                uint64_t maxVal = (1ULL << bits) - 1ULL;
                if (chVal < 0 || (uint64_t)chVal > maxVal) {
                    addError(expr->line, 0, "Character literal value is out of range", NULL);
                    return makeNonConst();
                }
            }
            return makeConstTyped(chVal, bits, isUnsigned);
        case AST_IDENTIFIER:
        {
            long long idVal = 0;
            if (evalIdentifier(expr, scope, &idVal, allowEnumRefs)) {
                ConstEvalResult r = makeConst(idVal);
                return finalizeWithExprType(r, expr, scope);
            }
            return makeNonConst();
        }
        case AST_CAST_EXPRESSION: {
            long long inner = 0;
            ConstEvalResult innerRes = constEvalInternal(expr->castExpr.expression, scope, allowEnumRefs);
            if (!innerRes.isConst) {
                return makeNonConst();
            }
            unsigned bits = 0;
            bool isUnsigned = false;
            if (!evalCast(innerRes.value, &expr->castExpr.castType, scope, &inner, &bits, &isUnsigned)) {
                return makeNonConst();
            }
            return makeConstTyped(inner, bits, isUnsigned);
        }
        case AST_UNARY_EXPRESSION: {
            if (!expr->expr.left || !expr->expr.op) return makeNonConst();
            ConstEvalResult valueRes = constEvalInternal(expr->expr.left, scope, allowEnumRefs);
            if (!valueRes.isConst) {
                return makeNonConst();
            }
            unsigned bits = valueRes.bitWidth ? valueRes.bitWidth : defaultIntBits(scope);
            bool isUnsigned = valueRes.isUnsigned;
            if (strcmp(expr->expr.op, "-") == 0) {
                ConstEvalResult r = makeConstTyped(-valueRes.value, bits, isUnsigned);
                return finalizeWithExprType(r, expr, scope);
            }
            if (strcmp(expr->expr.op, "+") == 0) {
                ConstEvalResult r = makeConstTyped(valueRes.value, bits, isUnsigned);
                return finalizeWithExprType(r, expr, scope);
            }
            if (strcmp(expr->expr.op, "!") == 0) {
                unsigned boolBits = defaultIntBits(scope);
                ConstEvalResult r = makeConstTyped((valueRes.value == 0) ? 1 : 0, boolBits, false);
                return finalizeWithExprType(r, expr, scope);
            }
            if (strcmp(expr->expr.op, "~") == 0) {
                ConstEvalResult r = makeConstTyped(~valueRes.value, bits, isUnsigned);
                return finalizeWithExprType(r, expr, scope);
            }
            return makeNonConst();
        }
        case AST_BINARY_EXPRESSION: {
            if (!expr->expr.op) return makeNonConst();
            if (strcmp(expr->expr.op, "||") == 0) {
                ConstEvalResult lhs = constEvalInternal(expr->expr.left, scope, allowEnumRefs);
                if (!lhs.isConst) return makeNonConst();
                if (lhs.value != 0) return makeConst(1);
                ConstEvalResult rhs = constEvalInternal(expr->expr.right, scope, allowEnumRefs);
                if (!rhs.isConst) return makeNonConst();
                ConstEvalResult res = makeConstTyped(rhs.value != 0, defaultIntBits(scope), false);
                return finalizeWithExprType(res, expr, scope);
            }
            if (strcmp(expr->expr.op, "&&") == 0) {
                ConstEvalResult lhs = constEvalInternal(expr->expr.left, scope, allowEnumRefs);
                if (!lhs.isConst) return makeNonConst();
                if (lhs.value == 0) return makeConst(0);
                ConstEvalResult rhs = constEvalInternal(expr->expr.right, scope, allowEnumRefs);
                if (!rhs.isConst) return makeNonConst();
                ConstEvalResult res = makeConstTyped(rhs.value != 0, defaultIntBits(scope), false);
                return finalizeWithExprType(res, expr, scope);
            }

            ConstEvalResult lhs = constEvalInternal(expr->expr.left, scope, allowEnumRefs);
            ConstEvalResult rhs = constEvalInternal(expr->expr.right, scope, allowEnumRefs);
            if (!lhs.isConst || !rhs.isConst) return makeNonConst();
            return evalBinaryOp(expr->expr.op, lhs, rhs, expr, scope);
        }
        case AST_TERNARY_EXPRESSION: {
            long long cond = 0;
            ConstEvalResult condRes = constEvalInternal(expr->ternaryExpr.condition, scope, allowEnumRefs);
            if (!condRes.isConst) return makeNonConst();
            cond = condRes.value;
            ASTNode* chosen = cond ? expr->ternaryExpr.trueExpr : expr->ternaryExpr.falseExpr;
            if (!chosen) return makeNonConst();
            ConstEvalResult branch = constEvalInternal(chosen, scope, allowEnumRefs);
            if (!branch.isConst) return branch;
            return finalizeWithExprType(branch, expr, scope);
        }
        case AST_COMMA_EXPRESSION: {
            if (!expr->commaExpr.expressions || expr->commaExpr.exprCount == 0) return makeNonConst();
            ConstEvalResult value = makeNonConst();
            for (size_t i = 0; i < expr->commaExpr.exprCount; ++i) {
                value = constEvalInternal(expr->commaExpr.expressions[i], scope, allowEnumRefs);
                if (!value.isConst) {
                    return makeNonConst();
                }
            }
            return finalizeWithExprType(value, expr, scope);
        }
        case AST_SIZEOF: {
            long long sizeVal = 0;
            if (!evalSizeof(expr->expr.left, scope, &sizeVal)) {
                return makeNonConst();
            }
            unsigned bits = defaultPointerBits(scope);
            return makeConstTyped(sizeVal, bits, true);
        }
        case AST_ALIGNOF: {
            long long alignVal = 0;
            if (!evalAlignof(expr->expr.left, scope, &alignVal)) {
                return makeNonConst();
            }
            unsigned bits = defaultPointerBits(scope);
            return makeConstTyped(alignVal, bits, true);
        }
        case AST_FUNCTION_CALL: {
            if (expr->functionCall.callee &&
                expr->functionCall.callee->type == AST_IDENTIFIER) {
                const char* name = expr->functionCall.callee->valueNode.value;
                if (name &&
                    (strcmp(name, "__builtin_offsetof") == 0 ||
                     strcmp(name, "offsetof") == 0)) {
                    if (expr->functionCall.argumentCount == 2) {
                        ASTNode* typeArg = expr->functionCall.arguments[0];
                        ASTNode* fieldArg = expr->functionCall.arguments[1];
                        if (typeArg && typeArg->type == AST_PARSED_TYPE &&
                            fieldArg && fieldArg->type == AST_IDENTIFIER) {
                            size_t offset = 0;
                            if (evalOffsetofField(scope, &typeArg->parsedTypeNode.parsed,
                                                  fieldArg->valueNode.value, &offset)) {
                                return makeConstTyped((long long)offset, defaultPointerBits(scope), true);
                            }
                        }
                    }
                    return makeNonConst();
                }
            }
            return makeNonConst();
        }
        default:
            return makeNonConst();
    }
}

ConstEvalResult constEval(ASTNode* expr,
                          Scope* scope,
                          bool allowEnumRefs) {
    return constEvalInternal(expr, scope, allowEnumRefs);
}

bool constEvalInteger(ASTNode* expr,
                      Scope* scope,
                      long long* out,
                      bool allowEnumRefs) {
    ConstEvalResult res = constEvalInternal(expr, scope, allowEnumRefs);
    if (res.isConst && out) {
        *out = res.value;
    }
    return res.isConst;
}
