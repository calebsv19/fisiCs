// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/pp_expr.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    PPExprEvaluator* eval;
    const Token* tokens;
    size_t count;
    size_t cursor;
    bool error;
} PPExprParser;

typedef struct {
    uint64_t bits;
    bool isUnsigned;
} PPValue;

static PPValue pp_value_signed(int64_t value) {
    PPValue out;
    out.bits = (uint64_t)value;
    out.isUnsigned = false;
    return out;
}

static PPValue pp_value_unsigned(uint64_t value) {
    PPValue out;
    out.bits = value;
    out.isUnsigned = true;
    return out;
}

static PPValue pp_value_bool(bool value) {
    return pp_value_signed(value ? 1 : 0);
}

static int64_t pp_value_as_signed(PPValue value) {
    return (int64_t)value.bits;
}

static uint64_t pp_value_as_unsigned(PPValue value) {
    return value.bits;
}

static bool pp_value_truthy(PPValue value) {
    return value.bits != 0;
}

static bool pp_value_use_unsigned(PPValue lhs, PPValue rhs) {
    return lhs.isUnsigned || rhs.isUnsigned;
}

static const Token* parser_peek(PPExprParser* parser) {
    if (!parser || parser->cursor >= parser->count) {
        return NULL;
    }
    return &parser->tokens[parser->cursor];
}

static bool token_is_pp_identifier(const Token* tok) {
    if (!tok || !tok->value) return false;
    if (tok->type == TOKEN_IDENTIFIER) return true;
    return tok->type < TOKEN_IDENTIFIER; // keyword tokens can be macro names in PP context
}

static const Token* parser_advance(PPExprParser* parser) {
    const Token* tok = parser_peek(parser);
    if (tok && tok->type != TOKEN_EOF) {
        parser->cursor++;
    } else if (tok && tok->type == TOKEN_EOF) {
        parser->cursor = parser->count;
    }
    return tok;
}

static bool parser_match(PPExprParser* parser, TokenType type) {
    const Token* tok = parser_peek(parser);
    if (tok && tok->type == type) {
        parser_advance(parser);
        return true;
    }
    return false;
}

static void parser_consume_trailing(PPExprParser* parser) {
    while (parser->cursor < parser->count &&
           parser->tokens[parser->cursor].type == TOKEN_EOF) {
        parser->cursor++;
    }
}

static bool parser_is_done(PPExprParser* parser) {
    parser_consume_trailing(parser);
    return parser->cursor >= parser->count;
}

static PPValue parse_conditional(PPExprParser* parser, bool eval);

static PPValue parse_integer_token(const Token* tok, bool* ok) {
    if (!tok || !tok->value) {
        if (ok) *ok = false;
        return pp_value_signed(0);
    }

    bool hasUnsignedSuffix = false;
    const char* suffix = tok->value;
    while (*suffix) {
        if (*suffix == 'u' || *suffix == 'U') {
            hasUnsignedSuffix = true;
        }
        suffix++;
    }

    errno = 0;
    char* end = NULL;
    unsigned long long raw = strtoull(tok->value, &end, 0);
    if (end) {
        while (*end && isalpha((unsigned char)*end)) {
            end++;
        }
    }
    if (errno != 0 || (end && *end != '\0')) {
        if (ok) *ok = false;
        return pp_value_signed(0);
    }
    if (ok) *ok = true;
    if (hasUnsignedSuffix || raw > (unsigned long long)INT64_MAX) {
        return pp_value_unsigned((uint64_t)raw);
    }
    return pp_value_signed((int64_t)raw);
}

static PPValue parse_primary(PPExprParser* parser, bool eval) {
    const Token* tok = parser_peek(parser);
    if (!tok) {
        parser->error = true;
        return pp_value_signed(0);
    }

    if (tok->type == TOKEN_IDENTIFIER && tok->value &&
        strcmp(tok->value, "defined") == 0) {
        parser_advance(parser);
        bool hasParen = parser_match(parser, TOKEN_LPAREN);
        const Token* idTok = parser_peek(parser);
        if (!token_is_pp_identifier(idTok)) {
            parser->error = true;
            return pp_value_signed(0);
        }
        parser_advance(parser);
        if (hasParen && !parser_match(parser, TOKEN_RPAREN)) {
            parser->error = true;
            return pp_value_signed(0);
        }
        if (!eval) {
            return pp_value_signed(0);
        }
        bool isDefined = false;
        if (parser->eval && parser->eval->macros) {
            isDefined = macro_table_lookup(parser->eval->macros, idTok->value) != NULL;
        }
        return pp_value_bool(isDefined);
    }

    switch (tok->type) {
        case TOKEN_NUMBER:
        case TOKEN_CHAR_LITERAL: {
            parser_advance(parser);
            if (!eval) return pp_value_signed(0);
            bool ok = false;
            PPValue value = parse_integer_token(tok, &ok);
            if (!ok) {
                parser->error = true;
            }
            return value;
        }
        case TOKEN_IDENTIFIER:
            parser_advance(parser);
            return pp_value_signed(0);
        case TOKEN_LPAREN: {
            parser_advance(parser);
            PPValue value = parse_conditional(parser, eval);
            if (!parser_match(parser, TOKEN_RPAREN)) {
                parser->error = true;
            }
            return eval ? value : pp_value_signed(0);
        }
        default:
            parser->error = true;
            return pp_value_signed(0);
    }
}

