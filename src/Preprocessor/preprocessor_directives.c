#include "Preprocessor/pp_internal.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "Lexer/token_buffer.h"
#include "Lexer/tokens.h"

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

static bool tokens_adjacent(const Token* left, const Token* right) {
    if (!left || !right) return false;
    if (left->location.end.line != right->location.start.line) return false;
    return right->location.start.column <= left->location.end.column;
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
    if (i < count && tokens[i].type == TOKEN_RPAREN) {
        i++; // consume ')'
        *cursor = i;
        return true;
    }
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
                               PPTokenBuffer* body,
                               Preprocessor* pp) {
    size_t i = *cursor;
    while (i < count) {
        const Token* tok = &tokens[i];
        if (tok->type == TOKEN_EOF) {
            break;
        }
        if (tok->line != directiveLine) {
            break;
        }
        if (!pp_token_buffer_append_clone_remap(body, pp, tok)) {
            return false;
        }
        i++;
    }
    *cursor = i;
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

bool process_define(Preprocessor* pp,
                    const Token* tokens,
                    size_t count,
                    size_t* cursor) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    i++;
    if (i >= count || !tokens[i].value) {
        DiagKind kind = pp && pp->lenientMissingIncludes ? DIAG_WARNING : DIAG_ERROR;
        pp_report_diag(pp, tokens ? &tokens[i] : NULL, kind, CDIAG_PREPROCESSOR_GENERIC, "expected identifier after #define");
        if (pp && pp->lenientMissingIncludes) {
            skip_to_line_end(tokens, count, &i);
            *cursor = (i == 0) ? 0 : i - 1;
            return true;
        }
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
        tokens_adjacent(nameTok, &tokens[i])) {
        isFunction = true;
        if (!parse_macro_parameters(tokens, count, &i, &params)) {
            DiagKind kind = pp->lenientMissingIncludes ? DIAG_WARNING : DIAG_ERROR;
            pp_report_diag(pp, nameTok, kind, CDIAG_PREPROCESSOR_GENERIC, "invalid parameter list in #define %s", nameTok->value ? nameTok->value : "");
            if (pp->lenientMissingIncludes) {
                while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
                    i++;
                }
                *cursor = (i == 0) ? 0 : i - 1;
                macro_param_parse_destroy(&params);
                pp_token_buffer_reset(&body);
                return true;
            }
            goto cleanup;
        }
    }

    if (!collect_macro_body(tokens, count, &i, directiveLine, &body, pp)) {
        DiagKind kind = pp->lenientMissingIncludes ? DIAG_WARNING : DIAG_ERROR;
        pp_report_diag(pp, nameTok, kind, CDIAG_PREPROCESSOR_GENERIC, "failed to collect macro body for %s", nameTok->value ? nameTok->value : "");
        if (pp->lenientMissingIncludes) {
            skip_to_line_end(tokens, count, &i);
            *cursor = (i == 0) ? 0 : i - 1;
            macro_param_parse_destroy(&params);
            pp_token_buffer_reset(&body);
            return true;
        }
        goto cleanup;
    }

    if (isFunction && params.variadic) {
        for (size_t b = 0; b < body.count; ++b) {
            const Token* btok = &body.tokens[b];
            if (btok->type == TOKEN_IDENTIFIER &&
                btok->value &&
                strcmp(btok->value, "__VA_OPT__") == 0) {
                params.hasVaOpt = true;
                break;
            }
        }
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
        DiagKind kind = pp->lenientMissingIncludes ? DIAG_WARNING : DIAG_ERROR;
        pp_report_diag(pp, nameTok, kind, CDIAG_PREPROCESSOR_GENERIC, "failed to record macro %s", nameTok->value ? nameTok->value : "");
        if (pp->lenientMissingIncludes) {
            skip_to_line_end(tokens, count, &i);
            *cursor = (i == 0) ? 0 : i - 1;
            macro_param_parse_destroy(&params);
            pp_token_buffer_reset(&body);
            return true;
        }
        goto cleanup;
    }

    *cursor = (i == 0) ? 0 : i - 1;
cleanup:
    macro_param_parse_destroy(&params);
    pp_token_buffer_reset(&body);
    return ok;
}

