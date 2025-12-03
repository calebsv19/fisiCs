#include "Preprocessor/preprocessor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Lexer/tokens.h"

static void pp_token_buffer_init_local(PPTokenBuffer* buffer) {
    if (!buffer) return;
    buffer->tokens = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
}

static bool pp_token_buffer_reserve(PPTokenBuffer* buffer, size_t extra) {
    if (!buffer) return false;
    size_t required = buffer->count + extra;
    if (required <= buffer->capacity) {
        return true;
    }
    size_t newCapacity = buffer->capacity ? buffer->capacity * 2 : 16;
    while (newCapacity < required) {
        newCapacity *= 2;
    }
    Token* tokens = (Token*)realloc(buffer->tokens, newCapacity * sizeof(Token));
    if (!tokens) {
        return false;
    }
    buffer->tokens = tokens;
    buffer->capacity = newCapacity;
    return true;
}

static char* pp_strdup_local(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = (char*)malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}

static void pp_token_free(Token* tok) {
    if (!tok) return;
    free(tok->value);
    tok->value = NULL;
}

static Token pp_token_clone(const Token* tok) {
    Token clone = *tok;
    if (tok->value) {
        clone.value = pp_strdup_local(tok->value);
    }
    return clone;
}

static bool pp_token_buffer_append_token(PPTokenBuffer* buffer, Token token) {
    if (!pp_token_buffer_reserve(buffer, 1)) {
        pp_token_free(&token);
        return false;
    }
    buffer->tokens[buffer->count++] = token;
    return true;
}

static bool pp_token_buffer_append_clone(PPTokenBuffer* buffer, const Token* tok) {
    Token clone = pp_token_clone(tok);
    if (!pp_token_buffer_append_token(buffer, clone)) {
        pp_token_free(&clone);
        return false;
    }
    return true;
}

static bool pp_token_buffer_append_buffer(PPTokenBuffer* dest, const PPTokenBuffer* src) {
    if (!dest || !src) return true;
    for (size_t i = 0; i < src->count; ++i) {
        if (!pp_token_buffer_append_clone(dest, &src->tokens[i])) {
            return false;
        }
    }
    return true;
}

static void pp_token_buffer_reset(PPTokenBuffer* buffer) {
    if (!buffer) return;
    for (size_t i = 0; i < buffer->count; ++i) {
        pp_token_free(&buffer->tokens[i]);
    }
    free(buffer->tokens);
    buffer->tokens = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
}

typedef struct {
    char** names;
    size_t count;
    size_t capacity;
    bool variadic;
    bool hasVaOpt;
} MacroParamParse;

static void macro_param_parse_destroy(MacroParamParse* params) {
    if (!params) return;
    for (size_t i = 0; i < params->count; ++i) {
        free(params->names[i]);
    }
    free(params->names);
    params->names = NULL;
    params->count = 0;
    params->capacity = 0;
    params->variadic = false;
    params->hasVaOpt = false;
}

static bool macro_param_append(MacroParamParse* params, const char* name) {
    if (!params) return false;
    if (params->count == params->capacity) {
        size_t newCapacity = params->capacity ? params->capacity * 2 : 4;
        char** names = (char**)realloc(params->names, newCapacity * sizeof(char*));
        if (!names) return false;
        params->names = names;
        params->capacity = newCapacity;
    }
    params->names[params->count] = pp_strdup_local(name);
    if (!params->names[params->count]) {
        return false;
    }
    params->count++;
    return true;
}

static bool tokens_are_adjacent(const Token* left, const Token* right) {
    if (!left || !right) return false;
    if (left->location.start.line != right->location.start.line) {
        return false;
    }
    size_t leftLen = left->value ? strlen(left->value) : 0;
    int expectedColumn = left->location.start.column + (int)leftLen;
    return right->location.start.column == expectedColumn;
}

static bool parse_macro_parameters(const Token* tokens,
                                   size_t count,
                                   size_t* cursor,
                                   MacroParamParse* params) {
    size_t i = *cursor;
    if (i >= count || tokens[i].type != TOKEN_LPAREN) {
        return false;
    }
    i++; // consume '('
    bool expectParam = true;
    bool variadicDetected = false;

    while (i < count) {
        const Token* tok = &tokens[i];
        if (tok->type == TOKEN_RPAREN) {
            i++;
            break;
        }
        if (!expectParam) {
            if (tok->type != TOKEN_COMMA) {
                return false;
            }
            expectParam = true;
            i++;
            continue;
        }
        if (tok->type == TOKEN_IDENTIFIER) {
            if (!macro_param_append(params, tok->value)) {
                return false;
            }
            expectParam = false;
            i++;
            continue;
        }
        if (tok->type == TOKEN_ELLIPSIS) {
            params->variadic = true;
            variadicDetected = true;
            expectParam = false;
            i++;
            continue;
        }
        return false;
    }

    if (i > count) {
        return false;
    }
    if (expectParam && !variadicDetected) {
        return false;
    }
    *cursor = i;
    return true;
}

