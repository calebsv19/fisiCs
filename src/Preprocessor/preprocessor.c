#include "Preprocessor/preprocessor.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "Lexer/tokens.h"
#include "Compiler/diagnostics.h"

static bool preprocess_tokens(Preprocessor* pp,
                              const TokenBuffer* input,
                              PPTokenBuffer* output,
                              bool appendEOF);
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

static void pp_report_diag(Preprocessor* pp,
                           const Token* tok,
                           DiagKind kind,
                           int code,
                           const char* fmt,
                           ...) {
    if (!pp || !pp->ctx || !fmt) return;
    va_list args;
    va_start(args, fmt);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    compiler_report_diag(pp->ctx,
                         tok ? tok->location : (SourceRange){0},
                         kind,
                         code,
                         NULL,
                         "%s",
                         buffer);
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

static void skip_to_line_end(const Token* tokens,
                             size_t count,
                             size_t* cursor) {
    size_t i = *cursor;
    int line = tokens[i].line;
    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == line) {
        i++;
    }
    *cursor = (i == 0) ? 0 : i - 1;
}

static const char* token_file(const Token* tok) {
    if (!tok || !tok->location.start.file) return NULL;
    return tok->location.start.file;
}

static const char* detect_include_guard(const TokenBuffer* buffer) {
    if (!buffer || buffer->count < 3) return NULL;
    const Token* tokens = buffer->tokens;
    if (tokens[0].type != TOKEN_IFNDEF) return NULL;
    if (tokens[1].type != TOKEN_IDENTIFIER) return NULL;
    const char* guard = tokens[1].value;
    size_t i = 2;
    int firstLine = tokens[0].line;
    while (i < buffer->count && tokens[i].type != TOKEN_EOF && tokens[i].line == firstLine) {
        i++;
    }
    if (i >= buffer->count || tokens[i].type == TOKEN_EOF) return NULL;
    int defineLine = tokens[i].line;
    if (tokens[i].type != TOKEN_DEFINE) return NULL;
    i++;
    if (i >= buffer->count || tokens[i].type == TOKEN_EOF || tokens[i].line != defineLine) return NULL;
    if (tokens[i].type != TOKEN_IDENTIFIER) return NULL;
    if (strcmp(tokens[i].value, guard) != 0) return NULL;
    return guard;
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

static bool append_directive_line(const Token* tokens,
                                  size_t count,
                                  size_t cursor,
                                  PPTokenBuffer* output) {
    if (!tokens || !output) return false;
    int line = tokens[cursor].line;
    size_t i = cursor;
    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == line) {
        if (!pp_token_buffer_append_clone(output, &tokens[i])) {
            return false;
        }
        i++;
    }
    return true;
}

static bool parse_include_operand(const Token* tokens,
                                  size_t count,
                                  size_t* cursor,
                                  char** outName,
                                  bool* outIsSystem) {
    size_t i = *cursor + 1;
    int directiveLine = tokens[*cursor].line;
    if (i >= count) return false;

    const Token* tok = &tokens[i];
    if (tok->type == TOKEN_STRING) {
        *outName = pp_strdup_local(tok->value);
        *outIsSystem = false;
        if (!*outName) return false;
        i++;
    } else if (tok->type == TOKEN_LESS) {
        char buffer[1024];
        size_t len = 0;
        i++;
        bool closed = false;
        while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
            if (tokens[i].type == TOKEN_GREATER) {
                closed = true;
                i++;
                break;
            }
            const char* piece = tokens[i].value ? tokens[i].value : "";
            size_t pieceLen = strlen(piece);
            if (len + pieceLen + 1 >= sizeof(buffer)) {
                return false;
            }
            memcpy(buffer + len, piece, pieceLen);
            len += pieceLen;
            i++;
        }
        if (!closed) return false;
        buffer[len] = '\0';
        *outName = pp_strdup_local(buffer);
        *outIsSystem = true;
        if (!*outName) return false;
    } else {
        return false;
    }

    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
        i++;
    }
    *cursor = (i == 0) ? 0 : i - 1;
    return true;
}

