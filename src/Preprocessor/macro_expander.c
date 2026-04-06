// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/macro_expander.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#if defined(__APPLE__)
#include <malloc/malloc.h>
#endif

#include "Lexer/lexer.h"

typedef struct {
    PPTokenBuffer* items;
    size_t count;
    size_t capacity;
} PPArgumentList;

typedef struct {
    PPTokenBuffer raw;
    PPTokenBuffer expanded;
} MacroArg;

typedef struct {
    MacroArg* positionals;
    size_t positionalCount;
    MacroArg* variadics;
    size_t variadicCount;
} MacroArgPack;

static SourceRange empty_range(void);
static void macro_expander_clear_error(MacroExpander* expander);
static void macro_expander_set_arg_count_error(MacroExpander* expander,
                                               const MacroDefinition* def,
                                               SourceRange callSite,
                                               size_t expectedArgs,
                                               size_t providedArgs);

static void safe_free_token_value(char* value) {
    if (!value) return;
#if defined(__APPLE__)
    if (malloc_zone_from_ptr(value) == NULL) {
        return;
    }
#endif
    free(value);
}

static char* pp_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = (char*)malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}

static void token_free(Token* tok) {
    if (!tok) return;
    safe_free_token_value(tok->value);
    tok->value = NULL;
}

static Token token_clone(const Token* tok) {
    Token copy = *tok;
    if (tok->value) {
        copy.value = pp_strdup(tok->value);
    }
    return copy;
}

static bool range_is_initialized(const SourceRange* range) {
    return range && (range->start.file || range->start.line || range->start.column);
}

static bool token_is_defined_operand_context(const Token* tokens, size_t index) {
    if (!tokens) return false;
    if (index > 0 &&
        tokens[index - 1].type == TOKEN_IDENTIFIER &&
        tokens[index - 1].value &&
        strcmp(tokens[index - 1].value, "defined") == 0) {
        return true;
    }
    if (index > 1 &&
        tokens[index - 1].type == TOKEN_LPAREN &&
        tokens[index - 2].type == TOKEN_IDENTIFIER &&
        tokens[index - 2].value &&
        strcmp(tokens[index - 2].value, "defined") == 0) {
        return true;
    }
    return false;
}

static SourceRange empty_range(void) {
    SourceRange range;
    range.start.file = NULL;
    range.start.line = 0;
    range.start.column = 0;
    range.end = range.start;
    return range;
}

static void macro_expander_clear_error(MacroExpander* expander) {
    if (!expander) return;
    expander->lastError.kind = ME_ERR_NONE;
    expander->lastError.macro = NULL;
    expander->lastError.callSite = empty_range();
    expander->lastError.expectedArgs = 0;
    expander->lastError.providedArgs = 0;
    expander->lastError.variadic = false;
}