static bool collect_macro_body(const Token* tokens,
                               size_t count,
                               size_t* cursor,
                               int directiveLine,
                               PPTokenBuffer* body) {
    size_t i = *cursor;
    while (i < count) {
        const Token* tok = &tokens[i];
        if (tok->type == TOKEN_EOF) {
            break;
        }
        if (tok->line != directiveLine) {
            break;
        }
        if (!pp_token_buffer_append_clone(body, tok)) {
            return false;
        }
        i++;
    }
    *cursor = i;
    return true;
}

static bool flush_chunk(Preprocessor* pp,
                        PPTokenBuffer* chunk,
                        PPTokenBuffer* output) {
    if (!chunk || chunk->count == 0) {
        return true;
    }
    PPTokenBuffer expanded = {0};
    if (!macro_expander_expand(&pp->expander, chunk->tokens, chunk->count, &expanded)) {
        pp_token_buffer_destroy(&expanded);
        return false;
    }
    bool ok = pp_token_buffer_append_buffer(output, &expanded);
    pp_token_buffer_destroy(&expanded);
    pp_token_buffer_destroy(chunk);
    pp_token_buffer_init_local(chunk);
    return ok;
}

static bool process_define(Preprocessor* pp,
                           const Token* tokens,
                           size_t count,
                           size_t* cursor) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    i++;
    if (i >= count || tokens[i].type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Preprocessor error: expected identifier after #define\n");
        return false;
    }
    const Token* nameTok = &tokens[i];
    i++;

    bool isFunction = false;
    MacroParamParse params = {0};
    PPTokenBuffer body = {0};
    bool ok = false;

    if (i < count &&
        tokens[i].type == TOKEN_LPAREN &&
        tokens[i].line == nameTok->line &&
        tokens_are_adjacent(nameTok, &tokens[i])) {
        isFunction = true;
        if (!parse_macro_parameters(tokens, count, &i, &params)) {
            fprintf(stderr, "Preprocessor error: invalid parameter list in #define %s\n", nameTok->value);
            goto cleanup;
        }
    }

    if (!collect_macro_body(tokens, count, &i, directiveLine, &body)) {
        fprintf(stderr, "Preprocessor error: failed to collect macro body for %s\n", nameTok->value);
        goto cleanup;
    }

    if (isFunction) {
        ok = macro_table_define_function(pp->table,
                                         nameTok->value,
                                         (const char* const*)params.names,
                                         params.count,
                                         params.variadic,
                                         params.hasVaOpt,
                                         body.tokens,
                                         body.count,
                                         tokens[*cursor].location);
    } else {
        ok = macro_table_define_object(pp->table,
                                       nameTok->value,
                                       body.tokens,
                                       body.count,
                                       tokens[*cursor].location);
    }

    if (!ok) {
        fprintf(stderr, "Preprocessor error: failed to record macro %s\n", nameTok->value);
        goto cleanup;
    }

    *cursor = (i == 0) ? 0 : i - 1;
    cleanup:
    macro_param_parse_destroy(&params);
    pp_token_buffer_reset(&body);
    return ok;
}

static bool process_undef(Preprocessor* pp,
                          const Token* tokens,
                          size_t count,
                          size_t* cursor) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    i++;
    if (i >= count || tokens[i].type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Preprocessor error: expected identifier after #undef\n");
        return false;
    }
    macro_table_undef(pp->table, tokens[i].value);
    i++;
    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
        i++;
    }
    *cursor = (i == 0) ? 0 : i - 1;
    return true;
}

bool preprocessor_init(Preprocessor* pp, bool preserveDirectives) {
    if (!pp) return false;
    pp->table = macro_table_create();
    if (!pp->table) {
        return false;
    }
    macro_expander_init(&pp->expander, pp->table);
    pp->preserveDirectives = preserveDirectives;
    return true;
}

void preprocessor_destroy(Preprocessor* pp) {
    if (!pp) return;
    macro_table_destroy(pp->table);
    pp->table = NULL;
    macro_expander_reset(&pp->expander);
}

MacroTable* preprocessor_get_macro_table(Preprocessor* pp) {
    return pp ? pp->table : NULL;
}

bool preprocessor_run(Preprocessor* pp,
                      const TokenBuffer* input,
                      PPTokenBuffer* output) {
    if (!pp || !input || !output) return false;
    pp_token_buffer_init_local(output);
    PPTokenBuffer chunk = {0};

    for (size_t i = 0; i < input->count; ++i) {
        const Token* tok = &input->tokens[i];
        switch (tok->type) {
            case TOKEN_DEFINE:
                if (!flush_chunk(pp, &chunk, output)) {
                    pp_token_buffer_reset(&chunk);
                    return false;
                }
                if (!process_define(pp, input->tokens, input->count, &i)) {
                    pp_token_buffer_reset(&chunk);
                    return false;
                }
                break;
            case TOKEN_UNDEF:
                if (!flush_chunk(pp, &chunk, output)) {
                    pp_token_buffer_reset(&chunk);
                    return false;
                }
                if (!process_undef(pp, input->tokens, input->count, &i)) {
                    pp_token_buffer_reset(&chunk);
                    return false;
                }
                break;
            default:
                if (!pp_token_buffer_append_clone(&chunk, tok)) {
                    pp_token_buffer_reset(&chunk);
                    return false;
                }
                break;
        }
    }

    bool flushed = flush_chunk(pp, &chunk, output);
    pp_token_buffer_reset(&chunk);
    return flushed;
}
