// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/pp_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Preprocessor/macro_expander.h"

#include "Utils/profiler.h"

bool pp_append_directive_line(const Token* tokens,
                              size_t count,
                              size_t cursor,
                              PPTokenBuffer* output,
                              Preprocessor* pp) {
    if (!tokens || !output) return false;
    int line = tokens[cursor].line;
    size_t end = cursor;
    while (end < count && tokens[end].type != TOKEN_EOF && tokens[end].line == line) {
        end++;
    }
    return pp_append_directive_span(tokens, cursor, end, output, pp);
}

bool pp_append_directive_span(const Token* tokens,
                              size_t start,
                              size_t end,
                              PPTokenBuffer* output,
                              Preprocessor* pp) {
    if (!tokens || !output || end <= start) return false;
    return pp_token_buffer_append_clone_remap_span(output, pp, tokens + start, end - start);
}

bool pp_directive_span_has_trailing_tokens(size_t start,
                                           size_t end) {
    return end > start + 1;
}

bool pp_directive_has_trailing_tokens(const Token* tokens,
                                      size_t count,
                                      size_t cursor) {
    if (!tokens || cursor >= count) return false;
    int line = tokens[cursor].line;
    size_t end = cursor + 1;
    while (end < count && tokens[end].type != TOKEN_EOF && tokens[end].line == line) {
        end++;
    }
    return pp_directive_span_has_trailing_tokens(cursor, end);
}

bool pp_skip_pragma_operator(const Token* tokens,
                             size_t count,
                             size_t* cursor) {
    if (!tokens || !cursor) return false;
    size_t i = *cursor;
    const Token* tok = &tokens[i];
    if (tok->type != TOKEN_IDENTIFIER || !tok->value) {
        return false;
    }
    if (strcmp(tok->value, "_Pragma") != 0) {
        return false;
    }
    size_t j = i + 1;
    if (j < count && tokens[j].type == TOKEN_LPAREN) {
        j++;
        if (j < count && tokens[j].type == TOKEN_STRING) {
            j++;
        }
        if (j < count && tokens[j].type == TOKEN_RPAREN) {
            j++;
        }
    }
    if (j > i) {
        *cursor = j - 1;
    }
    return true;
}

static void pp_report_macro_trace_diag(Preprocessor* pp,
                                       SourceRange callSite,
                                       const char* macroName,
                                       SourceRange definitionRange,
                                       const char* message) {
    const char* macro_names[2] = { macroName, macroName };
    const char* macro_roles[2] = { "call_site", "definition" };
    SourceRange macro_ranges[2] = { callSite, definitionRange };
    size_t macro_count = 1u;
    if (definitionRange.start.file || definitionRange.start.line > 0 || definitionRange.start.column > 0) {
        macro_count = 2u;
    }
    compiler_report_diag_with_macro_trace(pp->ctx,
                                          callSite,
                                          DIAG_ERROR,
                                          CDIAG_PREPROCESSOR_GENERIC,
                                          NULL,
                                          macro_names,
                                          macro_roles,
                                          macro_ranges,
                                          macro_count,
                                          "%s",
                                          message ? message : "macro expansion failed");
}

