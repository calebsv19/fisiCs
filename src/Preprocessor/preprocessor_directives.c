// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/pp_internal.h"
#include "Preprocessor/preprocessor_external.h"
#include "Preprocessor/preprocessor_include_support.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "Lexer/token_buffer.h"
#include "Lexer/tokens.h"
#include "Utils/profiler.h"

typedef struct {
    char** names;
    size_t count;
    size_t capacity;
    bool variadic;
    bool hasVaOpt;
} MacroParamParse;

static IncludeFile* pp_include_file_at(Preprocessor* pp, size_t includeFileIndex) {
    if (!pp || !pp->resolver) return NULL;
    if (includeFileIndex >= pp->resolver->count) return NULL;
    return &pp->resolver->files[includeFileIndex];
}

static const IncludeFile* pp_include_file_at_const(const Preprocessor* pp, size_t includeFileIndex) {
    if (!pp || !pp->resolver) return NULL;
    if (includeFileIndex >= pp->resolver->count) return NULL;
    return &pp->resolver->files[includeFileIndex];
}

static bool pp_include_file_was_included(const Preprocessor* pp, size_t includeFileIndex) {
    const IncludeFile* file = pp_include_file_at_const(pp, includeFileIndex);
    return file ? file->includedOnce : false;
}

static void pp_include_file_mark_included(Preprocessor* pp, size_t includeFileIndex) {
    IncludeFile* file = pp_include_file_at(pp, includeFileIndex);
    if (file) {
        file->includedOnce = true;
    }
}

static const char* pp_include_file_cached_guard(const Preprocessor* pp, size_t includeFileIndex) {
    const IncludeFile* file = pp_include_file_at_const(pp, includeFileIndex);
    return file ? file->cachedGuardName : NULL;
}

static void pp_include_file_cache_guard(Preprocessor* pp,
                                        size_t includeFileIndex,
                                        const char* guardName) {
    if (!guardName || !guardName[0]) return;
    IncludeFile* file = pp_include_file_at(pp, includeFileIndex);
    if (!file) return;
    if (file->cachedGuardName && strcmp(file->cachedGuardName, guardName) == 0) {
        return;
    }
    char* copy = pp_strdup_local(guardName);
    if (!copy) return;
    free(file->cachedGuardName);
    file->cachedGuardName = copy;
}

static void pp_include_file_cache_summary_probe(Preprocessor* pp,
                                                size_t includeFileIndex,
                                                IncludeSummaryProbe probe) {
    IncludeFile* file = pp_include_file_at(pp, includeFileIndex);
    if (file) {
        file->summaryProbe = probe;
    }
}

static void pp_include_file_cache_summary_actions(Preprocessor* pp,
                                                  size_t includeFileIndex,
                                                  const IncludeSummaryAction* actions,
                                                  size_t actionCount) {
    IncludeFile* file = pp_include_file_at(pp, includeFileIndex);
    if (!file) return;

    IncludeSummaryAction* copy = NULL;
    if (actions && actionCount > 0) {
        copy = (IncludeSummaryAction*)malloc(actionCount * sizeof(IncludeSummaryAction));
        if (!copy) return;
        memcpy(copy, actions, actionCount * sizeof(IncludeSummaryAction));
    }

    free(file->summaryActions);
    file->summaryActions = copy;
    file->summaryActionCount = copy ? actionCount : 0;
}

static IncludeSummaryAction* pp_include_file_summary_actions(Preprocessor* pp,
                                                             size_t includeFileIndex,
                                                             size_t* actionCountOut) {
    IncludeFile* file = pp_include_file_at(pp, includeFileIndex);
    if (!file) {
        if (actionCountOut) *actionCountOut = 0;
        return NULL;
    }
    if (actionCountOut) *actionCountOut = file->summaryActionCount;
    return file->summaryActions;
}

static bool pp_stagea_diagnostic_profile_enabled(void) {
    static int initialized = 0;
    static bool enabled = false;
    if (!initialized) {
        const char* env = getenv("FISICS_PP_STAGEA_DIAGNOSTIC_PROFILE");
        enabled = (env && env[0] && env[0] != '0');
        initialized = 1;
    }
    return enabled && profiler_counters_enabled();
}

static bool pp_summary_replay_any_enabled(void) {
    return pp_summary_replay_router_enabled() ||
           pp_summary_replay_scaffold_enabled() ||
           pp_summary_replay_general_enabled();
}