static PPValue parse_unary(PPExprParser* parser, bool eval) {
    const Token* tok = parser_peek(parser);
    if (tok) {
        switch (tok->type) {
            case TOKEN_PLUS:
                parser_advance(parser);
                return parse_unary(parser, eval);
            case TOKEN_MINUS: {
                parser_advance(parser);
                PPValue operand = parse_unary(parser, eval);
                if (!eval) return pp_value_signed(0);
                if (operand.isUnsigned) {
                    return pp_value_unsigned((uint64_t)(0 - operand.bits));
                }
                return pp_value_signed(-pp_value_as_signed(operand));
            }
            case TOKEN_LOGICAL_NOT: {
                parser_advance(parser);
                PPValue operand = parse_unary(parser, eval);
                return eval ? pp_value_bool(!pp_value_truthy(operand)) : pp_value_signed(0);
            }
            case TOKEN_BITWISE_NOT: {
                parser_advance(parser);
                PPValue operand = parse_unary(parser, eval);
                if (!eval) return pp_value_signed(0);
                if (operand.isUnsigned) {
                    return pp_value_unsigned(~operand.bits);
                }
                return pp_value_signed(~pp_value_as_signed(operand));
            }
            default:
                break;
        }
    }
    return parse_primary(parser, eval);
}

static PPValue parse_multiplicative(PPExprParser* parser, bool eval) {
    PPValue value = parse_unary(parser, eval);
    while (!parser->error) {
        const Token* tok = parser_peek(parser);
        if (!tok) break;
        if (tok->type != TOKEN_ASTERISK &&
            tok->type != TOKEN_DIVIDE &&
            tok->type != TOKEN_MODULO) {
            break;
        }
        TokenType op = tok->type;
        parser_advance(parser);
        PPValue rhs = parse_unary(parser, eval);
        if (eval) {
            if ((op == TOKEN_DIVIDE || op == TOKEN_MODULO) && !pp_value_truthy(rhs)) {
                parser->error = true;
                value = pp_value_signed(0);
                continue;
            }
            bool useUnsigned = pp_value_use_unsigned(value, rhs);
            switch (op) {
                case TOKEN_ASTERISK:
                    value = useUnsigned
                        ? pp_value_unsigned(pp_value_as_unsigned(value) * pp_value_as_unsigned(rhs))
                        : pp_value_signed(pp_value_as_signed(value) * pp_value_as_signed(rhs));
                    break;
                case TOKEN_DIVIDE:
                    value = useUnsigned
                        ? pp_value_unsigned(pp_value_as_unsigned(value) / pp_value_as_unsigned(rhs))
                        : pp_value_signed(pp_value_as_signed(value) / pp_value_as_signed(rhs));
                    break;
                case TOKEN_MODULO:
                    value = useUnsigned
                        ? pp_value_unsigned(pp_value_as_unsigned(value) % pp_value_as_unsigned(rhs))
                        : pp_value_signed(pp_value_as_signed(value) % pp_value_as_signed(rhs));
                    break;
                default:
                    break;
            }
        }
    }
    return eval ? value : pp_value_signed(0);
}

static PPValue parse_additive(PPExprParser* parser, bool eval) {
    PPValue value = parse_multiplicative(parser, eval);
    while (!parser->error) {
        const Token* tok = parser_peek(parser);
        if (!tok) break;
        if (tok->type != TOKEN_PLUS && tok->type != TOKEN_MINUS) {
            break;
        }
        TokenType op = tok->type;
        parser_advance(parser);
        PPValue rhs = parse_multiplicative(parser, eval);
        if (!eval) {
            continue;
        }
        bool useUnsigned = pp_value_use_unsigned(value, rhs);
        if (op == TOKEN_PLUS) {
            value = useUnsigned
                ? pp_value_unsigned(pp_value_as_unsigned(value) + pp_value_as_unsigned(rhs))
                : pp_value_signed(pp_value_as_signed(value) + pp_value_as_signed(rhs));
        } else {
            value = useUnsigned
                ? pp_value_unsigned(pp_value_as_unsigned(value) - pp_value_as_unsigned(rhs))
                : pp_value_signed(pp_value_as_signed(value) - pp_value_as_signed(rhs));
        }
    }
    return eval ? value : pp_value_signed(0);
}

