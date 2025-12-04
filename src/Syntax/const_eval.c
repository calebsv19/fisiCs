#include "const_eval.h"

#include "scope.h"
#include "symbol_table.h"
#include "type_checker.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

static long long clamp32(long long value) {
    return (long long)(int32_t)value;
}

static bool parseIntegerLiteral(const char* text, long long* out) {
    if (!text || !out) return false;
    char* endptr = NULL;
    long long value = strtoll(text, &endptr, 0);
    if (endptr == text) {
        return false;
    }
    while (endptr && *endptr) {
        char c = *endptr;
        if (c == 'u' || c == 'U' || c == 'l' || c == 'L') {
            ++endptr;
            continue;
        }
        return false;
    }
    *out = value;
    return true;
}

static bool evalCast(long long value, const ParsedType* castType, Scope* scope, long long* out) {
    if (!castType || !out) return false;
    TypeInfo info = typeInfoFromParsedType(castType, scope);
    if (info.category != TYPEINFO_INTEGER && info.category != TYPEINFO_ENUM && info.category != TYPEINFO_POINTER) {
        return false;
    }

    unsigned bits = info.bitWidth ? info.bitWidth : 32;
    if (bits >= 63) {
        *out = value;
        return true;
    }

    unsigned long long mask = (bits == 64) ? ~0ULL : ((1ULL << bits) - 1ULL);
    unsigned long long masked = ((unsigned long long)value) & mask;
    if (info.isSigned && bits > 0) {
        unsigned long long signBit = 1ULL << (bits - 1);
        if (masked & signBit) {
            unsigned long long extendMask = ~((1ULL << bits) - 1ULL);
            masked |= extendMask;
        }
    }
    *out = (long long)masked;
    return true;
}

static bool evalBinaryOp(const char* op, long long lhs, long long rhs, long long* out) {
    if (!op || !out) return false;
    if (strcmp(op, "||") == 0) { *out = (lhs != 0) || (rhs != 0); return true; }
    if (strcmp(op, "&&") == 0) { *out = (lhs != 0) && (rhs != 0); return true; }
    if (strcmp(op, "|") == 0)  { *out = clamp32(lhs | rhs); return true; }
    if (strcmp(op, "^") == 0)  { *out = clamp32(lhs ^ rhs); return true; }
    if (strcmp(op, "&") == 0)  { *out = clamp32(lhs & rhs); return true; }
    if (strcmp(op, "==") == 0) { *out = lhs == rhs; return true; }
    if (strcmp(op, "!=") == 0) { *out = lhs != rhs; return true; }
    if (strcmp(op, "<") == 0)  { *out = lhs < rhs; return true; }
    if (strcmp(op, "<=") == 0) { *out = lhs <= rhs; return true; }
    if (strcmp(op, ">") == 0)  { *out = lhs > rhs; return true; }
    if (strcmp(op, ">=") == 0) { *out = lhs >= rhs; return true; }
    if (strcmp(op, "<<") == 0) {
        if (rhs < 0 || rhs >= 63) return false;
        *out = clamp32(lhs << rhs);
        return true;
    }
    if (strcmp(op, ">>") == 0) {
        if (rhs < 0 || rhs >= 63) return false;
        *out = clamp32(lhs >> rhs);
        return true;
    }
    if (strcmp(op, "+") == 0) { *out = clamp32(lhs + rhs); return true; }
    if (strcmp(op, "-") == 0) { *out = clamp32(lhs - rhs); return true; }
    if (strcmp(op, "*") == 0) { *out = clamp32(lhs * rhs); return true; }
    if (strcmp(op, "/") == 0) {
        if (rhs == 0) return false;
        *out = clamp32(lhs / rhs);
        return true;
    }
    if (strcmp(op, "%") == 0) {
        if (rhs == 0) return false;
        *out = clamp32(lhs % rhs);
        return true;
    }
    return false;
}

