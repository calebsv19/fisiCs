#include "Preprocessor/pp_expr.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    PPExprEvaluator* eval;
    const Token* tokens;
    size_t count;
    size_t cursor;
    bool error;
} PPExprParser;

static int32_t clamp32(int64_t value) {
    return (int32_t)value;
}

static const Token* parser_peek(PPExprParser* parser) {
    if (!parser || parser->cursor >= parser->count) {
        return NULL;
    }
    return &parser->tokens[parser->cursor];
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

static int32_t parse_conditional(PPExprParser* parser, bool eval);

static int32_t parse_integer_token(const Token* tok, bool* ok) {
    if (!tok || !tok->value) {
        if (ok) *ok = false;
        return 0;
    }
    errno = 0;
    char* end = NULL;
    long long value = strtoll(tok->value, &end, 0);
    if (end) {
        while (*end && isalpha((unsigned char)*end)) {
            end++;
        }
    }
    if (errno != 0 || (end && *end != '\0')) {
        if (ok) *ok = false;
        return 0;
    }
    if (ok) *ok = true;
    return clamp32(value);
}

static int32_t parse_primary(PPExprParser* parser, bool eval) {
    const Token* tok = parser_peek(parser);
    if (!tok) {
        parser->error = true;
        return 0;
    }

    if (tok->type == TOKEN_IDENTIFIER && tok->value &&
        strcmp(tok->value, "defined") == 0) {
        parser_advance(parser);
        bool hasParen = parser_match(parser, TOKEN_LPAREN);
        const Token* idTok = parser_peek(parser);
        if (!idTok || idTok->type != TOKEN_IDENTIFIER) {
            parser->error = true;
            return 0;
        }
        parser_advance(parser);
        if (hasParen && !parser_match(parser, TOKEN_RPAREN)) {
            parser->error = true;
            return 0;
        }
        if (!eval) {
            return 0;
        }
        bool isDefined = false;
        if (parser->eval && parser->eval->macros) {
            isDefined = macro_table_lookup(parser->eval->macros, idTok->value) != NULL;
        }
        return isDefined ? 1 : 0;
    }

    switch (tok->type) {
        case TOKEN_NUMBER:
        case TOKEN_CHAR_LITERAL: {
            parser_advance(parser);
            if (!eval) return 0;
            bool ok = false;
            int32_t value = parse_integer_token(tok, &ok);
            if (!ok) {
                parser->error = true;
            }
            return value;
        }
        case TOKEN_TRUE:
            parser_advance(parser);
            return eval ? 1 : 0;
        case TOKEN_FALSE:
            parser_advance(parser);
            return 0;
        case TOKEN_IDENTIFIER:
            parser_advance(parser);
            return 0;
        case TOKEN_LPAREN: {
            parser_advance(parser);
            int32_t value = parse_conditional(parser, eval);
            if (!parser_match(parser, TOKEN_RPAREN)) {
                parser->error = true;
            }
            return eval ? value : 0;
        }
        default:
            parser->error = true;
            return 0;
    }
}

static int32_t parse_unary(PPExprParser* parser, bool eval) {
    const Token* tok = parser_peek(parser);
    if (tok) {
        switch (tok->type) {
            case TOKEN_PLUS:
                parser_advance(parser);
                return parse_unary(parser, eval);
            case TOKEN_MINUS: {
                parser_advance(parser);
                int32_t operand = parse_unary(parser, eval);
                return eval ? clamp32(-(int64_t)operand) : 0;
            }
            case TOKEN_LOGICAL_NOT: {
                parser_advance(parser);
                int32_t operand = parse_unary(parser, eval);
                return eval ? (operand ? 0 : 1) : 0;
            }
            case TOKEN_BITWISE_NOT: {
                parser_advance(parser);
                int32_t operand = parse_unary(parser, eval);
                return eval ? ~operand : 0;
            }
            default:
                break;
        }
    }
    return parse_primary(parser, eval);
}

static int32_t parse_multiplicative(PPExprParser* parser, bool eval) {
    int32_t value = parse_unary(parser, eval);
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
        int32_t rhs = parse_unary(parser, eval);
        if (eval) {
            if ((op == TOKEN_DIVIDE || op == TOKEN_MODULO) && rhs == 0) {
                parser->error = true;
                value = 0;
                continue;
            }
            switch (op) {
                case TOKEN_ASTERISK:
                    value = clamp32((int64_t)value * rhs);
                    break;
                case TOKEN_DIVIDE:
                    value = clamp32(value / rhs);
                    break;
                case TOKEN_MODULO:
                    value = clamp32(value % rhs);
                    break;
                default:
                    break;
            }
        }
    }
    return eval ? value : 0;
}

static int32_t parse_additive(PPExprParser* parser, bool eval) {
    int32_t value = parse_multiplicative(parser, eval);
    while (!parser->error) {
        const Token* tok = parser_peek(parser);
        if (!tok) break;
        if (tok->type != TOKEN_PLUS && tok->type != TOKEN_MINUS) {
            break;
        }
        TokenType op = tok->type;
        parser_advance(parser);
        int32_t rhs = parse_multiplicative(parser, eval);
        if (!eval) {
            continue;
        }
        if (op == TOKEN_PLUS) {
            value = clamp32((int64_t)value + rhs);
        } else {
            value = clamp32((int64_t)value - rhs);
        }
    }
    return eval ? value : 0;
}