static bool process_define(Preprocessor* pp,
                           const Token* tokens,
                           size_t count,
                           size_t* cursor) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    i++;
    if (i >= count || !tokens[i].value) {
        pp_report_diag(pp, tokens ? &tokens[i] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "expected identifier after #define");
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
        tokens[i].line == nameTok->line) {
        isFunction = true;
        if (!parse_macro_parameters(tokens, count, &i, &params)) {
            pp_report_diag(pp, nameTok, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "invalid parameter list in #define %s", nameTok->value ? nameTok->value : "");
            goto cleanup;
        }
    }

    if (!collect_macro_body(tokens, count, &i, directiveLine, &body)) {
        pp_report_diag(pp, nameTok, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "failed to collect macro body for %s", nameTok->value ? nameTok->value : "");
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
        pp_report_diag(pp, nameTok, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "failed to record macro %s", nameTok->value ? nameTok->value : "");
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
        pp_report_diag(pp, tokens ? &tokens[i] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "expected identifier after #undef");
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

typedef struct {
    bool parentActive;
    bool selfActive;
    bool branchTaken;
    bool sawElse;
} PPConditionalFrame;

static bool conditional_stack_is_active(const PPConditionalFrame* stack, size_t depth) {
    if (depth == 0) return true;
    return stack[depth - 1].selfActive;
}

static bool push_conditional(PPConditionalFrame** stack,
                             size_t* depth,
                             size_t* capacity,
                             PPConditionalFrame frame) {
    if (*depth == *capacity) {
        size_t newCap = (*capacity == 0) ? 4 : (*capacity * 2);
        PPConditionalFrame* newStack = realloc(*stack, newCap * sizeof(PPConditionalFrame));
        if (!newStack) return false;
        *stack = newStack;
        *capacity = newCap;
    }
    (*stack)[(*depth)++] = frame;
    return true;
}

static bool process_if(Preprocessor* pp,
                       const Token* tokens,
                       size_t count,
                       size_t* cursor,
                       PPConditionalFrame** stack,
                       size_t* depth,
                       size_t* capacity) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    size_t exprStart = i + 1;
    size_t exprEnd = exprStart;
    while (exprEnd < count && tokens[exprEnd].type != TOKEN_EOF && tokens[exprEnd].line == directiveLine) {
        exprEnd++;
    }

    bool parentActive = conditional_stack_is_active(*stack, *depth);
    bool condValue = false;
    if (parentActive) {
        int32_t value = 0;
        preprocessor_eval_tokens(pp, tokens + exprStart, exprEnd - exprStart, &value);
        condValue = (value != 0);
    }

    PPConditionalFrame frame;
    frame.parentActive = parentActive;
    frame.selfActive = parentActive && condValue;
    frame.branchTaken = frame.selfActive;
    frame.sawElse = false;

    if (!push_conditional(stack, depth, capacity, frame)) {
        return false;
    }

    *cursor = (exprEnd == 0) ? 0 : exprEnd - 1;
    return true;
}

static bool process_ifdeflike(Preprocessor* pp,
                              const Token* tokens,
                              size_t count,
                              size_t* cursor,
                              PPConditionalFrame** stack,
                              size_t* depth,
                              size_t* capacity,
                              bool negate) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    i++;
    if (i >= count || tokens[i].type != TOKEN_IDENTIFIER) {
        pp_report_diag(pp, tokens ? &tokens[i] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "expected identifier after #%s",
                       negate ? "ifndef" : "ifdef");
        return false;
    }
    const char* name = tokens[i].value;
    bool parentActive = conditional_stack_is_active(*stack, *depth);
    bool isDefined = parentActive && (macro_table_lookup(pp->table, name) != NULL);
    bool selfActive = parentActive && (negate ? !isDefined : isDefined);

    PPConditionalFrame frame;
    frame.parentActive = parentActive;
    frame.selfActive = selfActive;
    frame.branchTaken = selfActive;
    frame.sawElse = false;

    if (!push_conditional(stack, depth, capacity, frame)) {
        return false;
    }

    // skip rest of the line
    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
        i++;
    }
    *cursor = (i == 0) ? 0 : i - 1;
    return true;
}