static void macro_expander_set_arg_count_error(MacroExpander* expander,
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

static bool pp_token_buffer_append(PPTokenBuffer* buffer, Token token) {
    if (!pp_token_buffer_reserve(buffer, 1)) {
        token_free(&token);
        return false;
    }
    buffer->tokens[buffer->count++] = token;
    return true;
}

static bool pp_token_buffer_append_clone(PPTokenBuffer* buffer, const Token* tok) {
    Token clone = token_clone(tok);
    if (!pp_token_buffer_append(buffer, clone)) {
        token_free(&clone);
        return false;
    }
    return true;
}

static void pp_token_buffer_move(PPTokenBuffer* dest, PPTokenBuffer* src) {
    if (!dest || !src) return;
    *dest = *src;
    src->tokens = NULL;
    src->count = 0;
    src->capacity = 0;
}

static void pp_token_buffer_release(PPTokenBuffer* buffer) {
    if (!buffer) return;
    for (size_t i = 0; i < buffer->count; ++i) {
        token_free(&buffer->tokens[i]);
    }
    free(buffer->tokens);
    buffer->tokens = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
}

void pp_token_buffer_destroy(PPTokenBuffer* buffer) {
    pp_token_buffer_release(buffer);
}

void macro_expander_set_builtins(MacroExpander* expander, PPBuiltinState state) {
    if (!expander) return;
    expander->builtins = state;
}

static void pp_argument_list_init(PPArgumentList* list) {
    if (!list) return;
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void pp_argument_list_destroy(PPArgumentList* list) {
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

static void macro_arg_init(MacroArg* arg) {
    if (!arg) return;
    arg->raw.tokens = NULL;
    arg->raw.count = 0;
    arg->raw.capacity = 0;
    arg->expanded.tokens = NULL;
    arg->expanded.count = 0;
    arg->expanded.capacity = 0;
}

static void macro_arg_destroy(MacroArg* arg) {
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

static void macro_arg_pack_destroy(MacroArgPack* pack) {
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

static bool parse_macro_argument_tokens(const Token* input,
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

static bool build_macro_arg_pack(MacroExpander* expander,
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

    // Distinguish "empty invocation" from "one explicit argument":
    // F() with required named parameters should count as 0 provided args.
    if (expected > 0 && singleEmptyArg) {
        provided = 0;
    }

    if (!def->params.variadic) {
        if (provided != expected) {
            macro_expander_set_arg_count_error(expander, def, callSite, expected, provided);
            macro_arg_pack_destroy(pack);
            return false;
        }
    } else {
        if (provided < expected) {
            macro_expander_set_arg_count_error(expander, def, callSite, expected, provided);
            macro_arg_pack_destroy(pack);
            return false;
        }
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

static bool emit_builtin_literal(PPTokenBuffer* dest,
                                 const Token* tok,
                                 TokenType type,
                                 const char* text) {
    Token clone = token_clone(tok);
    clone.type = type;
    if (clone.value) free(clone.value);
    clone.value = text ? pp_strdup(text) : NULL;
    clone.macroDefinition = empty_range();
    clone.macroCallSite = tok ? tok->location : empty_range();
    if (!pp_token_buffer_append(dest, clone)) {
        token_free(&clone);
        return false;
    }
    return true;
}

static bool handle_builtin_identifier(MacroExpander* expander,
                                      const Token* tok,
                                      PPTokenBuffer* out) {
    if (!tok || tok->type != TOKEN_IDENTIFIER || !tok->value) return false;
    const char* name = tok->value;
    const SourceRange* loc = range_is_initialized(&tok->macroCallSite)
        ? &tok->macroCallSite
        : &tok->location;
    if (strcmp(name, "__FILE__") == 0) {
        const char* file = loc->start.file ? loc->start.file : expander->builtins.baseFile;
        if (expander->builtins.lineRemapActive &&
            *expander->builtins.lineRemapActive &&
            expander->builtins.logicalFile &&
            *expander->builtins.logicalFile) {
            file = *expander->builtins.logicalFile;
        }
        return emit_builtin_literal(out, tok, TOKEN_STRING, file ? file : "");
    }
    if (strcmp(name, "__LINE__") == 0) {
        char buffer[32];
        int line = loc->start.line;
        if (expander->builtins.lineRemapActive &&
            *expander->builtins.lineRemapActive &&
            expander->builtins.lineOffset) {
            line += *expander->builtins.lineOffset;
            if (line < 1) line = 1;
        }
        snprintf(buffer, sizeof(buffer), "%d", line);
        return emit_builtin_literal(out, tok, TOKEN_NUMBER, buffer);
    }
    if (strcmp(name, "__BASE_FILE__") == 0) {
        const char* file = expander->builtins.baseFile;
        return emit_builtin_literal(out, tok, TOKEN_STRING, file ? file : "");
    }
    if (strcmp(name, "__COUNTER__") == 0) {
        int value = expander->builtins.counter ? *expander->builtins.counter : 0;
        if (expander->builtins.counter) {
            (*expander->builtins.counter)++;
        }
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", value);
        return emit_builtin_literal(out, tok, TOKEN_NUMBER, buffer);
    }
    if (strcmp(name, "__DATE__") == 0) {
        const char* date = expander->builtins.dateString ? expander->builtins.dateString : "";
        return emit_builtin_literal(out, tok, TOKEN_STRING, date);
    }
    if (strcmp(name, "__TIME__") == 0) {
        const char* timeStr = expander->builtins.timeString ? expander->builtins.timeString : "";
        return emit_builtin_literal(out, tok, TOKEN_STRING, timeStr);
    }
    return false;
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

static bool expand_macro_arguments(MacroExpander* expander, MacroArgPack* pack) {
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

static ssize_t macro_param_index(const MacroDefinition* def, const char* name) {
    if (!def || !name) return -1;
    for (size_t i = 0; i < def->params.count; ++i) {
        if (def->params.names && def->params.names[i] &&
            strcmp(def->params.names[i], name) == 0) {
            return (ssize_t)i;
        }
    }
    return -1;
}

static Token make_simple_token(TokenType type, const char* text, SourceRange origin) {
    Token tok;
    tok.type = type;
    tok.value = text ? pp_strdup(text) : NULL;
    tok.line = origin.start.line;
    tok.location = origin;
    tok.macroCallSite = empty_range();
    tok.macroDefinition = empty_range();
    return tok;
}

static bool append_argument_tokens(PPTokenBuffer* dest, const PPTokenBuffer* arg) {
    if (!dest || !arg) return true;
    for (size_t i = 0; i < arg->count; ++i) {
        if (!pp_token_buffer_append_clone(dest, &arg->tokens[i])) {
            return false;
        }
    }
    return true;
}

static bool append_variadic_tokens(PPTokenBuffer* dest,
                                   const MacroArgPack* pack,
                                   bool useRaw,
                                   SourceRange origin) {
    if (!pack || pack->variadicCount == 0) {
        return true;
    }
    bool first = true;
    for (size_t i = 0; i < pack->variadicCount; ++i) {
        const PPTokenBuffer* buf = useRaw
            ? &pack->variadics[i].raw
            : &pack->variadics[i].expanded;
        if (!first) {
            Token comma = make_simple_token(TOKEN_COMMA, ",", origin);
            if (!pp_token_buffer_append(dest, comma)) {
                token_free(&comma);
                return false;
            }
        }
        if (!append_argument_tokens(dest, buf)) {
            return false;
        }
        first = false;
    }
    return true;
}

static const char* token_text(const Token* tok) {
    return (tok && tok->value) ? tok->value : "";
}

static bool append_stringified_argument(PPTokenBuffer* dest,
                                        const MacroArg* arg,
                                        SourceRange origin) {
    size_t totalLen = 0;
    if (arg) {
        for (size_t i = 0; i < arg->raw.count; ++i) {
            const char* text = token_text(&arg->raw.tokens[i]);
            size_t len = strlen(text);
            size_t extraEscapes = 0;
            for (size_t j = 0; j < len; ++j) {
                if (text[j] == '\\' || text[j] == '"') {
                    extraEscapes++;
                }
            }
            totalLen += len + extraEscapes;
            if (i + 1 < arg->raw.count) {
                totalLen += 1; // space
            }
        }
    }
    char* buffer = (char*)malloc(totalLen + 1);
    if (!buffer) {
        return false;
    }
    char* ptr = buffer;
    if (arg) {
        for (size_t i = 0; i < arg->raw.count; ++i) {
            if (i > 0) {
                *ptr++ = ' ';
            }
            const char* text = token_text(&arg->raw.tokens[i]);
            for (const char* c = text; *c; ++c) {
                if (*c == '\\' || *c == '"') {
                    *ptr++ = '\\';
                }
                *ptr++ = *c;
            }
        }
    }
    *ptr = '\0';

    Token stringTok = make_simple_token(TOKEN_STRING, buffer, origin);
    free(buffer);
    return pp_token_buffer_append(dest, stringTok);
}

static bool stringify_variadic_arguments(PPTokenBuffer* dest,
                                         const MacroArgPack* pack,
                                         SourceRange origin) {
    MacroArg temp;
    macro_arg_init(&temp);

    if (pack && pack->variadicCount > 0) {
        for (size_t i = 0; i < pack->variadicCount; ++i) {
            if (temp.raw.count > 0) {
                Token comma = make_simple_token(TOKEN_COMMA, ",", origin);
                if (!pp_token_buffer_append(&temp.raw, comma)) {
                    token_free(&comma);
                    macro_arg_destroy(&temp);
                    return false;
                }
            }
            if (!append_argument_tokens(&temp.raw, &pack->variadics[i].raw)) {
                macro_arg_destroy(&temp);
                return false;
            }
        }
    }

    bool ok = append_stringified_argument(dest, &temp, origin);
    macro_arg_destroy(&temp);
    return ok;
}

static bool relex_single_token(const char* text, SourceRange origin, Token* out) {
    if (!text || !out) return false;
    Lexer lexer;
    initLexer(&lexer, text, "<macro paste>", false);
    Token first = getNextToken(&lexer);
    if (first.type == TOKEN_EOF) {
        destroyLexer(&lexer);
        return false;
    }
    Token second = getNextToken(&lexer);
    if (second.type != TOKEN_EOF) {
        token_free(&first);
        destroyLexer(&lexer);
        return false;
    }
    first.location = origin;
    first.line = origin.start.line;
    first.macroCallSite = empty_range();
    first.macroDefinition = empty_range();
    *out = first;
    destroyLexer(&lexer);
    return true;
}

static bool apply_token_paste(PPTokenBuffer* buffer, SourceRange origin) {
    if (!buffer || buffer->count == 0) {
        return true;
    }
    PPTokenBuffer result = {0};
    bool hasPending = false;
    Token pending = {0};

    for (size_t i = 0; i < buffer->count; ++i) {
        const Token* tok = &buffer->tokens[i];
        if (tok->type == TOKEN_DOUBLE_HASH) {
            if (!hasPending) {
                continue;
            }
            size_t nextIndex = i + 1;
            while (nextIndex < buffer->count &&
                   buffer->tokens[nextIndex].type == TOKEN_DOUBLE_HASH) {
                nextIndex++;
            }
            if (nextIndex >= buffer->count) {
                if (pending.type == TOKEN_COMMA) {
                    token_free(&pending);
                    hasPending = false;
                }
                continue;
            }
            const Token* rightTok = &buffer->tokens[nextIndex];
            const char* leftText = token_text(&pending);
            const char* rightText = token_text(rightTok);
            size_t len = strlen(leftText) + strlen(rightText) + 1;
            char* combined = (char*)malloc(len);
            if (!combined) {
                if (hasPending) token_free(&pending);
                pp_token_buffer_release(&result);
                return false;
            }
            snprintf(combined, len, "%s%s", leftText, rightText);

            Token relexed;
            if (!relex_single_token(combined, origin, &relexed)) {
                free(combined);
                if (hasPending) token_free(&pending);
                pp_token_buffer_release(&result);
                return false;
            }
            free(combined);
            token_free(&pending);
            pending = relexed;
            hasPending = true;
            i = nextIndex;
            continue;
        }

        if (hasPending) {
            if (!pp_token_buffer_append(&result, pending)) {
                token_free(&pending);
                pp_token_buffer_release(&result);
                return false;
            }
            hasPending = false;
        }
        pending = token_clone(tok);
        hasPending = true;
    }

    if (hasPending) {
        if (!pp_token_buffer_append(&result, pending)) {
            token_free(&pending);
            pp_token_buffer_release(&result);
            return false;
        }
    }

    pp_token_buffer_release(buffer);
    *buffer = result;
    return true;
}

static bool substitute_macro(MacroExpander* expander,
                             const MacroDefinition* def,
                             const MacroArgPack* args,
                             SourceRange callSite,
                             PPTokenBuffer* out) {
    PPTokenBuffer working = {0};
    (void)expander;

    for (size_t i = 0; i < def->body.count; ++i) {
        const Token* tok = &def->body.tokens[i];

        if (def->kind == MACRO_FUNCTION && args && tok->type == TOKEN_HASH) {
            if (i + 1 < def->body.count) {
                const Token* next = &def->body.tokens[i + 1];
                if (next->type == TOKEN_IDENTIFIER) {
                    ssize_t index = macro_param_index(def, next->value);
                    if (index >= 0) {
                        bool ok = append_stringified_argument(&working,
                                                              &args->positionals[index],
                                                              callSite);
                        if (!ok) {
                            pp_token_buffer_release(&working);
                            return false;
                        }
                        i++;
                        continue;
                    }
                    if (def->params.variadic && strcmp(next->value, "__VA_ARGS__") == 0) {
                        if (!stringify_variadic_arguments(&working, args, callSite)) {
                            pp_token_buffer_release(&working);
                            return false;
                        }
                        i++;
                        continue;
                    }
                }
            }
        }

        if (def->kind == MACRO_FUNCTION && args && tok->type == TOKEN_IDENTIFIER) {
            if (def->params.variadic &&
                def->params.hasVaOpt &&
                tok->value &&
                strcmp(tok->value, "__VA_OPT__") == 0) {
                // Expect a parenthesized sequence after __VA_OPT__
                if (i + 1 < def->body.count && def->body.tokens[i + 1].type == TOKEN_LPAREN) {
                    size_t start = i + 2;
                    size_t j = i + 2;
                    int depth = 1;
                    for (; j < def->body.count; ++j) {
                        if (def->body.tokens[j].type == TOKEN_LPAREN) depth++;
                        else if (def->body.tokens[j].type == TOKEN_RPAREN) {
                            depth--;
                            if (depth == 0) {
                                break;
                            }
                        }
                    }
                    if (depth == 0 && j > start) {
                        if (args->variadicCount > 0) {
                            MacroDefinition inner = *def;
                            inner.body.tokens = def->body.tokens + start;
                            inner.body.count = j - start;
                            PPTokenBuffer optOut = {0};
                            bool ok = substitute_macro(expander, &inner, args, callSite, &optOut);
                            if (!ok) {
                                pp_token_buffer_release(&working);
                                return false;
                            }
                            if (!append_argument_tokens(&working, &optOut)) {
                                pp_token_buffer_release(&optOut);
                                pp_token_buffer_release(&working);
                                return false;
                            }
                            pp_token_buffer_release(&optOut);
                        }
                        i = j; // skip to closing paren
                        continue;
                    }
                }
                // If malformed, fall through and treat as identifier.
            }
            ssize_t index = macro_param_index(def, tok->value);
            bool adjacentPaste = false;
            bool definedOperandContext = false;
            if (index >= 0) {
                if (i > 0 &&
                    def->body.tokens[i - 1].type == TOKEN_IDENTIFIER &&
                    def->body.tokens[i - 1].value &&
                    strcmp(def->body.tokens[i - 1].value, "defined") == 0) {
                    definedOperandContext = true;
                } else if (i > 1 &&
                           def->body.tokens[i - 1].type == TOKEN_LPAREN &&
                           def->body.tokens[i - 2].type == TOKEN_IDENTIFIER &&
                           def->body.tokens[i - 2].value &&
                           strcmp(def->body.tokens[i - 2].value, "defined") == 0) {
                    definedOperandContext = true;
                }
                if ((i + 1 < def->body.count &&
                     def->body.tokens[i + 1].type == TOKEN_DOUBLE_HASH) ||
                    (i > 0 && def->body.tokens[i - 1].type == TOKEN_DOUBLE_HASH)) {
                    adjacentPaste = true;
                }
                const PPTokenBuffer* buffer = (adjacentPaste || definedOperandContext)
                    ? &args->positionals[index].raw
                    : &args->positionals[index].expanded;
                if (!append_argument_tokens(&working, buffer)) {
                    pp_token_buffer_release(&working);
                    return false;
                }
                continue;
            }
            if (def->params.variadic && strcmp(tok->value, "__VA_ARGS__") == 0) {
                if ((i + 1 < def->body.count &&
                     def->body.tokens[i + 1].type == TOKEN_DOUBLE_HASH) ||
                    (i > 0 && def->body.tokens[i - 1].type == TOKEN_DOUBLE_HASH)) {
                    adjacentPaste = true;
                }
                if (!append_variadic_tokens(&working, args, adjacentPaste, callSite)) {
                    pp_token_buffer_release(&working);
                    return false;
                }
                continue;
            }
        }

        if (!pp_token_buffer_append_clone(&working, tok)) {
            pp_token_buffer_release(&working);
            return false;
        }
    }

    if (!apply_token_paste(&working, callSite)) {
        pp_token_buffer_release(&working);
        return false;
    }

    for (size_t i = 0; i < working.count; ++i) {
        working.tokens[i].macroCallSite = callSite;
        if (!range_is_initialized(&working.tokens[i].macroDefinition)) {
            working.tokens[i].macroDefinition = def->definitionRange;
        }
    }

    *out = working;
    return true;
}

static bool append_buffer(PPTokenBuffer* dest, const PPTokenBuffer* src) {
    if (!dest || !src) return true;
    for (size_t i = 0; i < src->count; ++i) {
        if (!pp_token_buffer_append_clone(dest, &src->tokens[i])) {
            return false;
        }
    }
    return true;
}

void macro_expander_init(MacroExpander* expander, MacroTable* table) {
    if (!expander) return;
    expander->table = table;
    expander->builtins.baseFile = NULL;
    expander->builtins.dateString = NULL;
    expander->builtins.timeString = NULL;
    expander->builtins.counter = NULL;
    expander->builtins.logicalFile = NULL;
    expander->builtins.lineOffset = NULL;
    expander->builtins.lineRemapActive = NULL;
    expander->preserveDefinedOperands = false;
    macro_expander_clear_error(expander);
}

void macro_expander_reset(MacroExpander* expander) {
    macro_expander_clear_error(expander);
}

void macro_expander_set_preserve_defined_operands(MacroExpander* expander, bool enabled) {
    if (!expander) return;
    expander->preserveDefinedOperands = enabled;
}

MacroExpandErrorInfo macro_expander_last_error(const MacroExpander* expander) {
    MacroExpandErrorInfo empty = {0};
    if (!expander) return empty;
    return expander->lastError;
}

bool macro_expander_expand(MacroExpander* expander,
                           const Token* input,
                           size_t count,
                           PPTokenBuffer* outTokens) {
    if (!outTokens) return false;
    macro_expander_clear_error(expander);
    outTokens->tokens = NULL;
    outTokens->count = 0;
    outTokens->capacity = 0;
    if (!input || count == 0) {
        return true;
    }

    for (size_t i = 0; i < count; ++i) {
        const Token* tok = &input[i];

        if (tok->type == TOKEN_IDENTIFIER && expander) {
            if (handle_builtin_identifier(expander, tok, outTokens)) {
                continue;
            }
        }

        if (tok->type == TOKEN_IDENTIFIER && expander && expander->table) {
            if (expander->preserveDefinedOperands &&
                token_is_defined_operand_context(input, i)) {
                if (!pp_token_buffer_append_clone(outTokens, tok)) {
                    return false;
                }
                continue;
            }
            const MacroDefinition* def = macro_table_lookup(expander->table, tok->value);
            if (def && macro_table_is_expanding(expander->table, tok->value)) {
                if (!pp_token_buffer_append_clone(outTokens, tok)) {
                    return false;
                }
                continue;
            }
            if (def && !macro_table_is_expanding(expander->table, tok->value)) {
                SourceRange callRange = tok->location;
                if (def->kind == MACRO_OBJECT) {
                    PPTokenBuffer replaced = {0};
                    if (!macro_table_push_expansion(expander->table, def, callRange, def->definitionRange)) {
                        pp_token_buffer_release(&replaced);
                        return false;
                    }
                    bool ok = substitute_macro(expander, def, NULL, callRange, &replaced);
                    if (ok) {
                        PPTokenBuffer nested = {0};
                        ok = macro_expander_expand(expander, replaced.tokens, replaced.count, &nested);
                        if (ok) {
                            ok = append_buffer(outTokens, &nested);
                        }
                        pp_token_buffer_release(&nested);
                    }
                    macro_table_pop_expansion(expander->table, def);
                    pp_token_buffer_release(&replaced);
                    if (!ok) {
                        return false;
                    }
                    continue;
                } else if (def->kind == MACRO_FUNCTION) {
                    bool expanded = false;
                    if (i + 1 < count && input[i + 1].type == TOKEN_LPAREN) {
                        size_t cursor = i + 2;
                        PPArgumentList rawArgs;
                        pp_argument_list_init(&rawArgs);
                        if (parse_macro_argument_tokens(input, count, &cursor, &rawArgs, def)) {
                            MacroArgPack pack;
                            if (build_macro_arg_pack(expander, def, &rawArgs, &pack, callRange)) {
                                if (expand_macro_arguments(expander, &pack) &&
                                    macro_table_push_expansion(expander->table, def, callRange, def->definitionRange)) {
                                    PPTokenBuffer replaced = {0};
                                    bool ok = substitute_macro(expander, def, &pack, callRange, &replaced);
                                    if (ok) {
                                        PPTokenBuffer nested = {0};
                                        ok = macro_expander_expand(expander, replaced.tokens, replaced.count, &nested);
                                        if (ok) {
                                            ok = append_buffer(outTokens, &nested);
                                        }
                                        pp_token_buffer_release(&nested);
                                    }
                                    macro_table_pop_expansion(expander->table, def);
                                    pp_token_buffer_release(&replaced);
                                    if (ok) {
                                        expanded = true;
                                        i = cursor - 1;
                                    } else {
                                        macro_arg_pack_destroy(&pack);
                                        pp_argument_list_destroy(&rawArgs);
                                        return false;
                                    }
                                }
                                macro_arg_pack_destroy(&pack);
                            } else if (expander && expander->lastError.kind != ME_ERR_NONE) {
                                pp_argument_list_destroy(&rawArgs);
                                return false;
                            }
                        }
                        pp_argument_list_destroy(&rawArgs);
                    }
                    if (expanded) {
                        continue;
                    }
                }
            }
        }

        if (!pp_token_buffer_append_clone(outTokens, tok)) {
            return false;
        }
    }

    return true;
}