static PPValue parse_shift(PPExprParser* parser, bool eval) {
    PPValue value = parse_additive(parser, eval);
    while (!parser->error) {
        const Token* tok = parser_peek(parser);
        if (!tok) break;
        if (tok->type != TOKEN_LEFT_SHIFT && tok->type != TOKEN_RIGHT_SHIFT) {
            break;
        }
        TokenType op = tok->type;
        parser_advance(parser);
        PPValue rhs = parse_additive(parser, eval);
        if (!eval) {
            continue;
        }
        unsigned int amount = (unsigned int)(pp_value_as_unsigned(rhs) & 63u);
        if (op == TOKEN_LEFT_SHIFT) {
            if (value.isUnsigned) {
                value = pp_value_unsigned(pp_value_as_unsigned(value) << amount);
            } else {
                value = pp_value_signed(pp_value_as_signed(value) << amount);
            }
        } else {
            if (value.isUnsigned) {
                value = pp_value_unsigned(pp_value_as_unsigned(value) >> amount);
            } else {
                value = pp_value_signed(pp_value_as_signed(value) >> amount);
            }
        }
    }
    return eval ? value : pp_value_signed(0);
}

static PPValue parse_relational(PPExprParser* parser, bool eval) {
    PPValue value = parse_shift(parser, eval);
    while (!parser->error) {
        const Token* tok = parser_peek(parser);
        if (!tok) break;
        switch (tok->type) {
            case TOKEN_LESS:
            case TOKEN_LESS_EQUAL:
            case TOKEN_GREATER:
            case TOKEN_GREATER_EQUAL: {
                TokenType op = tok->type;
                parser_advance(parser);
                PPValue rhs = parse_shift(parser, eval);
                if (eval) {
                    bool result = false;
                    bool useUnsigned = pp_value_use_unsigned(value, rhs);
                    switch (op) {
                        case TOKEN_LESS:
                            result = useUnsigned
                                ? (pp_value_as_unsigned(value) < pp_value_as_unsigned(rhs))
                                : (pp_value_as_signed(value) < pp_value_as_signed(rhs));
                            break;
                        case TOKEN_LESS_EQUAL:
                            result = useUnsigned
                                ? (pp_value_as_unsigned(value) <= pp_value_as_unsigned(rhs))
                                : (pp_value_as_signed(value) <= pp_value_as_signed(rhs));
                            break;
                        case TOKEN_GREATER:
                            result = useUnsigned
                                ? (pp_value_as_unsigned(value) > pp_value_as_unsigned(rhs))
                                : (pp_value_as_signed(value) > pp_value_as_signed(rhs));
                            break;
                        case TOKEN_GREATER_EQUAL:
                            result = useUnsigned
                                ? (pp_value_as_unsigned(value) >= pp_value_as_unsigned(rhs))
                                : (pp_value_as_signed(value) >= pp_value_as_signed(rhs));
                            break;
                        default: break;
                    }
                    value = pp_value_bool(result);
                }
                break;
            }
            default:
                goto done_relational;
        }
    }
done_relational:
    return eval ? value : pp_value_signed(0);
}

static PPValue parse_equality(PPExprParser* parser, bool eval) {
    PPValue value = parse_relational(parser, eval);
    while (!parser->error) {
        const Token* tok = parser_peek(parser);
        if (!tok) break;
        if (tok->type != TOKEN_EQUAL && tok->type != TOKEN_NOT_EQUAL) {
            break;
        }
        TokenType op = tok->type;
        parser_advance(parser);
        PPValue rhs = parse_relational(parser, eval);
        if (eval) {
            bool useUnsigned = pp_value_use_unsigned(value, rhs);
            bool equal = useUnsigned
                ? (pp_value_as_unsigned(value) == pp_value_as_unsigned(rhs))
                : (pp_value_as_signed(value) == pp_value_as_signed(rhs));
            value = pp_value_bool((op == TOKEN_EQUAL) ? equal : !equal);
        }
    }
    return eval ? value : pp_value_signed(0);
}

static PPValue parse_bitwise_and(PPExprParser* parser, bool eval) {
    PPValue value = parse_equality(parser, eval);
    while (!parser->error && parser_match(parser, TOKEN_BITWISE_AND)) {
        PPValue rhs = parse_equality(parser, eval);
        if (eval) {
            bool useUnsigned = pp_value_use_unsigned(value, rhs);
            uint64_t bits = pp_value_as_unsigned(value) & pp_value_as_unsigned(rhs);
            value = useUnsigned ? pp_value_unsigned(bits) : pp_value_signed((int64_t)bits);
        }
    }
    return eval ? value : pp_value_signed(0);
}