static bool process_include(Preprocessor* pp,
                            const Token* tokens,
                            size_t count,
                            size_t* cursor,
                            PPTokenBuffer* output) {
    if (!pp || !pp->resolver) {
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "include resolver not initialized");
        return false;
    }
    char* name = NULL;
    bool isSystem = false;
    if (!parse_include_operand(tokens, count, cursor, &name, &isSystem)) {
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "invalid #include operand");
        return false;
    }

    const char* parentFile = token_file(&tokens[*cursor]);
    const IncludeFile* inc = include_resolver_load(pp->resolver, parentFile, name, isSystem);
    if (!inc) {
        if (isSystem) {
            pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_WARNING, CDIAG_PREPROCESSOR_GENERIC, "skipping missing system include '%s'",
                           name ? name : "<null>");
            free(name);
            return true;
        }
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "could not resolve include '%s'", name ? name : "<null>");
        free(name);
        return false;
    }
    free(name);

    include_graph_add(&pp->includeGraph,
                      parentFile ? parentFile : "<unknown>",
                      inc->path ? inc->path : "<unknown>");

    if (inc->pragmaOnce && include_resolver_was_included(pp->resolver, inc->path)) {
        return true;
    }

    Lexer lexer;
    initLexer(&lexer, inc->contents, inc->path);
    TokenBuffer buffer;
    token_buffer_init(&buffer);
    if (!token_buffer_fill_from_lexer(&buffer, &lexer)) {
        token_buffer_destroy(&buffer);
        return false;
    }

    const char* guard = detect_include_guard(&buffer);
    if (guard && macro_table_lookup(pp->table, guard) != NULL) {
        token_buffer_destroy(&buffer);
        include_resolver_mark_included(pp->resolver, inc->path);
        return true;
    }

    bool ok = preprocess_tokens(pp, &buffer, output, false);
    token_buffer_destroy(&buffer);
    include_resolver_mark_included(pp->resolver, inc->path);
    return ok;
}

static bool process_elif(Preprocessor* pp,
                         const Token* tokens,
                         size_t count,
                         size_t* cursor,
                         PPConditionalFrame* stack,
                         size_t depth) {
    if (depth == 0) {
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "#elif without matching #if");
        return false;
    }
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    size_t exprStart = i + 1;
    size_t exprEnd = exprStart;
    while (exprEnd < count && tokens[exprEnd].type != TOKEN_EOF && tokens[exprEnd].line == directiveLine) {
        exprEnd++;
    }

    PPConditionalFrame* frame = &stack[depth - 1];
    if (frame->sawElse) {
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "#elif after #else");
        return false;
    }

    bool newActive = false;
    if (frame->parentActive && !frame->branchTaken) {
        int32_t value = 0;
        preprocessor_eval_tokens(pp, tokens + exprStart, exprEnd - exprStart, &value);
        newActive = (value != 0);
    }
    frame->selfActive = frame->parentActive && newActive;
    if (newActive && frame->parentActive) {
        frame->branchTaken = true;
    }

    *cursor = (exprEnd == 0) ? 0 : exprEnd - 1;
    return true;
}

static bool process_else(Preprocessor* pp, PPConditionalFrame* stack, size_t depth, const Token* tok) {
    if (depth == 0) {
        pp_report_diag(pp, tok, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "#else without matching #if");
        return false;
    }
    PPConditionalFrame* frame = &stack[depth - 1];
    if (frame->sawElse) {
        pp_report_diag(pp, tok, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "duplicate #else");
        return false;
    }
    bool newActive = frame->parentActive && !frame->branchTaken;
    frame->selfActive = newActive;
    if (newActive) {
        frame->branchTaken = true;
    }
    frame->sawElse = true;
    return true;
}

