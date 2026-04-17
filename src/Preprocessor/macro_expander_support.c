// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/macro_expander_internal.h"

#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#endif

static void safe_free_token_value(char* value) {
    if (!value) return;
#if defined(__APPLE__)
    if (malloc_zone_from_ptr(value) == NULL) {
        return;
    }
#endif
    free(value);
}

SourceRange empty_range(void) {
    SourceRange range;
    range.start.file = NULL;
    range.start.line = 0;
    range.start.column = 0;
    range.end = range.start;
    return range;
}

void macro_expander_clear_error(MacroExpander* expander) {
    if (!expander) return;
    expander->lastError.kind = ME_ERR_NONE;
    expander->lastError.macro = NULL;
    expander->lastError.callSite = empty_range();
    expander->lastError.expectedArgs = 0;
    expander->lastError.providedArgs = 0;
    expander->lastError.variadic = false;
}

void macro_expander_set_arg_count_error(MacroExpander* expander,
                                        const MacroDefinition* def,
                                        SourceRange callSite,
                                        size_t expectedArgs,
                                        size_t providedArgs) {
    if (!expander) return;
    expander->lastError.kind = ME_ERR_MACRO_ARG_COUNT;
    expander->lastError.macro = def;
    expander->lastError.callSite = callSite;
    expander->lastError.expectedArgs = expectedArgs;
    expander->lastError.providedArgs = providedArgs;
    expander->lastError.variadic = def ? def->params.variadic : false;
}

char* pp_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = (char*)malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}

void token_free(Token* tok) {
    if (!tok) return;
    safe_free_token_value(tok->value);
    tok->value = NULL;
}

Token token_clone(const Token* tok) {
    Token copy = *tok;
    if (tok->value) {
        copy.value = pp_strdup(tok->value);
    }
    return copy;
}

bool range_is_initialized(const SourceRange* range) {
    return range && (range->start.file || range->start.line || range->start.column);
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
    Token* newTokens = (Token*)realloc(buffer->tokens, newCapacity * sizeof(Token));
    if (!newTokens) {
        return false;
    }
    buffer->tokens = newTokens;
    buffer->capacity = newCapacity;
    return true;
}

bool pp_token_buffer_append(PPTokenBuffer* buffer, Token token) {
    if (!pp_token_buffer_reserve(buffer, 1)) {
        token_free(&token);
        return false;
    }
    buffer->tokens[buffer->count++] = token;
    return true;
}

void pp_token_buffer_move(PPTokenBuffer* dest, PPTokenBuffer* src) {
    if (!dest || !src) return;
    *dest = *src;
    src->tokens = NULL;
    src->count = 0;
    src->capacity = 0;
}

void pp_token_buffer_release(PPTokenBuffer* buffer) {
    if (!buffer) return;
    for (size_t i = 0; i < buffer->count; ++i) {
        token_free(&buffer->tokens[i]);
    }
    free(buffer->tokens);
    buffer->tokens = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
}

bool pp_token_buffer_pop_if_type(PPTokenBuffer* buffer, TokenType type) {
    if (!buffer || buffer->count == 0) {
        return false;
    }
    Token* last = &buffer->tokens[buffer->count - 1];
    if (last->type != type) {
        return false;
    }
    token_free(last);
    buffer->count--;
    return true;
}

void pp_token_buffer_destroy(PPTokenBuffer* buffer) {
    pp_token_buffer_release(buffer);
}

