#include "const_eval.h"

#include "analyze_expr.h"
#include "layout.h"
#include "scope.h"
#include "symbol_table.h"
#include "type_checker.h"
#include "syntax_errors.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

static ConstEvalResult makeConst(long long value) {
    ConstEvalResult r = { true, value };
    return r;
}

static ConstEvalResult makeNonConst(void) {
    ConstEvalResult r = { false, 0 };
    return r;
}

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

static ConstEvalResult evalBinaryOp(const char* op, long long lhs, long long rhs) {
    if (!op) return makeNonConst();
    if (strcmp(op, "||") == 0) { return makeConst((lhs != 0) || (rhs != 0)); }
    if (strcmp(op, "&&") == 0) { return makeConst((lhs != 0) && (rhs != 0)); }
    if (strcmp(op, "|") == 0)  { return makeConst(clamp32(lhs | rhs)); }
    if (strcmp(op, "^") == 0)  { return makeConst(clamp32(lhs ^ rhs)); }
    if (strcmp(op, "&") == 0)  { return makeConst(clamp32(lhs & rhs)); }
    if (strcmp(op, "==") == 0) { return makeConst(lhs == rhs); }
    if (strcmp(op, "!=") == 0) { return makeConst(lhs != rhs); }
    if (strcmp(op, "<") == 0)  { return makeConst(lhs < rhs); }
    if (strcmp(op, "<=") == 0) { return makeConst(lhs <= rhs); }
    if (strcmp(op, ">") == 0)  { return makeConst(lhs > rhs); }
    if (strcmp(op, ">=") == 0) { return makeConst(lhs >= rhs); }
    if (strcmp(op, "<<") == 0) {
        if (rhs < 0 || rhs >= 63) return makeNonConst();
        return makeConst(clamp32(lhs << rhs));
    }
    if (strcmp(op, ">>") == 0) {
        if (rhs < 0 || rhs >= 63) return makeNonConst();
        return makeConst(clamp32(lhs >> rhs));
    }
    if (strcmp(op, "+") == 0) { return makeConst(clamp32(lhs + rhs)); }
    if (strcmp(op, "-") == 0) { return makeConst(clamp32(lhs - rhs)); }
    if (strcmp(op, "*") == 0) { return makeConst(clamp32(lhs * rhs)); }
    if (strcmp(op, "/") == 0) {
        if (rhs == 0) return makeNonConst();
        return makeConst(clamp32(lhs / rhs));
    }
    if (strcmp(op, "%") == 0) {
        if (rhs == 0) return makeNonConst();
        return makeConst(clamp32(lhs % rhs));
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
            if (!parseIntegerLiteral(expr->valueNode.value, &value)) {
                return makeNonConst();
            }
            return makeConst(value);
        }
        case AST_CHAR_LITERAL:
            if (!expr->valueNode.value) return makeNonConst();
            const char* payload = NULL;
            ast_literal_encoding(expr->valueNode.value, &payload);
            if (!payload) return makeNonConst();
            long long chVal = 0;
            if (!parseIntegerLiteral(payload, &chVal)) {
                chVal = (unsigned char)payload[0];
            }
            int bitWidth = 32;
            if (expr->valueNode.value[0] && expr->valueNode.value[1] && expr->valueNode.value[0] == 'W' && expr->valueNode.value[1] == '|') {
                TypeInfo w = makeIntegerType(32, true, TOKEN_INT);
                if (scope) {
                    Symbol* sym = resolveInScopeChain(scope, "wchar_t");
                    if (sym && sym->kind == SYMBOL_TYPEDEF) {
                        w = typeInfoFromParsedType(&sym->type, scope);
                    }
                }
                bitWidth = w.bitWidth ? w.bitWidth : 32;
            }
            if (bitWidth < 63) {
                uint64_t maxVal = (1ULL << bitWidth) - 1ULL;
                if (chVal < 0 || (uint64_t)chVal > maxVal) {
                    addError(expr->line, 0, "Wide character literal value is out of range for wchar_t", NULL);
                    return makeNonConst();
                }
            }
            return makeConst(chVal);
        case AST_IDENTIFIER:
        {
            long long idVal = 0;
            if (evalIdentifier(expr, scope, &idVal, allowEnumRefs)) {
                return makeConst(idVal);
            }
            return makeNonConst();
        }
        case AST_CAST_EXPRESSION: {
            long long inner = 0;
            ConstEvalResult innerRes = constEvalInternal(expr->castExpr.expression, scope, allowEnumRefs);
            if (!innerRes.isConst) {
                return makeNonConst();
            }
            if (!evalCast(innerRes.value, &expr->castExpr.castType, scope, &inner)) {
                return makeNonConst();
            }
            return makeConst(inner);
        }
        case AST_UNARY_EXPRESSION: {
            if (!expr->expr.left || !expr->expr.op) return makeNonConst();
            ConstEvalResult valueRes = constEvalInternal(expr->expr.left, scope, allowEnumRefs);
            if (!valueRes.isConst) {
                return makeNonConst();
            }
            if (strcmp(expr->expr.op, "-") == 0) {
                return makeConst(clamp32(-valueRes.value));
            }
            if (strcmp(expr->expr.op, "+") == 0) {
                return makeConst(clamp32(valueRes.value));
            }
            if (strcmp(expr->expr.op, "!") == 0) {
                return makeConst((valueRes.value == 0) ? 1 : 0);
            }
            if (strcmp(expr->expr.op, "~") == 0) {
                return makeConst(clamp32(~valueRes.value));
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
                return makeConst(rhs.value != 0);
            }
            if (strcmp(expr->expr.op, "&&") == 0) {
                ConstEvalResult lhs = constEvalInternal(expr->expr.left, scope, allowEnumRefs);
                if (!lhs.isConst) return makeNonConst();
                if (lhs.value == 0) return makeConst(0);
                ConstEvalResult rhs = constEvalInternal(expr->expr.right, scope, allowEnumRefs);
                if (!rhs.isConst) return makeNonConst();
                return makeConst(rhs.value != 0);
            }

            ConstEvalResult lhs = constEvalInternal(expr->expr.left, scope, allowEnumRefs);
            ConstEvalResult rhs = constEvalInternal(expr->expr.right, scope, allowEnumRefs);
            if (!lhs.isConst || !rhs.isConst) return makeNonConst();
            return evalBinaryOp(expr->expr.op, lhs.value, rhs.value);
        }
        case AST_TERNARY_EXPRESSION: {
            long long cond = 0;
            ConstEvalResult condRes = constEvalInternal(expr->ternaryExpr.condition, scope, allowEnumRefs);
            if (!condRes.isConst) return makeNonConst();
            cond = condRes.value;
            ASTNode* chosen = cond ? expr->ternaryExpr.trueExpr : expr->ternaryExpr.falseExpr;
            if (!chosen) return makeNonConst();
            return constEvalInternal(chosen, scope, allowEnumRefs);
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
            return value;
        }
        case AST_SIZEOF: {
            long long sizeVal = 0;
            if (!evalSizeof(expr->expr.left, scope, &sizeVal)) {
                return makeNonConst();
            }
            return makeConst(sizeVal);
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
