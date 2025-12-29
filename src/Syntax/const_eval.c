#include "const_eval.h"

#include "analyze_expr.h"
#include "layout.h"
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

static uint64_t maxUnsignedForBits(unsigned bits) {
    if (bits == 0 || bits >= 64) return UINT64_MAX;
    return (1ULL << bits) - 1ULL;
}

static uint64_t maxSignedForBits(unsigned bits) {
    if (bits == 0 || bits >= 64) return INT64_MAX;
    return (1ULL << (bits - 1)) - 1ULL;
}

static bool fitsInBits(uint64_t value, unsigned bits, bool isUnsigned) {
    if (isUnsigned) {
        return value <= maxUnsignedForBits(bits);
    }
    return value <= maxSignedForBits(bits);
}

static bool parseIntegerLiteral(const char* text,
                                Scope* scope,
                                long long* out,
                                unsigned* bitsOut,
                                bool* isUnsignedOut) {
    if (!text || !out) return false;

    char* endptr = NULL;
    unsigned long long raw = strtoull(text, &endptr, 0);
    if (endptr == text) {
        return false;
    }

    bool hasU = false;
    int lCount = 0;
    for (const char* p = endptr; p && *p; ++p) {
        if (*p == 'u' || *p == 'U') {
            hasU = true;
            continue;
        }
        if (*p == 'l' || *p == 'L') {
            ++lCount;
            continue;
        }
        // Unrecognised suffix -> treat as invalid for const folding.
        return false;
    }
    if (lCount > 2) lCount = 2;

    int base = 10;
    if (text[0] == '0') {
        if (text[1] == 'x' || text[1] == 'X') base = 16;
        else base = 8;
    }

    const TargetLayout* tl = currentLayout(scope);
    unsigned intBits = (unsigned)((tl && tl->intBits) ? tl->intBits : 32);
    unsigned longBits = (unsigned)((tl && tl->longBits) ? tl->longBits : 64);
    unsigned longLongBits = (unsigned)((tl && tl->longLongBits) ? tl->longLongBits : 64);

    typedef struct { unsigned bits; bool isUnsigned; } Candidate;
    Candidate cands[8];
    int candCount = 0;
    #define ADD(bitsVal, unsVal) do { cands[candCount].bits = (bitsVal); cands[candCount].isUnsigned = (unsVal); candCount++; } while(0)

    if (hasU) {
        if (lCount >= 2) {
            ADD(longLongBits, true);
        } else if (lCount == 1) {
            ADD(longBits, true);
            ADD(longLongBits, true);
        } else {
            ADD(intBits, true);
            ADD(longBits, true);
            ADD(longLongBits, true);
        }
    } else if (lCount >= 2) {
        ADD(longLongBits, false);
        ADD(longLongBits, true);
    } else if (lCount == 1) {
        ADD(longBits, false);
        ADD(longLongBits, false);
        ADD(longLongBits, true);
    } else {
        if (base == 10) {
            ADD(intBits, false);
            ADD(longBits, false);
            ADD(longLongBits, false);
            ADD(longLongBits, true);
        } else {
            ADD(intBits, false);
            ADD(intBits, true);
            ADD(longBits, false);
            ADD(longBits, true);
            ADD(longLongBits, false);
            ADD(longLongBits, true);
        }
    }
    #undef ADD

    if (candCount == 0) return false;

    int chosen = candCount - 1;
    for (int i = 0; i < candCount; ++i) {
        if (fitsInBits(raw, cands[i].bits, cands[i].isUnsigned)) {
            chosen = i;
            break;
        }
    }

    unsigned bits = cands[chosen].bits;
    bool isUnsigned = cands[chosen].isUnsigned;
    uint64_t masked = raw;
    if (bits > 0 && bits < 64) {
        uint64_t mask = (1ULL << bits) - 1ULL;
        masked &= mask;
        if (!isUnsigned) {
            uint64_t sign = 1ULL << (bits - 1);
            if (masked & sign) {
                masked |= ~mask;
            }
        }
    }
    long long val = (long long)masked;

    if (out) *out = val;
    if (bitsOut) *bitsOut = bits;
    if (isUnsignedOut) *isUnsignedOut = isUnsigned;
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
        unsigned opBits = lhs.bitWidth ? lhs.bitWidth : rhs.bitWidth;
        if (opBits == 0) opBits = defaultIntBits(scope);
        bool opUnsigned = lhs.isUnsigned || rhs.isUnsigned;
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
        long long l = lhs.value;
        long long r = rhs.value;
        int cmp = 0;
        if (strcmp(op, "==") == 0) cmp = (l == r);
        else if (strcmp(op, "!=") == 0) cmp = (l != r);
        else if (strcmp(op, "<") == 0) cmp = (l < r);
        else if (strcmp(op, "<=") == 0) cmp = (l <= r);
        else if (strcmp(op, ">") == 0) cmp = (l > r);
        else cmp = (l >= r);
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
        long long v = 0;
        if (strcmp(op, "+") == 0) v = lhs.value + rhs.value;
        else if (strcmp(op, "-") == 0) v = lhs.value - rhs.value;
        else if (strcmp(op, "*") == 0) v = lhs.value * rhs.value;
        else if (strcmp(op, "/") == 0) v = lhs.value / rhs.value;
        else v = lhs.value % rhs.value;
        ConstEvalResult res = makeConstTyped(v, resultBits, resultUnsigned);
        return finalizeWithExprType(res, expr, scope);
    }

    return makeNonConst();
}

static bool evalIdentifier(ASTNode* expr, Scope* scope, long long* out, bool allowEnumRefs) {
    if (!expr || !scope || !out || !allowEnumRefs) return false;
    Symbol* sym = resolveInScopeChain(scope, expr->valueNode.value);
    if (!sym || sym->kind != SYMBOL_ENUM || !sym->definition) {
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