static bool process_endif(Preprocessor* pp, PPConditionalFrame* stack, size_t* depth, const Token* tok) {
    (void)stack;
    if (*depth == 0) {
        pp_report_diag(pp, tok, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "#endif without matching #if");
        return false;
    }
    (*depth)--;
    return true;
}

static bool process_pragma(Preprocessor* pp,
                           const Token* tokens,
                           size_t count,
                           size_t* cursor) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    i++;
    if (i < count && tokens[i].type == TOKEN_ONCE) {
        const char* path = token_file(&tokens[i]);
        if (!path) path = token_file(&tokens[*cursor]);
        if (path && pp && pp->resolver) {
            include_resolver_mark_pragma_once(pp->resolver, path);
        }
    }
    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
        i++;
    }
    *cursor = (i == 0) ? 0 : i - 1;
    return true;
}

bool preprocessor_init(Preprocessor* pp,
                       CompilerContext* ctx,
                       bool preserveDirectives,
                       const char* const* includePaths,
                       size_t includePathCount) {
    if (!pp) return false;
    pp->table = macro_table_create();
    if (!pp->table) {
        return false;
    }
    macro_expander_init(&pp->expander, pp->table);
    pp_expr_evaluator_init(&pp->exprEval, pp->table);
    pp->resolver = include_resolver_create(includePaths, includePathCount);
    if (!pp->resolver) {
        macro_table_destroy(pp->table);
        pp->table = NULL;
        return false;
    }
    include_graph_init(&pp->includeGraph);
    pp->preserveDirectives = preserveDirectives;
    pp->ctx = ctx;
    return true;
}

void preprocessor_destroy(Preprocessor* pp) {
    if (!pp) return;
    macro_table_destroy(pp->table);
    pp->table = NULL;
    macro_expander_reset(&pp->expander);
    pp_expr_evaluator_reset(&pp->exprEval);
    include_resolver_destroy(pp->resolver);
    pp->resolver = NULL;
    include_graph_destroy(&pp->includeGraph);
}

MacroTable* preprocessor_get_macro_table(Preprocessor* pp) {
    return pp ? pp->table : NULL;
}

IncludeResolver* preprocessor_get_resolver(Preprocessor* pp) {
    return pp ? pp->resolver : NULL;
}

IncludeGraph* preprocessor_get_include_graph(Preprocessor* pp) {
    return pp ? &pp->includeGraph : NULL;
}

bool preprocessor_eval_tokens(Preprocessor* pp,
                              const Token* tokens,
                              size_t count,
                              int32_t* outValue) {
    if (!pp) {
        if (outValue) *outValue = 0;
        return false;
    }
    return pp_expr_eval_tokens(&pp->exprEval, tokens, count, outValue);
}

bool preprocessor_eval_range(Preprocessor* pp,
                             const TokenBuffer* buffer,
                             size_t start,
                             size_t end,
                             int32_t* outValue) {
    if (!pp) {
        if (outValue) *outValue = 0;
        return false;
    }
    return pp_expr_eval_range(&pp->exprEval, buffer, start, end, outValue);
}