static int32_t parse_shift(PPExprParser* parser, bool eval) {
    int32_t value = parse_additive(parser, eval);
    while (!parser->error) {
        const Token* tok = parser_peek(parser);
        if (!tok) break;
        if (tok->type != TOKEN_LEFT_SHIFT && tok->type != TOKEN_RIGHT_SHIFT) {
            break;
        }
        TokenType op = tok->type;
        parser_advance(parser);
        int32_t rhs = parse_additive(parser, eval);
        if (!eval) {
            continue;
        }
        unsigned int amount = (unsigned int)(rhs & 31);
        if (op == TOKEN_LEFT_SHIFT) {
            value = clamp32((int64_t)value << amount);
        } else {
            value = clamp32((int64_t)value >> amount);
        }
    }
    return eval ? value : 0;
}

static int32_t parse_relational(PPExprParser* parser, bool eval) {
    int32_t value = parse_shift(parser, eval);
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
                int32_t rhs = parse_shift(parser, eval);
                if (eval) {
                    switch (op) {
                        case TOKEN_LESS: value = (value < rhs) ? 1 : 0; break;
                        case TOKEN_LESS_EQUAL: value = (value <= rhs) ? 1 : 0; break;
                        case TOKEN_GREATER: value = (value > rhs) ? 1 : 0; break;
                        case TOKEN_GREATER_EQUAL: value = (value >= rhs) ? 1 : 0; break;
                        default: break;
                    }
                }
                break;
            }
            default:
                goto done_relational;
        }
    }
done_relational:
    return eval ? value : 0;
}

static int32_t parse_equality(PPExprParser* parser, bool eval) {
    int32_t value = parse_relational(parser, eval);
    while (!parser->error) {
        const Token* tok = parser_peek(parser);
        if (!tok) break;
        if (tok->type != TOKEN_EQUAL && tok->type != TOKEN_NOT_EQUAL) {
            break;
        }
        TokenType op = tok->type;
        parser_advance(parser);
        int32_t rhs = parse_relational(parser, eval);
        if (eval) {
            value = (op == TOKEN_EQUAL) ? ((value == rhs) ? 1 : 0)
                                        : ((value != rhs) ? 1 : 0);
        }
    }
    return eval ? value : 0;
}

static int32_t parse_bitwise_and(PPExprParser* parser, bool eval) {
    int32_t value = parse_equality(parser, eval);
    while (!parser->error && parser_match(parser, TOKEN_BITWISE_AND)) {
        int32_t rhs = parse_equality(parser, eval);
        if (eval) {
            value = value & rhs;
        }
    }
    return eval ? value : 0;
}

static int32_t parse_bitwise_xor(PPExprParser* parser, bool eval) {
    int32_t value = parse_bitwise_and(parser, eval);
    while (!parser->error && parser_match(parser, TOKEN_BITWISE_XOR)) {
        int32_t rhs = parse_bitwise_and(parser, eval);
        if (eval) {
            value = value ^ rhs;
        }
    }
    return eval ? value : 0;
}

static int32_t parse_bitwise_or(PPExprParser* parser, bool eval) {
    int32_t value = parse_bitwise_xor(parser, eval);
    while (!parser->error && parser_match(parser, TOKEN_BITWISE_OR)) {
        int32_t rhs = parse_bitwise_xor(parser, eval);
        if (eval) {
            value = value | rhs;
        }
    }
    return eval ? value : 0;
}

static int32_t parse_logical_and(PPExprParser* parser, bool eval) {
    int32_t value = parse_bitwise_or(parser, eval);
    while (!parser->error && parser_match(parser, TOKEN_LOGICAL_AND)) {
        bool evalRight = eval && value != 0;
        int32_t rhs = parse_bitwise_or(parser, evalRight);
        if (eval) {
            value = (value && rhs) ? 1 : 0;
        }
    }
    return eval ? value : 0;
}

static int32_t parse_logical_or(PPExprParser* parser, bool eval) {
    int32_t value = parse_logical_and(parser, eval);
    while (!parser->error && parser_match(parser, TOKEN_LOGICAL_OR)) {
        bool evalRight = eval && value == 0;
        int32_t rhs = parse_logical_and(parser, evalRight);
        if (eval) {
            value = (value || rhs) ? 1 : 0;
        }
    }
    return eval ? value : 0;
}

static int32_t parse_conditional(PPExprParser* parser, bool eval) {
    int32_t condition = parse_logical_or(parser, eval);
    const Token* tok = parser_peek(parser);
    if (!tok || tok->type != TOKEN_QUESTION) {
        return eval ? condition : 0;
    }
    parser_advance(parser);
    bool evalTrue = eval && condition != 0;
    int32_t trueValue = parse_conditional(parser, evalTrue);
    if (!parser_match(parser, TOKEN_COLON)) {
        parser->error = true;
        return 0;
    }
    bool evalFalse = eval && condition == 0;
    int32_t falseValue = parse_conditional(parser, evalFalse);
    if (!eval) {
        return 0;
    }
    return condition ? trueValue : falseValue;
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

    int32_t value = parse_conditional(&parser, true);
    bool ok = !parser.error;
    ok = ok && parser_is_done(&parser);

    if (ok && outValue) {
        *outValue = value;
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