static bool pp_path_is_absolute(const char* path) {
    if (!path || !path[0]) return false;
    if (path[0] == '/') return true;
    return isalpha((unsigned char)path[0]) && path[1] == ':';
}

static bool pp_include_supports_summary_probe(IncludeSearchOrigin origin,
                                              const char* resolvedPath) {
    if (origin == INCLUDE_SEARCH_INCLUDE_PATH) {
        return true;
    }
    if (origin == INCLUDE_SEARCH_SAME_DIR) {
        return pp_summary_replay_any_enabled() &&
               pp_path_is_absolute(resolvedPath);
    }
    return false;
}

static bool pp_profile_include_path_full_token_walk(Preprocessor* pp,
                                                    const TokenBuffer* buffer,
                                                    PPTokenBuffer* output,
                                                    const char* countName,
                                                    const char* scopeName) {
    if (!pp || !buffer || !output) return false;
    if (!pp_stagea_diagnostic_profile_enabled()) {
        return preprocess_tokens(pp, buffer, output, false);
    }
    if (countName && countName[0]) {
        profiler_record_value(countName, 1);
    }
    ProfilerScope scope = {0};
    if (scopeName && scopeName[0]) {
        scope = profiler_begin(scopeName);
    }
    bool ok = preprocess_tokens(pp, buffer, output, false);
    if (scopeName && scopeName[0]) {
        profiler_end(scope);
    }
    return ok;
}

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
    bool timingEnabled = profiler_timing_enabled();
    bool counterEnabled = profiler_counters_enabled();
    char* name = NULL;
    bool isSystem = false;
    size_t lineStart = *cursor;
    int directiveLine = tokens && count > 0 ? tokens[lineStart].line : 0;
    size_t lineEnd = lineStart + 1;
    while (lineEnd < count && tokens[lineEnd].type != TOKEN_EOF && tokens[lineEnd].line == directiveLine) {
        lineEnd++;
    }

    size_t tempCursor = lineStart;
    if (parse_include_operand(tokens, count, &tempCursor, &name, &isSystem)) {
        pp_profile_event("pp_include_operand_direct");
        *cursor = tempCursor;
    } else {
        ProfilerScope operandExpandScope =
            pp_profiler_begin_if_enabled(timingEnabled, "pp_include_operand_expand_path");
        PPTokenBuffer expanded = {0};
        size_t span = (lineEnd > lineStart + 1) ? (lineEnd - (lineStart + 1)) : 0;
        pp_profiler_record_value_if_enabled(counterEnabled, "pp_count_macro_expand_calls", 1);
        pp_profiler_record_value_if_enabled(counterEnabled, "pp_count_macro_expand_input_tokens", span);
        if (!macro_expander_expand(&pp->expander, tokens + lineStart + 1, span, &expanded)) {
            pp_profiler_end_if_enabled(timingEnabled, operandExpandScope);
            pp_token_buffer_destroy(&expanded);
            pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "failed to expand #include operand");
            return false;
        }

        size_t tempCount = expanded.count + 2;
        Token* tempTokens = (Token*)calloc(tempCount, sizeof(Token));
        if (!tempTokens) {
            pp_profiler_end_if_enabled(timingEnabled, operandExpandScope);
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

        tempCursor = 0;
        if (!parse_include_operand(tempTokens, tempCount, &tempCursor, &name, &isSystem)) {
            pp_profiler_end_if_enabled(timingEnabled, operandExpandScope);
            free(tempTokens);
            pp_token_buffer_destroy(&expanded);
            pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "invalid #include operand");
            return false;
        }
        free(tempTokens);
        pp_token_buffer_destroy(&expanded);
        pp_profiler_end_if_enabled(timingEnabled, operandExpandScope);
        *cursor = (lineEnd == 0) ? 0 : lineEnd - 1;
    }

    const char* parentFile = token_file(&tokens[*cursor]);
    IncludeSearchOrigin origin = INCLUDE_SEARCH_RAW;
    size_t originIndex = (size_t)-1;
    ProfilerScope resolveScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_include_resolve");
    const IncludeFile* incPtr = include_resolver_load(pp->resolver,
                                                      parentFile,
                                                      name,
                                                      isSystem,
                                                      isIncludeNext,
                                                      &origin,
                                                      &originIndex);
    pp_profiler_end_if_enabled(timingEnabled, resolveScope);
    size_t includeFileIndex = (size_t)-1;
    bool haveInc = false;
    if (incPtr) {
        includeFileIndex = (size_t)(incPtr - pp->resolver->files);
        haveInc = true;
    }
    IncludeSummaryProbe summaryProbe = {0};
    IncludeSummaryAction* summaryActions = NULL;
    size_t summaryActionCount = 0;
    const IncludeFile* incValue = NULL;
    if (haveInc) {
        incValue = pp_include_file_at_const(pp, includeFileIndex);
        if (incValue) {
            summaryProbe = incValue->summaryProbe;
            summaryActions = incValue->summaryActions;
            summaryActionCount = incValue->summaryActionCount;
        } else {
            haveInc = false;
        }
    }

    FisicsInclude incRec = {0};
    incRec.name = name;
    incRec.kind = isSystem ? FISICS_INCLUDE_SYSTEM : FISICS_INCLUDE_LOCAL;
    incRec.line = tokens ? tokens[*cursor].line : 0;
    incRec.column = tokens ? tokens[*cursor].location.start.column : 0;
    incRec.resolved = haveInc;
    incRec.resolved_path = haveInc ? incValue->path : NULL;
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
        bool suspiciousSystemName = isSystem && include_name_is_suspicious(name);
        if (warnOnly) {
            if (suspiciousSystemName) {
                // Some SDK macro-expanded include operands are non-header expressions
                // (e.g. "(*__error()).h"); ignore in lenient/system mode.
                free(name);
                return true;
            }
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
                      incValue->path ? incValue->path : "<unknown>");

    bool nestedIncludeDispatch = pp->includeStack.depth > 1;
    bool wasIncludedBefore = pp_include_file_was_included(pp, includeFileIndex);
    pp_profiler_record_value_if_enabled(counterEnabled,
                                        wasIncludedBefore
                                            ? "pp_count_repeat_include_attempts"
                                            : "pp_count_first_include_attempts",
                                        1);
    if (nestedIncludeDispatch) {
        pp_profile_event(wasIncludedBefore
                             ? "pp_nested_include_repeat_seen"
                             : "pp_nested_include_first_seen");
    }

    if (pp->preprocessMode == PREPROCESS_HYBRID && isSystem) {
        const char* cmd = pp->externalPreprocessCmd;
        if (!cmd || !cmd[0]) {
            const char* env = getenv("FISICS_EXTERNAL_CPP");
            cmd = (env && env[0]) ? env : "clang";
        }
        const char* args = pp->externalPreprocessArgs;
        if (!args || !args[0]) {
            const char* env = getenv("FISICS_EXTERNAL_CPP_ARGS");
            args = (env && env[0]) ? env : NULL;
        }
        char* extBuf = NULL;
        size_t extLen = 0;
        ProfilerScope externalScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_include_external");
        bool externalOk = pp_run_external_preprocessor(cmd,
                                                       args,
                                                       incValue->path,
                                                       pp->includePaths,
                                                       pp->includePathCount,
                                                       &extBuf,
                                                       &extLen);
        pp_profiler_end_if_enabled(timingEnabled, externalScope);
        if (!externalOk) {
            bool warnOnly = pp->lenientMissingIncludes;
            if (warnOnly) {
                pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_WARNING, CDIAG_PREPROCESSOR_GENERIC,
                               "external preprocessing failed for system include '%s'", incValue->path ? incValue->path : "<unknown>");
                free(extBuf);
                return true;
            }
            pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                           "external preprocessing failed for system include '%s'", incValue->path ? incValue->path : "<unknown>");
            free(extBuf);
            return false;
        }

        ProfilerScope externalLexScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_include_external_lex");
        Lexer extLexer;
        initLexer(&extLexer, extBuf, incValue->path, pp->enableTrigraphs);
        TokenBuffer extTokens;
        token_buffer_init(&extTokens);
        bool lexOk = token_buffer_fill_from_lexer(&extTokens, &extLexer);
        destroyLexer(&extLexer);
        if (lexOk) {
            for (size_t i = 0; i < extTokens.count; ++i) {
                if (extTokens.tokens[i].type == TOKEN_EOF) break;
                if (!pp_token_buffer_append_clone(output, &extTokens.tokens[i])) {
                    lexOk = false;
                    break;
                }
            }
        }
        token_buffer_destroy(&extTokens);
        pp_profiler_end_if_enabled(timingEnabled, externalLexScope);
        free(extBuf);
        pp_include_file_mark_included(pp, includeFileIndex);
        return lexOk;
    }

    if (incValue->pragmaOnce && wasIncludedBefore) {
        pp_profiler_record_value_if_enabled(counterEnabled, "pp_count_include_short_circuit_pragma_once", 1);
        if (nestedIncludeDispatch) {
            pp_profile_event("pp_nested_include_repeat_pragma_once_skip");
        }
        return true;
    }

    const char* cachedGuard = pp_include_file_cached_guard(pp, includeFileIndex);
    if (cachedGuard && macro_table_lookup(pp->table, cachedGuard) != NULL) {
        pp_profiler_record_value_if_enabled(counterEnabled, "pp_count_include_short_circuit_cached_guard", 1);
        if (nestedIncludeDispatch) {
            pp_profile_event(wasIncludedBefore
                                 ? "pp_nested_include_repeat_cached_guard_skip"
                                 : "pp_nested_include_first_cached_guard_skip");
        }
        return true;
    }

    bool includeWouldRecurse = pp_include_stack_contains(pp, incValue->path);
    bool summaryProbeSupported = pp_include_supports_summary_probe(origin, incValue->path);

    int savedOffset = pp->lineOffset;
    char* savedLogical = pp->logicalFile;
    bool savedRemap = pp->lineRemapActive;
    pp->lineOffset = 0;
    pp->lineRemapActive = false;
    pp->logicalFile = NULL;

    ProfilerScope lexScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_include_lex");
    Lexer lexer;
    initLexer(&lexer, incValue->contents, incValue->path, pp->enableTrigraphs);
    TokenBuffer buffer;
    token_buffer_init(&buffer);
    if (!token_buffer_fill_from_lexer(&buffer, &lexer)) {
        token_buffer_destroy(&buffer);
        destroyLexer(&lexer);
        pp_profiler_end_if_enabled(timingEnabled, lexScope);
        pp->lineOffset = savedOffset;
        pp->logicalFile = savedLogical;
        pp->lineRemapActive = savedRemap;
        return false;
    }
    destroyLexer(&lexer);
    pp_profiler_end_if_enabled(timingEnabled, lexScope);

    const char* guard = cachedGuard;
    bool deferGuardDetection = !guard &&
                               summaryProbeSupported &&
                               summaryProbe.status == INCLUDE_SUMMARY_PROBE_UNKNOWN;
    if (!guard && !deferGuardDetection) {
        ProfilerScope guardScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_include_guard_check");
        guard = detect_include_guard(&buffer);
        pp_profiler_end_if_enabled(timingEnabled, guardScope);
        if (guard) {
            pp_profiler_record_value_if_enabled(counterEnabled, "pp_count_guarded_headers_detected", 1);
            pp_include_file_cache_guard(pp, includeFileIndex, guard);
        }
    }
    if (summaryProbeSupported &&
        summaryProbe.status == INCLUDE_SUMMARY_PROBE_UNKNOWN) {
        IncludeSummaryAction* builtSummaryActions = NULL;
        size_t builtSummaryActionCount = 0;
        ProfilerScope summaryProbeScope =
            pp_profiler_begin_if_enabled(timingEnabled, "pp_include_summary_probe_scan");
        summaryProbe = analyze_include_summary_probe(&buffer,
                                                    &builtSummaryActions,
                                                    &builtSummaryActionCount);
        if (!guard) {
            ProfilerScope guardScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_include_guard_check");
            guard = detect_include_guard_from_summary_actions(&buffer,
                                                             builtSummaryActions,
                                                             builtSummaryActionCount);
            if (!guard) {
                guard = detect_include_guard(&buffer);
            }
            pp_profiler_end_if_enabled(timingEnabled, guardScope);
            if (guard) {
                pp_profiler_record_value_if_enabled(counterEnabled, "pp_count_guarded_headers_detected", 1);
                pp_include_file_cache_guard(pp, includeFileIndex, guard);
            }
        }
        if (summaryProbe.status == INCLUDE_SUMMARY_PROBE_CANDIDATE) {
            summaryProbe.routerOnly = classify_include_summary_router_only(&buffer,
                                                                           builtSummaryActions,
                                                                           builtSummaryActionCount,
                                                                           guard,
                                                                           &summaryProbe);
            if (!summaryProbe.routerOnly) {
                summaryProbe.routerRawTail = classify_include_summary_router_raw_tail(&buffer,
                                                                                      builtSummaryActions,
                                                                                      builtSummaryActionCount,
                                                                                      guard,
                                                                                      &summaryProbe);
            }
            summaryProbe.behaviorClass =
                classify_include_summary_behavior_class(pp,
                                                        &buffer,
                                                        builtSummaryActions,
                                                        builtSummaryActionCount,
                                                        guard,
                                                        &summaryProbe);
        }
        pp_profiler_end_if_enabled(timingEnabled, summaryProbeScope);
        pp_include_file_cache_summary_probe(pp, includeFileIndex, summaryProbe);
        if (summaryProbe.status == INCLUDE_SUMMARY_PROBE_CANDIDATE) {
            pp_include_file_cache_summary_actions(pp,
                                                  includeFileIndex,
                                                  builtSummaryActions,
                                                  builtSummaryActionCount);
        }
        free(builtSummaryActions);
        summaryActions = pp_include_file_summary_actions(pp,
                                                         includeFileIndex,
                                                         &summaryActionCount);
        pp_profile_summary_probe_scan_result(&summaryProbe);
    }
    if (summaryProbeSupported) {
        pp_profile_summary_probe(&summaryProbe);
        pp_profile_behavior_class(&summaryProbe);
        if (summaryProbe.status == INCLUDE_SUMMARY_PROBE_CANDIDATE &&
            summaryProbe.behaviorClass == INCLUDE_HEADER_BEHAVIOR_CONDITIONAL_SCAFFOLD) {
            pp_profiler_record_value_if_enabled(counterEnabled, "pp_conditional_scaffold_profile_entered", 1);
            pp_profiler_record_value_if_enabled(counterEnabled, "pp_conditional_scaffold_action_count_seen", (long long)summaryActionCount);
            if (!summaryActions || summaryActionCount == 0) {
                pp_profiler_record_value_if_enabled(counterEnabled, "pp_conditional_scaffold_missing_actions", 1);
            }
        }
        pp_profile_conditional_scaffold_shape(pp,
                                              &buffer,
                                              &summaryProbe,
                                              summaryActions,
                                              summaryActionCount,
                                              guard);
    }
    if (guard && macro_table_lookup(pp->table, guard) != NULL) {
        pp_profiler_record_value_if_enabled(counterEnabled, "pp_count_include_short_circuit_guard_after_lex", 1);
        if (nestedIncludeDispatch) {
            pp_profile_event(wasIncludedBefore
                                 ? "pp_nested_include_repeat_guard_skip"
                                 : "pp_nested_include_first_guard_skip");
        }
        token_buffer_destroy(&buffer);
        pp_include_file_mark_included(pp, includeFileIndex);
        pp->lineOffset = savedOffset;
        pp->logicalFile = savedLogical;
        pp->lineRemapActive = savedRemap;
        return true;
    }

    if (includeWouldRecurse) {
        if (nestedIncludeDispatch) {
            pp_profile_event("pp_nested_include_recursive_cycle");
        }
        token_buffer_destroy(&buffer);
        pp->lineOffset = savedOffset;
        pp->logicalFile = savedLogical;
        pp->lineRemapActive = savedRemap;
        if (pp->lenientMissingIncludes) {
            return true;
        }
        pp_report_diag(pp,
                       tokens ? &tokens[*cursor] : NULL,
                       DIAG_ERROR,
                       CDIAG_PREPROCESSOR_GENERIC,
                       "detected recursive include of '%s'",
                       incValue->path ? incValue->path : "<unknown>");
        return false;
    }

    bool pushedFrame = pp_include_stack_push(pp, incValue->path, origin, originIndex);

    ProfilerScope recurseScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_include_recurse");
    ProfilerScope nestedOriginScope = {0};
    if (timingEnabled && nestedIncludeDispatch) {
        nestedOriginScope = profiler_begin(nested_recurse_scope_name(wasIncludedBefore, origin));
    }
    bool ok = false;
    if (summaryProbeSupported &&
        pp_summary_replay_router_enabled() &&
        summaryProbe.status == INCLUDE_SUMMARY_PROBE_CANDIDATE &&
        (summaryProbe.behaviorClass == INCLUDE_HEADER_BEHAVIOR_ROUTER_ONLY ||
         summaryProbe.behaviorClass == INCLUDE_HEADER_BEHAVIOR_ROUTER_RAW_TAIL)) {
        PPSummaryReplayResult replayResult =
            preprocess_tokens_router_replay(pp,
                                            &buffer,
                                            &summaryProbe,
                                            summaryActions,
                                            summaryActionCount,
                                            output,
                                            false);
        if (replayResult == PP_SUMMARY_REPLAY_USED) {
            pp_profile_event("pp_include_router_replay_used");
            if (summaryProbe.routerRawTail) {
                pp_profile_event("pp_include_router_raw_tail_replay_used");
            }
            ok = true;
        } else if (replayResult == PP_SUMMARY_REPLAY_UNSUPPORTED) {
            pp_profile_event("pp_include_router_replay_fallback");
            if (pp_stagea_diagnostic_profile_enabled()) {
                profiler_record_value("pp_count_include_path_replay_unsupported_fallback", 1);
                profiler_record_value("pp_count_include_path_router_replay_fallback", 1);
            }
            ok = pp_profile_include_path_full_token_walk(
                pp,
                &buffer,
                output,
                NULL,
                "pp_recurse_include_path_router_replay_fallback_full_walk");
        } else {
            pp_profile_event("pp_include_router_replay_error");
            ok = false;
        }
    } else if (summaryProbeSupported &&
               pp_summary_replay_scaffold_enabled() &&
               summaryProbe.status == INCLUDE_SUMMARY_PROBE_CANDIDATE &&
               summaryProbe.behaviorClass == INCLUDE_HEADER_BEHAVIOR_INCLUDE_DEFINE_SCAFFOLD) {
        PPSummaryReplayResult replayResult =
            preprocess_tokens_scaffold_replay(pp,
                                              &buffer,
                                              &summaryProbe,
                                              summaryActions,
                                              summaryActionCount,
                                              output,
                                              false);
        if (replayResult == PP_SUMMARY_REPLAY_USED) {
            pp_profiler_record_value_if_enabled(counterEnabled, "pp_include_behavior_class_scaffold_replay", 1);
            ok = true;
        } else if (replayResult == PP_SUMMARY_REPLAY_UNSUPPORTED) {
            pp_profiler_record_value_if_enabled(counterEnabled, "pp_include_behavior_class_scaffold_fallback", 1);
            if (pp_stagea_diagnostic_profile_enabled()) {
                profiler_record_value("pp_count_include_path_replay_unsupported_fallback", 1);
                profiler_record_value("pp_count_include_path_scaffold_replay_fallback", 1);
            }
            ok = pp_profile_include_path_full_token_walk(
                pp,
                &buffer,
                output,
                NULL,
                "pp_recurse_include_path_scaffold_replay_fallback_full_walk");
        } else {
            pp_profile_event("pp_include_scaffold_replay_error");
            ok = false;
        }
    } else if (summaryProbeSupported &&
        summaryProbe.status == INCLUDE_SUMMARY_PROBE_CANDIDATE &&
        pp_summary_replay_general_enabled()) {
        PPSummaryReplayResult replayResult =
            preprocess_tokens_summary_replay(pp,
                                             &buffer,
                                             summaryActions,
                                             summaryActionCount,
                                             output,
                                             false);
        if (replayResult == PP_SUMMARY_REPLAY_USED) {
            pp_profile_event("pp_include_summary_replay_used");
            ok = true;
        } else if (replayResult == PP_SUMMARY_REPLAY_UNSUPPORTED) {
            pp_profile_event("pp_include_summary_replay_fallback");
            if (pp_stagea_diagnostic_profile_enabled()) {
                profiler_record_value("pp_count_include_path_replay_unsupported_fallback", 1);
                profiler_record_value("pp_count_include_path_summary_replay_fallback", 1);
            }
            ok = pp_profile_include_path_full_token_walk(
                pp,
                &buffer,
                output,
                NULL,
                "pp_recurse_include_path_summary_replay_fallback_full_walk");
        } else {
            pp_profile_event("pp_include_summary_replay_error");
            ok = false;
        }
    } else {
        if (summaryProbeSupported) {
            if (pp_stagea_diagnostic_profile_enabled()) {
                profiler_record_value("pp_count_include_path_full_token_walk", 1);
            }
            ok = pp_profile_include_path_full_token_walk(
                pp,
                &buffer,
                output,
                NULL,
                "pp_recurse_include_path_full_token_walk");
        } else {
            ok = preprocess_tokens(pp, &buffer, output, false);
        }
    }
    if (timingEnabled && nestedIncludeDispatch) {
        profiler_end(nestedOriginScope);
    }
    pp_profiler_end_if_enabled(timingEnabled, recurseScope);
    if (!ok) {
        pp_debug_fail("process_include_body", tokens ? &tokens[*cursor] : NULL);
    }
    token_buffer_destroy(&buffer);
    pp->lineOffset = savedOffset;
    pp->logicalFile = savedLogical;
    pp->lineRemapActive = savedRemap;
    pp_include_file_mark_included(pp, includeFileIndex);
    if (pushedFrame) pp_include_stack_pop(pp);
    return ok;
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