static bool evalIdentifier(ASTNode* expr, Scope* scope, long long* out, bool allowEnumRefs) {
    if (!expr || !scope || !out || !allowEnumRefs) return false;
    Symbol* sym = resolveInScopeChain(scope, expr->valueNode.value);
    if (!sym || sym->kind != SYMBOL_ENUM || !sym->definition) {
        return false;
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

bool constEvalInteger(ASTNode* expr,
                      Scope* scope,
                      long long* out,
                      bool allowEnumRefs) {
    if (!expr || !out) return false;

    switch (expr->type) {
        case AST_NUMBER_LITERAL:
            return parseIntegerLiteral(expr->valueNode.value, out);
        case AST_CHAR_LITERAL:
            if (!expr->valueNode.value) return false;
            *out = (unsigned char)expr->valueNode.value[0];
            return true;
        case AST_IDENTIFIER:
            return evalIdentifier(expr, scope, out, allowEnumRefs);
        case AST_CAST_EXPRESSION: {
            long long inner = 0;
            if (!constEvalInteger(expr->castExpr.expression, scope, &inner, allowEnumRefs)) {
                return false;
            }
            return evalCast(inner, &expr->castExpr.castType, scope, out);
        }
        case AST_UNARY_EXPRESSION: {
            if (!expr->expr.left || !expr->expr.op) return false;
            long long value = 0;
            if (!constEvalInteger(expr->expr.left, scope, &value, allowEnumRefs)) {
                return false;
            }
            if (strcmp(expr->expr.op, "-") == 0) {
                *out = clamp32(-value);
                return true;
            }
            if (strcmp(expr->expr.op, "+") == 0) {
                *out = clamp32(value);
                return true;
            }
            if (strcmp(expr->expr.op, "!") == 0) {
                *out = (value == 0) ? 1 : 0;
                return true;
            }
            if (strcmp(expr->expr.op, "~") == 0) {
                *out = clamp32(~value);
                return true;
            }
            return false;
        }
        case AST_BINARY_EXPRESSION: {
            if (!expr->expr.op) return false;
            if (strcmp(expr->expr.op, "||") == 0) {
                long long lhs = 0;
                if (!constEvalInteger(expr->expr.left, scope, &lhs, allowEnumRefs)) return false;
                if (lhs != 0) { *out = 1; return true; }
                long long rhs = 0;
                if (!constEvalInteger(expr->expr.right, scope, &rhs, allowEnumRefs)) return false;
                *out = (rhs != 0);
                return true;
            }
            if (strcmp(expr->expr.op, "&&") == 0) {
                long long lhs = 0;
                if (!constEvalInteger(expr->expr.left, scope, &lhs, allowEnumRefs)) return false;
                if (lhs == 0) { *out = 0; return true; }
                long long rhs = 0;
                if (!constEvalInteger(expr->expr.right, scope, &rhs, allowEnumRefs)) return false;
                *out = (rhs != 0);
                return true;
            }

            long long lhs = 0;
            long long rhs = 0;
            if (!constEvalInteger(expr->expr.left, scope, &lhs, allowEnumRefs)) return false;
            if (!constEvalInteger(expr->expr.right, scope, &rhs, allowEnumRefs)) return false;
            return evalBinaryOp(expr->expr.op, lhs, rhs, out);
        }
        case AST_TERNARY_EXPRESSION: {
            long long cond = 0;
            if (!constEvalInteger(expr->ternaryExpr.condition, scope, &cond, allowEnumRefs)) return false;
            ASTNode* chosen = cond ? expr->ternaryExpr.trueExpr : expr->ternaryExpr.falseExpr;
            if (!chosen) return false;
            return constEvalInteger(chosen, scope, out, allowEnumRefs);
        }
        case AST_COMMA_EXPRESSION: {
            if (!expr->commaExpr.expressions || expr->commaExpr.exprCount == 0) return false;
            long long value = 0;
            for (size_t i = 0; i < expr->commaExpr.exprCount; ++i) {
                if (!constEvalInteger(expr->commaExpr.expressions[i], scope, &value, allowEnumRefs)) {
                    return false;
                }
            }
            *out = value;
            return true;
        }
        case AST_SIZEOF: {
            *out = (long long)sizeof(void*);
            return true;
        }
        default:
            return false;
    }
}