bool process_undef(Preprocessor* pp,
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

bool process_include(Preprocessor* pp,
                     const Token* tokens,
                     size_t count,
                     size_t* cursor,
                     PPTokenBuffer* output,
                     bool isIncludeNext) {
    if (!pp || !pp->resolver) {
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "include resolver not initialized");
        return false;
    }
    char* name = NULL;
    bool isSystem = false;
    size_t lineStart = *cursor;
    int directiveLine = tokens && count > 0 ? tokens[lineStart].line : 0;
    size_t lineEnd = lineStart + 1;
    while (lineEnd < count && tokens[lineEnd].type != TOKEN_EOF && tokens[lineEnd].line == directiveLine) {
        lineEnd++;
    }

    PPTokenBuffer expanded = {0};
    size_t span = (lineEnd > lineStart + 1) ? (lineEnd - (lineStart + 1)) : 0;
    if (!macro_expander_expand(&pp->expander, tokens + lineStart + 1, span, &expanded)) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "failed to expand #include operand");
        return false;
    }

    size_t tempCount = expanded.count + 2;
    Token* tempTokens = (Token*)calloc(tempCount, sizeof(Token));
    if (!tempTokens) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "out of memory parsing #include");
        return false;
    }
    tempTokens[0] = tokens[*cursor];
    for (size_t i = 0; i < expanded.count; ++i) {
        tempTokens[i + 1] = expanded.tokens[i];
        tempTokens[i + 1].line = directiveLine;
        tempTokens[i + 1].location.start.line = directiveLine;
        tempTokens[i + 1].location.end.line = directiveLine;
    }
    tempTokens[tempCount - 1].type = TOKEN_EOF;
    tempTokens[tempCount - 1].line = directiveLine;

    size_t tempCursor = 0;
    if (!parse_include_operand(tempTokens, tempCount, &tempCursor, &name, &isSystem)) {
        free(tempTokens);
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "invalid #include operand");
        return false;
    }
    free(tempTokens);
    pp_token_buffer_destroy(&expanded);
    *cursor = (lineEnd == 0) ? 0 : lineEnd - 1;

    const char* parentFile = token_file(&tokens[*cursor]);
    IncludeSearchOrigin origin = INCLUDE_SEARCH_RAW;
    size_t originIndex = (size_t)-1;
    const IncludeFile* incPtr = include_resolver_load(pp->resolver,
                                                      parentFile,
                                                      name,
                                                      isSystem,
                                                      isIncludeNext,
                                                      &origin,
                                                      &originIndex);
    IncludeFile incValue;
    bool haveInc = false;
    if (incPtr) {
        incValue = *incPtr;
        incPtr = NULL;
        haveInc = true;
    }

    FisicsInclude incRec = {0};
    incRec.name = name;
    incRec.kind = isSystem ? FISICS_INCLUDE_SYSTEM : FISICS_INCLUDE_LOCAL;
    incRec.line = tokens ? tokens[*cursor].line : 0;
    incRec.column = tokens ? tokens[*cursor].location.start.column : 0;
    incRec.resolved = haveInc;
    incRec.resolved_path = haveInc ? incValue.path : NULL;
    if (!haveInc) {
        incRec.origin = FISICS_INCLUDE_UNRESOLVED;
    } else {
        switch (origin) {
            case INCLUDE_SEARCH_SAME_DIR:
                incRec.origin = FISICS_INCLUDE_PROJECT;
                break;
            case INCLUDE_SEARCH_INCLUDE_PATH:
                incRec.origin = isSystem ? FISICS_INCLUDE_SYSTEM_ORIGIN : FISICS_INCLUDE_PROJECT;
                break;
            case INCLUDE_SEARCH_RAW:
            default:
                incRec.origin = isSystem ? FISICS_INCLUDE_SYSTEM_ORIGIN : FISICS_INCLUDE_EXTERNAL;
                break;
        }
    }
    if (pp->ctx) {
        cc_append_include(pp->ctx, &incRec);
    }
    if (!haveInc) {
        bool warnOnly = isSystem || pp->lenientMissingIncludes;
        if (warnOnly) {
            pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_WARNING, CDIAG_PREPROCESSOR_GENERIC,
                           "skipping missing %s include '%s'", isSystem ? "system" : "local", name ? name : "<null>");
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
                      incValue.path ? incValue.path : "<unknown>");

    if (incValue.pragmaOnce && include_resolver_was_included(pp->resolver, incValue.path)) {
        return true;
    }

    if (pp_include_stack_contains(pp, incValue.path)) {
        pp_report_diag(pp,
                       tokens ? &tokens[*cursor] : NULL,
                       DIAG_ERROR,
                       CDIAG_PREPROCESSOR_GENERIC,
                       "detected recursive include of '%s'",
                       incValue.path ? incValue.path : "<unknown>");
        return false;
    }

    int savedOffset = pp->lineOffset;
    char* savedLogical = pp->logicalFile;
    bool savedRemap = pp->lineRemapActive;
    pp->lineOffset = 0;
    pp->lineRemapActive = false;
    pp_set_logical_file(pp, incValue.path);

    Lexer lexer;
    initLexer(&lexer, incValue.contents, incValue.path, pp->enableTrigraphs);
    TokenBuffer buffer;
    token_buffer_init(&buffer);
    bool pushedFrame = pp_include_stack_push(pp, incValue.path, origin, originIndex);

    if (!token_buffer_fill_from_lexer(&buffer, &lexer)) {
        token_buffer_destroy(&buffer);
        destroyLexer(&lexer);
        pp->lineOffset = savedOffset;
        pp->logicalFile = savedLogical;
        pp->lineRemapActive = savedRemap;
        if (pushedFrame) pp_include_stack_pop(pp);
        return false;
    }
    destroyLexer(&lexer);

    const char* guard = detect_include_guard(&buffer);
    if (guard && macro_table_lookup(pp->table, guard) != NULL) {
        token_buffer_destroy(&buffer);
        include_resolver_mark_included(pp->resolver, incValue.path);
        pp->lineOffset = savedOffset;
        pp->logicalFile = savedLogical;
        pp->lineRemapActive = savedRemap;
        if (pushedFrame) pp_include_stack_pop(pp);
        return true;
    }

    bool ok = preprocess_tokens(pp, &buffer, output, false);
    if (!ok) {
        pp_debug_fail("process_include_body", tokens ? &tokens[*cursor] : NULL);
    }
    token_buffer_destroy(&buffer);
    pp->lineOffset = savedOffset;
    pp->logicalFile = savedLogical;
    pp->lineRemapActive = savedRemap;
    include_resolver_mark_included(pp->resolver, incValue.path);
    if (pushedFrame) pp_include_stack_pop(pp);
    return ok;
}