void pp_argument_list_init(PPArgumentList* list) {
    if (!list) return;
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void pp_argument_list_destroy(PPArgumentList* list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; ++i) {
        pp_token_buffer_release(&list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static bool pp_argument_list_reserve(PPArgumentList* list, size_t extra) {
    if (!list) return false;
    size_t required = list->count + extra;
    if (required <= list->capacity) {
        return true;
    }
    size_t newCapacity = list->capacity ? list->capacity * 2 : 4;
    while (newCapacity < required) {
        newCapacity *= 2;
    }
    PPTokenBuffer* newItems = (PPTokenBuffer*)realloc(list->items, newCapacity * sizeof(PPTokenBuffer));
    if (!newItems) {
        return false;
    }
    list->items = newItems;
    list->capacity = newCapacity;
    return true;
}

static bool pp_argument_list_append(PPArgumentList* list, PPTokenBuffer* buffer) {
    if (!pp_argument_list_reserve(list, 1)) {
        return false;
    }
    list->items[list->count] = *buffer;
    list->count++;
    buffer->tokens = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
    return true;
}

void macro_arg_init(MacroArg* arg) {
    if (!arg) return;
    arg->raw.tokens = NULL;
    arg->raw.count = 0;
    arg->raw.capacity = 0;
    arg->expanded.tokens = NULL;
    arg->expanded.count = 0;
    arg->expanded.capacity = 0;
}

void macro_arg_destroy(MacroArg* arg) {
    if (!arg) return;
    pp_token_buffer_release(&arg->raw);
    pp_token_buffer_release(&arg->expanded);
}

static void macro_arg_pack_init(MacroArgPack* pack) {
    if (!pack) return;
    pack->positionals = NULL;
    pack->positionalCount = 0;
    pack->variadics = NULL;
    pack->variadicCount = 0;
}

void macro_arg_pack_destroy(MacroArgPack* pack) {
    if (!pack) return;
    for (size_t i = 0; i < pack->positionalCount; ++i) {
        macro_arg_destroy(&pack->positionals[i]);
    }
    for (size_t i = 0; i < pack->variadicCount; ++i) {
        macro_arg_destroy(&pack->variadics[i]);
    }
    free(pack->positionals);
    free(pack->variadics);
    pack->positionals = NULL;
    pack->variadics = NULL;
    pack->positionalCount = 0;
    pack->variadicCount = 0;
}

bool parse_macro_argument_tokens(const Token* input,
                                 size_t totalCount,
                                 size_t* cursor,
                                 PPArgumentList* outArgs,
                                 const MacroDefinition* def) {
    size_t idx = *cursor;
    int depth = 0;
    bool hitClosing = false;
    PPTokenBuffer current = {0};
    pp_argument_list_init(outArgs);

    while (idx < totalCount) {
        const Token* tok = &input[idx];
        if (tok->type == TOKEN_RPAREN && depth == 0) {
            hitClosing = true;
            idx++;
            break;
        }

        if (tok->type == TOKEN_COMMA && depth == 0) {
            if (!pp_argument_list_append(outArgs, &current)) {
                pp_token_buffer_release(&current);
                pp_argument_list_destroy(outArgs);
                return false;
            }
            memset(&current, 0, sizeof(current));
            idx++;
            continue;
        }

        if (tok->type == TOKEN_LPAREN) {
            depth++;
        } else if (tok->type == TOKEN_RPAREN) {
            depth--;
        }

        if (!pp_token_buffer_append_clone(&current, tok)) {
            pp_token_buffer_release(&current);
            pp_argument_list_destroy(outArgs);
            return false;
        }
        idx++;
    }

    if (!hitClosing) {
        pp_token_buffer_release(&current);
        pp_argument_list_destroy(outArgs);
        return false;
    }

    bool needFinal = (current.count > 0) ||
                     (outArgs->count > 0) ||
                     def->params.count > 0 ||
                     def->params.variadic;
    if (needFinal) {
        if (!pp_argument_list_append(outArgs, &current)) {
            pp_token_buffer_release(&current);
            pp_argument_list_destroy(outArgs);
            return false;
        }
    } else {
        pp_token_buffer_release(&current);
    }

    *cursor = idx;
    return true;
}

bool build_macro_arg_pack(MacroExpander* expander,
                          const MacroDefinition* def,
                          PPArgumentList* rawArgs,
                          MacroArgPack* pack,
                          SourceRange callSite) {
    macro_arg_pack_init(pack);
    size_t expected = def->params.count;
    size_t provided = rawArgs->count;
    bool singleEmptyArg = (provided == 1 &&
                           rawArgs->items &&
                           rawArgs->items[0].count == 0);

    if (expected > 0 && singleEmptyArg) {
        provided = 0;
    }

    if (!def->params.variadic) {
        if (provided != expected) {
            macro_expander_set_arg_count_error(expander, def, callSite, expected, provided);
            macro_arg_pack_destroy(pack);
            return false;
        }
    } else if (provided < expected) {
        macro_expander_set_arg_count_error(expander, def, callSite, expected, provided);
        macro_arg_pack_destroy(pack);
        return false;
    }

    if (expected > 0) {
        pack->positionals = (MacroArg*)calloc(expected, sizeof(MacroArg));
        if (!pack->positionals) {
            macro_arg_pack_destroy(pack);
            return false;
        }
        pack->positionalCount = expected;
        for (size_t i = 0; i < expected; ++i) {
            macro_arg_init(&pack->positionals[i]);
            if (i >= rawArgs->count) {
                macro_arg_pack_destroy(pack);
                return false;
            }
            pp_token_buffer_move(&pack->positionals[i].raw, &rawArgs->items[i]);
        }
    }

    size_t extra = (provided > expected) ? (provided - expected) : 0;
    if (extra > 0) {
        pack->variadics = (MacroArg*)calloc(extra, sizeof(MacroArg));
        if (!pack->variadics) {
            macro_arg_pack_destroy(pack);
            return false;
        }
        pack->variadicCount = extra;
        for (size_t j = 0; j < extra; ++j) {
            macro_arg_init(&pack->variadics[j]);
            pp_token_buffer_move(&pack->variadics[j].raw, &rawArgs->items[expected + j]);
        }
    }

    return true;
}

static bool expand_macro_argument(MacroExpander* expander,
                                  const PPTokenBuffer* input,
                                  PPTokenBuffer* output) {
    if (!input || input->count == 0) {
        output->tokens = NULL;
        output->count = 0;
        output->capacity = 0;
        return true;
    }
    PPTokenBuffer temp = {0};
    if (!macro_expander_expand(expander, input->tokens, input->count, &temp)) {
        pp_token_buffer_release(&temp);
        return false;
    }
    *output = temp;
    return true;
}

bool expand_macro_arguments(MacroExpander* expander, MacroArgPack* pack) {
    for (size_t i = 0; i < pack->positionalCount; ++i) {
        if (!expand_macro_argument(expander, &pack->positionals[i].raw, &pack->positionals[i].expanded)) {
            return false;
        }
    }
    for (size_t i = 0; i < pack->variadicCount; ++i) {
        if (!expand_macro_argument(expander, &pack->variadics[i].raw, &pack->variadics[i].expanded)) {
            return false;
        }
    }
    return true;
}
