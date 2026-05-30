// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/pp_internal.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "Utils/profiler.h"

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

static bool token_is_macro_identifier(const Token* tok) {
    if (!tok || !tok->value) return false;
    if (tok->type == TOKEN_IDENTIFIER) return true;
    return tok->type < TOKEN_IDENTIFIER; // keyword tokens remain valid macro names in PP context
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

bool process_define(Preprocessor* pp,
                    const Token* tokens,
                    size_t count,
                    size_t* cursor) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    i++;
    if (i >= count || tokens[i].line != directiveLine || !token_is_macro_identifier(&tokens[i])) {
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
    if (i >= count || tokens[i].line != directiveLine || !token_is_macro_identifier(&tokens[i])) {
        pp_report_diag(pp, tokens ? &tokens[i] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "expected identifier after #undef");
        return false;
    }
    macro_table_undef(pp->table, tokens[i].value);
    i++;
    if (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
        pp_report_diag(pp,
                       &tokens[i],
                       DIAG_ERROR,
                       CDIAG_PREPROCESSOR_GENERIC,
                       "unexpected tokens after #undef directive");
        return false;
    }
    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) i++;
    *cursor = (i == 0) ? 0 : i - 1;
    return true;
}

bool process_pragma(Preprocessor* pp,
                    const Token* tokens,
                    size_t count,
                    size_t* cursor) {
    bool counterEnabled = profiler_counters_enabled();
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    i++;
    if (i < count && tokens[i].type == TOKEN_ONCE) {
        pp_profiler_record_value_if_enabled(counterEnabled, "pp_count_pragma_once_headers_detected", 1);
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
    size_t lineStart = i;
    size_t lineEnd = lineStart + 1;
    while (lineEnd < count && tokens[lineEnd].type != TOKEN_EOF && tokens[lineEnd].line == directiveLine) {
        lineEnd++;
    }
    size_t span = (lineEnd > lineStart + 1) ? (lineEnd - (lineStart + 1)) : 0;

    PPTokenBuffer expanded = {0};
    bool counterEnabled = profiler_counters_enabled();
    pp_profiler_record_value_if_enabled(counterEnabled, "pp_count_macro_expand_calls", 1);
    pp_profiler_record_value_if_enabled(counterEnabled, "pp_count_macro_expand_input_tokens", span);
    if (!macro_expander_expand(&pp->expander, tokens + lineStart + 1, span, &expanded)) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                       "failed to expand #line directive");
        return false;
    }

    size_t cursorExp = 0;
    while (cursorExp < expanded.count && expanded.tokens[cursorExp].type == TOKEN_EOF) {
        cursorExp++;
    }
    if (cursorExp >= expanded.count || expanded.tokens[cursorExp].type != TOKEN_NUMBER) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                       "expected line number after #line");
        return false;
    }
    const char* lineText = expanded.tokens[cursorExp].value ? expanded.tokens[cursorExp].value : "";
    if (!lineText[0]) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                       "expected line number after #line");
        return false;
    }
    for (const unsigned char* p = (const unsigned char*)lineText; *p; ++p) {
        if (!isdigit(*p)) {
            pp_token_buffer_destroy(&expanded);
            pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                           "line number after #line must use decimal digits");
            return false;
        }
    }
    errno = 0;
    char* end = NULL;
    unsigned long parsedLine = strtoul(lineText, &end, 10);
    if (errno == ERANGE || !end || *end != '\0' || parsedLine > (unsigned long)INT_MAX) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                       "line number after #line is out of range");
        return false;
    }
    if (parsedLine == 0UL) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                       "line number after #line must be positive");
        return false;
    }
    long newLine = (long)parsedLine;
    cursorExp++;

    const char* newFile = NULL;
    while (cursorExp < expanded.count && expanded.tokens[cursorExp].type == TOKEN_EOF) {
        cursorExp++;
    }
    if (cursorExp < expanded.count && expanded.tokens[cursorExp].type == TOKEN_STRING) {
        newFile = expanded.tokens[cursorExp].value;
        cursorExp++;
    }
    while (cursorExp < expanded.count && expanded.tokens[cursorExp].type == TOKEN_EOF) {
        cursorExp++;
    }
    if (cursorExp < expanded.count) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                       "unexpected tokens after #line directive");
        return false;
    }

    int nextPhysical = directiveLine + 1;
    pp->lineOffset = (int)newLine - nextPhysical;
    pp->lineRemapActive = true;
    if (newFile && newFile[0]) {
        pp_set_logical_file(pp, newFile);
    }

    pp_token_buffer_destroy(&expanded);

    *cursor = (lineEnd == 0) ? 0 : lineEnd - 1;
    return true;
}