static PPValue parse_bitwise_xor(PPExprParser* parser, bool eval) {
    PPValue value = parse_bitwise_and(parser, eval);
    while (!parser->error && parser_match(parser, TOKEN_BITWISE_XOR)) {
        PPValue rhs = parse_bitwise_and(parser, eval);
        if (eval) {
            bool useUnsigned = pp_value_use_unsigned(value, rhs);
            uint64_t bits = pp_value_as_unsigned(value) ^ pp_value_as_unsigned(rhs);
            value = useUnsigned ? pp_value_unsigned(bits) : pp_value_signed((int64_t)bits);
        }
    }
    return eval ? value : pp_value_signed(0);
}

static PPValue parse_bitwise_or(PPExprParser* parser, bool eval) {
    PPValue value = parse_bitwise_xor(parser, eval);
    while (!parser->error && parser_match(parser, TOKEN_BITWISE_OR)) {
        PPValue rhs = parse_bitwise_xor(parser, eval);
        if (eval) {
            bool useUnsigned = pp_value_use_unsigned(value, rhs);
            uint64_t bits = pp_value_as_unsigned(value) | pp_value_as_unsigned(rhs);
            value = useUnsigned ? pp_value_unsigned(bits) : pp_value_signed((int64_t)bits);
        }
    }
    return eval ? value : pp_value_signed(0);
}

static PPValue parse_logical_and(PPExprParser* parser, bool eval) {
    PPValue value = parse_bitwise_or(parser, eval);
    while (!parser->error && parser_match(parser, TOKEN_LOGICAL_AND)) {
        bool evalRight = eval && pp_value_truthy(value);
        PPValue rhs = parse_bitwise_or(parser, evalRight);
        if (eval) {
            value = pp_value_bool(pp_value_truthy(value) && pp_value_truthy(rhs));
        }
    }
    return eval ? value : pp_value_signed(0);
}

static PPValue parse_logical_or(PPExprParser* parser, bool eval) {
    PPValue value = parse_logical_and(parser, eval);
    while (!parser->error && parser_match(parser, TOKEN_LOGICAL_OR)) {
        bool evalRight = eval && !pp_value_truthy(value);
        PPValue rhs = parse_logical_and(parser, evalRight);
        if (eval) {
            value = pp_value_bool(pp_value_truthy(value) || pp_value_truthy(rhs));
        }
    }
    return eval ? value : pp_value_signed(0);
}

static PPValue parse_conditional(PPExprParser* parser, bool eval) {
    PPValue condition = parse_logical_or(parser, eval);
    const Token* tok = parser_peek(parser);
    if (!tok || tok->type != TOKEN_QUESTION) {
        return eval ? condition : pp_value_signed(0);
    }
    parser_advance(parser);
    bool evalTrue = eval && pp_value_truthy(condition);
    PPValue trueValue = parse_conditional(parser, evalTrue);
    if (!parser_match(parser, TOKEN_COLON)) {
        parser->error = true;
        return pp_value_signed(0);
    }
    bool evalFalse = eval && !pp_value_truthy(condition);
    PPValue falseValue = parse_conditional(parser, evalFalse);
    if (!eval) {
        return pp_value_signed(0);
    }
    return pp_value_truthy(condition) ? trueValue : falseValue;
}

void pp_expr_evaluator_init(PPExprEvaluator* eval, MacroTable* table) {
    if (!eval) return;
    eval->macros = table;
}

void pp_expr_evaluator_reset(PPExprEvaluator* eval) {
    if (!eval) return;
    eval->macros = NULL;
}

bool pp_expr_eval_tokens(PPExprEvaluator* eval,
                         const Token* tokens,
                         size_t count,
                         int32_t* outValue) {
    if (!tokens || count == 0) {
        if (outValue) *outValue = 0;
        return false;
    }
    PPExprParser parser;
    parser.eval = eval;
    parser.tokens = tokens;
    parser.count = count;
    parser.cursor = 0;
    parser.error = false;

    PPValue value = parse_conditional(&parser, true);
    bool ok = !parser.error;
    ok = ok && parser_is_done(&parser);

    if (ok && outValue) {
        *outValue = (int32_t)(value.isUnsigned ? value.bits : (uint64_t)pp_value_as_signed(value));
    }
    if (!ok && outValue) {
        *outValue = 0;
    }
    return ok;
}

bool pp_expr_eval_range(PPExprEvaluator* eval,
                        const TokenBuffer* buffer,
                        size_t start,
                        size_t end,
                        int32_t* outValue) {
    if (!buffer || start > buffer->count || start > end || end > buffer->count) {
        if (outValue) *outValue = 0;
        return false;
    }
    size_t length = end - start;
    if (length == 0) {
        if (outValue) *outValue = 0;
        return false;
    }
    return pp_expr_eval_tokens(eval, buffer->tokens + start, length, outValue);
}