static bool pp_flush_chunk(Preprocessor* pp,
                           PPTokenBuffer* chunk,
                           PPTokenBuffer* output) {
    if (!chunk || chunk->count == 0) {
        return true;
    }
    bool timingEnabled = profiler_timing_enabled();
    bool counterEnabled = profiler_counters_enabled();
    PPTokenBuffer expanded = {0};
    ProfilerScope scope = pp_profiler_begin_if_enabled(timingEnabled, "pp_expand");
    pp_profiler_record_value_if_enabled(counterEnabled, "pp_count_macro_expand_calls", 1);
    pp_profiler_record_value_if_enabled(counterEnabled, "pp_count_macro_expand_input_tokens", chunk->count);
    if (!macro_expander_expand(&pp->expander, chunk->tokens, chunk->count, &expanded)) {
        pp_profiler_end_if_enabled(timingEnabled, scope);
        MacroExpandErrorInfo expandErr = macro_expander_last_error(&pp->expander);
        if (expandErr.kind == ME_ERR_MACRO_ARG_COUNT) {
            const char* macroName = (expandErr.macro && expandErr.macro->name)
                ? expandErr.macro->name
                : "<macro>";
            char diagMessage[256];
            if (expandErr.variadic) {
                snprintf(diagMessage,
                         sizeof(diagMessage),
                         "macro '%s' requires at least %zu argument(s), got %zu",
                         macroName,
                         expandErr.expectedArgs,
                         expandErr.providedArgs);
                pp_report_macro_trace_diag(pp,
                                           expandErr.callSite,
                                           macroName,
                                           expandErr.macro ? expandErr.macro->definitionRange : (SourceRange){0},
                                           diagMessage);
                fprintf(stderr, "%s:%d:%d: error: macro '%s' requires at least %zu argument(s), got %zu\n",
                        expandErr.callSite.start.file ? expandErr.callSite.start.file : "<unknown>",
                        expandErr.callSite.start.line,
                        expandErr.callSite.start.column,
                        macroName,
                        expandErr.expectedArgs,
                        expandErr.providedArgs);
            } else {
                snprintf(diagMessage,
                         sizeof(diagMessage),
                         "macro '%s' requires %zu argument(s), got %zu",
                         macroName,
                         expandErr.expectedArgs,
                         expandErr.providedArgs);
                pp_report_macro_trace_diag(pp,
                                           expandErr.callSite,
                                           macroName,
                                           expandErr.macro ? expandErr.macro->definitionRange : (SourceRange){0},
                                           diagMessage);
                fprintf(stderr, "%s:%d:%d: error: macro '%s' requires %zu argument(s), got %zu\n",
                        expandErr.callSite.start.file ? expandErr.callSite.start.file : "<unknown>",
                        expandErr.callSite.start.line,
                        expandErr.callSite.start.column,
                        macroName,
                        expandErr.expectedArgs,
                        expandErr.providedArgs);
            }
            pp_token_buffer_destroy(&expanded);
            return false;
        }
        if (expandErr.kind == ME_ERR_UNSUPPORTED_GNU_COMMA_VA_ARGS) {
            const char* macroName = (expandErr.macro && expandErr.macro->name)
                ? expandErr.macro->name
                : "<macro>";
            pp_report_macro_trace_diag(pp,
                                       expandErr.callSite,
                                       macroName,
                                       expandErr.macro ? expandErr.macro->definitionRange : (SourceRange){0},
                                       "GNU ', ##__VA_ARGS__' extension is not supported");
            fprintf(stderr, "%s:%d:%d: error: GNU ', ##__VA_ARGS__' extension is not supported\n",
                    expandErr.callSite.start.file ? expandErr.callSite.start.file : "<unknown>",
                    expandErr.callSite.start.line,
                    expandErr.callSite.start.column);
            pp_token_buffer_destroy(&expanded);
            return false;
        }
        const MacroExpansionFrame* top = NULL;
        MacroExpansionError err = macro_table_last_error(pp->table, &top);
        if (err == MT_ERR_RECURSION || err == MT_ERR_DEPTH || err == MT_ERR_NONE) {
            const char* macroName = (top && top->macro && top->macro->name) ? top->macro->name : "<macro>";
            SourceRange loc = top ? top->callSiteRange : (SourceRange){0};
            const char* msg = "macro recursion detected while expanding '%s'";
            if (err == MT_ERR_DEPTH) {
                msg = "macro expansion depth exceeded (possible recursion) for '%s'";
            } else if (err == MT_ERR_NONE) {
                msg = "macro expansion failed (possible recursion) for '%s'";
            }
            char buf[256];
            snprintf(buf, sizeof(buf), msg, macroName);
            if (top) {
                pp_report_macro_trace_diag(pp,
                                           loc,
                                           macroName,
                                           top->macro ? top->macro->definitionRange : (SourceRange){0},
                                           buf);
            } else {
                pp_report_diag(pp,
                               NULL,
                               DIAG_ERROR,
                               CDIAG_PREPROCESSOR_GENERIC,
                               "%s",
                               buf);
            }
            const char* path = loc.start.file ? loc.start.file : "<unknown>";
            fprintf(stderr, "%s:%d:%d: error: %s\n",
                    path,
                    loc.start.line,
                    loc.start.column,
                    buf);
        }
        pp_token_buffer_destroy(&expanded);
        return false;
    }
    pp_profiler_end_if_enabled(timingEnabled, scope);
    bool ok = true;
    size_t spanStart = 0;
    for (size_t j = 0; j < expanded.count; ++j) {
        size_t cursor = j;
        bool skipPragma = pp_skip_pragma_operator(expanded.tokens, expanded.count, &cursor);
        bool skipUnknown = expanded.tokens[j].type == TOKEN_UNKNOWN;
        if (!skipPragma && !skipUnknown) {
            continue;
        }
        if (j > spanStart &&
            !pp_token_buffer_append_clone_remap_span(output,
                                                     pp,
                                                     expanded.tokens + spanStart,
                                                     j - spanStart)) {
            ok = false;
            break;
        }
        spanStart = skipPragma ? cursor + 1 : j + 1;
        if (skipPragma) {
            j = cursor;
        }
    }
    if (ok && expanded.count > spanStart) {
        ok = pp_token_buffer_append_clone_remap_span(output,
                                                     pp,
                                                     expanded.tokens + spanStart,
                                                     expanded.count - spanStart);
    }
    pp_token_buffer_destroy(&expanded);
    pp_token_buffer_destroy(chunk);
    pp_token_buffer_init_local(chunk);
    return ok;
}

bool pp_flush_chunk_profiled(Preprocessor* pp,
                             PPTokenBuffer* chunk,
                             PPTokenBuffer* output,
                             bool nestedIncludeBody,
                             bool includePathBody) {
    if (!chunk || chunk->count == 0) {
        return true;
    }
    bool timingEnabled = profiler_timing_enabled();
    ProfilerScope nestedScope = {0};
    ProfilerScope includePathScope = {0};
    if (timingEnabled && nestedIncludeBody) {
        nestedScope = profiler_begin("pp_recurse_flush");
    }
    if (timingEnabled && includePathBody) {
        includePathScope = profiler_begin("pp_recurse_include_path_flush");
    }
    bool ok = pp_flush_chunk(pp, chunk, output);
    if (timingEnabled && includePathBody) {
        profiler_end(includePathScope);
    }
    if (timingEnabled && nestedIncludeBody) {
        profiler_end(nestedScope);
    }
    return ok;
}

size_t pp_count_line_tokens_from_cursor(const Token* tokens,
                                        size_t count,
                                        size_t cursor) {
    if (!tokens || cursor >= count) return 0;
    int line = tokens[cursor].line;
    size_t total = 0;
    while (cursor < count && tokens[cursor].type != TOKEN_EOF && tokens[cursor].line == line) {
        total++;
        cursor++;
    }
    return total;
}

bool pp_debug_layout_enabled(void) {
    static int initialized = 0;
    static bool enabled = false;
    if (!initialized) {
        const char* env = getenv("FISICS_DEBUG_LAYOUT");
        enabled = (env && env[0] && env[0] != '0');
        initialized = 1;
    }
    return enabled;
}