static bool preprocess_tokens(Preprocessor* pp,
                              const TokenBuffer* input,
                              PPTokenBuffer* output,
                              bool appendEOF) {
    if (!pp || !input || !output) return false;
    PPTokenBuffer chunk = {0};
    PPConditionalFrame* condStack = NULL;
    size_t condDepth = 0;
    size_t condCap = 0;

    for (size_t i = 0; i < input->count; ++i) {
        const Token* tok = &input->tokens[i];
        bool active = conditional_stack_is_active(condStack, condDepth);
        switch (tok->type) {
            case TOKEN_DEFINE:
                if (active) {
                    if (!flush_chunk(pp, &chunk, output)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                    if (pp->preserveDirectives) {
                        if (!append_directive_line(input->tokens, input->count, i, output)) {
                            pp_token_buffer_reset(&chunk);
                            free(condStack);
                            return false;
                        }
                    }
                    if (!process_define(pp, input->tokens, input->count, &i)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                } else {
                    skip_to_line_end(input->tokens, input->count, &i);
                }
                break;
            case TOKEN_INCLUDE:
                if (active) {
                    if (!flush_chunk(pp, &chunk, output)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                    if (pp->preserveDirectives) {
                        if (!append_directive_line(input->tokens, input->count, i, output)) {
                            pp_token_buffer_reset(&chunk);
                            free(condStack);
                            return false;
                        }
                    }
                    if (!process_include(pp, input->tokens, input->count, &i, output)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                } else {
                    skip_to_line_end(input->tokens, input->count, &i);
                }
                break;
            case TOKEN_UNDEF:
                if (active) {
                    if (!flush_chunk(pp, &chunk, output)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                    if (!process_undef(pp, input->tokens, input->count, &i)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                } else {
                    skip_to_line_end(input->tokens, input->count, &i);
                }
                break;
            case TOKEN_PP_IF:
                if (!flush_chunk(pp, &chunk, output)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                if (!process_if(pp, input->tokens, input->count, &i,
                                &condStack, &condDepth, &condCap)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                break;
            case TOKEN_PP_ELIF:
                if (!flush_chunk(pp, &chunk, output)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                if (!process_elif(pp, input->tokens, input->count, &i,
                                  condStack, condDepth)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                break;
            case TOKEN_PP_ELSE:
                if (!flush_chunk(pp, &chunk, output)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                if (!process_else(pp, condStack, condDepth, &input->tokens[i])) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                skip_to_line_end(input->tokens, input->count, &i);
                break;
            case TOKEN_ENDIF:
                if (!flush_chunk(pp, &chunk, output)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                if (pp->preserveDirectives && active) {
                    if (!append_directive_line(input->tokens, input->count, i, output)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                }
                if (!process_endif(pp, condStack, &condDepth, &input->tokens[i])) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                break;
            case TOKEN_IFDEF:
                if (!flush_chunk(pp, &chunk, output)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                if (pp->preserveDirectives && active) {
                    if (!append_directive_line(input->tokens, input->count, i, output)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                }
                if (!process_ifdeflike(pp, input->tokens, input->count, &i,
                                       &condStack, &condDepth, &condCap, false)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                break;
            case TOKEN_IFNDEF:
                if (!flush_chunk(pp, &chunk, output)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                if (pp->preserveDirectives && active) {
                    if (!append_directive_line(input->tokens, input->count, i, output)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                }
                if (!process_ifdeflike(pp, input->tokens, input->count, &i,
                                       &condStack, &condDepth, &condCap, true)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                break;
            case TOKEN_PRAGMA:
                if (active) {
                    if (!flush_chunk(pp, &chunk, output)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                    if (!process_pragma(pp, input->tokens, input->count, &i)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                } else {
                    skip_to_line_end(input->tokens, input->count, &i);
                }
                break;
            case TOKEN_EOF:
                // handled after loop
                break;
            default:
                if (active) {
                    if (!pp_token_buffer_append_clone(&chunk, tok)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                }
                break;
        }
    }

    bool flushed = flush_chunk(pp, &chunk, output);
    if (flushed && appendEOF && input->count > 0) {
        const Token* eofTok = &input->tokens[input->count - 1];
        if (eofTok->type == TOKEN_EOF) {
            flushed = pp_token_buffer_append_clone(output, eofTok);
        }
    }
    pp_token_buffer_reset(&chunk);
    free(condStack);
    return flushed;
}

bool preprocessor_run(Preprocessor* pp,
                      const TokenBuffer* input,
                      PPTokenBuffer* output) {
    if (!pp || !input || !output) return false;
    pp_token_buffer_init_local(output);
    return preprocess_tokens(pp, input, output, true);
}