bool process_pragma(Preprocessor* pp,
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
    } else if (i < count && tokens[i].type == TOKEN_IDENTIFIER && tokens[i].value &&
               strcmp(tokens[i].value, "STDC") == 0) {
        const char* kind = NULL;
        if ((i + 1) < count &&
            tokens[i + 1].line == directiveLine &&
            tokens[i + 1].type == TOKEN_IDENTIFIER &&
            tokens[i + 1].value) {
            const char* val = tokens[i + 1].value;
            if (strcmp(val, "FP_CONTRACT") == 0 ||
                strcmp(val, "CX_LIMITED_RANGE") == 0 ||
                strcmp(val, "FENV_ACCESS") == 0) {
                kind = val;
            }
        }
        if (kind) {
            pp_report_diag(pp, &tokens[i], DIAG_WARNING, CDIAG_PREPROCESSOR_GENERIC,
                           "ignored #pragma STDC %s", kind);
        }
    }
    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
        i++;
    }
    *cursor = (i == 0) ? 0 : i - 1;
    return true;
}

bool process_line_directive(Preprocessor* pp,
                            const Token* tokens,
                            size_t count,
                            size_t* cursor) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    i++; // move past "line"
    if (i >= count || tokens[i].type != TOKEN_NUMBER) {
        pp_report_diag(pp, tokens ? &tokens[i] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "expected line number after #line");
        return false;
    }
    long newLine = strtol(tokens[i].value ? tokens[i].value : "0", NULL, 10);
    if (newLine < 1) newLine = 1;
    const char* newFile = NULL;
    if ((i + 1) < count &&
        tokens[i + 1].line == directiveLine &&
        tokens[i + 1].type == TOKEN_STRING) {
        newFile = tokens[i + 1].value;
        i++;
    }
    int nextPhysical = directiveLine + 1;
    pp->lineOffset = (int)newLine - nextPhysical;
    pp->lineRemapActive = true;
    if (newFile && newFile[0]) {
        pp_set_logical_file(pp, newFile);
    }
    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
        i++;
    }
    *cursor = (i == 0) ? 0 : i - 1;
    return true;
}
