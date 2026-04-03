#include "Preprocessor/preprocessor.h"

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Preprocessor/pp_internal.h"

#include "Lexer/lexer.h"
#include "Lexer/token_buffer.h"
#include "Lexer/tokens.h"
#include "Compiler/diagnostics.h"
#include "Utils/logging.h"
#include "Utils/profiler.h"

static bool append_directive_line(const Token* tokens,
                                  size_t count,
                                  size_t cursor,
                                  PPTokenBuffer* output,
                                  Preprocessor* pp) {
    if (!tokens || !output) return false;
    int line = tokens[cursor].line;
    size_t i = cursor;
    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == line) {
        if (!pp_token_buffer_append_clone_remap(output, pp, &tokens[i])) {
            return false;
        }
        i++;
    }
    return true;
}

static bool directive_has_trailing_tokens(const Token* tokens,
                                          size_t count,
                                          size_t cursor) {
    if (!tokens || cursor >= count) return false;
    int line = tokens[cursor].line;
    size_t i = cursor + 1;
    return (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == line);
}

// Consume a _Pragma("...") operator; returns true if handled (skipped).
static bool skip_pragma_operator(const Token* tokens,
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
    // Advance caller to last token we looked at so the for-loop increments past it.
    if (j > i) {
        *cursor = j - 1;
    }
    return true;
}

static bool tokenize_macro_replacement(const char* value, PPTokenBuffer* out) {
    if (!out) return false;
    pp_token_buffer_init_local(out);
    if (!value || value[0] == '\0') {
        return true;
    }

    Lexer lexer;
    initLexer(&lexer, value, "<command-line-macro>", false);

    TokenBuffer raw = {0};
    token_buffer_init(&raw);
    bool ok = token_buffer_fill_from_lexer(&raw, &lexer);
    destroyLexer(&lexer);
    if (!ok) {
        token_buffer_destroy(&raw);
        return false;
    }

    for (size_t i = 0; i < raw.count; ++i) {
        if (raw.tokens[i].type == TOKEN_EOF) break;
        if (!pp_token_buffer_append_clone(out, &raw.tokens[i])) {
            ok = false;
            break;
        }
    }
    token_buffer_destroy(&raw);
    if (!ok) {
        pp_token_buffer_reset(out);
    }
    return ok;
}

static char* normalize_cli_macro_value(const char* value) {
    if (!value) return NULL;
    size_t len = strlen(value);
    char* normalized = (char*)malloc(len + 1);
    if (!normalized) return NULL;

    size_t w = 0;
    for (size_t i = 0; i < len; ++i) {
        if (value[i] == '\\') {
            size_t j = i;
            while (j < len && value[j] == '\\') {
                ++j;
            }
            if (j < len && value[j] == '"') {
                normalized[w++] = '"';
                i = j;
                continue;
            }
        }
        normalized[w++] = value[i];
    }
    normalized[w] = '\0';
    return normalized;
}

static bool try_tokenize_cli_quoted_string(const char* value, PPTokenBuffer* out) {
    if (!value || !out) return false;
    size_t len = strlen(value);
    if (len < 2) return false;
    if (value[0] != '"' || value[len - 1] != '"') return false;

    Token tok = {0};
    tok.type = TOKEN_STRING;
    tok.value = (char*)malloc(len - 1);
    if (!tok.value) return false;
    memcpy(tok.value, value + 1, len - 2);
    tok.value[len - 2] = '\0';
    if (!pp_token_buffer_append_token(out, tok)) {
        pp_token_free(&tok);
        return false;
    }
    return true;
}

static void define_builtin_object(Preprocessor* pp, const char* name, const char* value) {
    if (!pp || !name) return;
    PPTokenBuffer body = {0};
    if (!tokenize_macro_replacement(value, &body)) {
        return;
    }
    macro_table_define_object(pp->table, name, body.tokens, body.count, (SourceRange){0});
    pp_token_buffer_reset(&body);
}

static void define_builtin_function(Preprocessor* pp,
                                    const char* name,
                                    const char* const* params,
                                    size_t paramCount,
                                    bool variadic,
                                    const char* value) {
    if (!pp || !name) return;
    PPTokenBuffer body = {0};
    if (!tokenize_macro_replacement(value, &body)) {
        return;
    }
    macro_table_define_function(pp->table,
                                name,
                                params,
                                paramCount,
                                variadic,
                                false,
                                body.tokens,
                                body.count,
                                (SourceRange){0});
    pp_token_buffer_reset(&body);
}

// Define a macro that expands to nothing for both object-like and variadic function-like
// forms (to swallow attribute-style uses with parentheses/arguments).
static void define_builtin_empty_macro(Preprocessor* pp, const char* name) {
    define_builtin_object(pp, name, "");
    macro_table_define_function(pp->table,
                                name,
                                NULL,
                                0,
                                true,   // variadic to accept any arg list
                                false,
                                NULL,
                                0,
                                (SourceRange){0});
}

static bool flush_chunk(Preprocessor* pp,
                        PPTokenBuffer* chunk,
                        PPTokenBuffer* output) {
    if (!chunk || chunk->count == 0) {
        return true;
    }
    PPTokenBuffer expanded = {0};
    ProfilerScope scope = profiler_begin("pp_expand");
    if (!macro_expander_expand(&pp->expander, chunk->tokens, chunk->count, &expanded)) {
        profiler_end(scope);
        MacroExpandErrorInfo expandErr = macro_expander_last_error(&pp->expander);
        if (expandErr.kind == ME_ERR_MACRO_ARG_COUNT) {
            const char* macroName = (expandErr.macro && expandErr.macro->name)
                ? expandErr.macro->name
                : "<macro>";
            Token tmp = {0};
            tmp.location = expandErr.callSite;
            if (expandErr.variadic) {
                pp_report_diag(pp,
                               &tmp,
                               DIAG_ERROR,
                               CDIAG_PREPROCESSOR_GENERIC,
                               "macro '%s' requires at least %zu argument(s), got %zu",
                               macroName,
                               expandErr.expectedArgs,
                               expandErr.providedArgs);
                fprintf(stderr, "%s:%d:%d: error: macro '%s' requires at least %zu argument(s), got %zu\n",
                        expandErr.callSite.start.file ? expandErr.callSite.start.file : "<unknown>",
                        expandErr.callSite.start.line,
                        expandErr.callSite.start.column,
                        macroName,
                        expandErr.expectedArgs,
                        expandErr.providedArgs);
            } else {
                pp_report_diag(pp,
                               &tmp,
                               DIAG_ERROR,
                               CDIAG_PREPROCESSOR_GENERIC,
                               "macro '%s' requires %zu argument(s), got %zu",
                               macroName,
                               expandErr.expectedArgs,
                               expandErr.providedArgs);
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
        const MacroExpansionFrame* top = NULL;
        MacroExpansionError err = macro_table_last_error(pp->table, &top);
        if (err == MT_ERR_RECURSION || err == MT_ERR_DEPTH || err == MT_ERR_NONE) {
            const char* macroName = (top && top->macro && top->macro->name) ? top->macro->name : "<macro>";
            SourceRange loc = top ? top->callSiteRange : (SourceRange){0};
            Token tmp = {0};
            tmp.location = loc;
            const char* msg = "macro recursion detected while expanding '%s'";
            if (err == MT_ERR_DEPTH) {
                msg = "macro expansion depth exceeded (possible recursion) for '%s'";
            } else if (err == MT_ERR_NONE) {
                msg = "macro expansion failed (possible recursion) for '%s'";
            }
            char buf[256];
            snprintf(buf, sizeof(buf), msg, macroName);
            pp_report_diag(pp,
                           top ? &tmp : NULL,
                           DIAG_ERROR,
                           CDIAG_PREPROCESSOR_GENERIC,
                           "%s",
                           buf);
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
    profiler_end(scope);
    bool ok = true;
    for (size_t j = 0; j < expanded.count; ++j) {
        size_t cursor = j;
        if (skip_pragma_operator(expanded.tokens, expanded.count, &cursor)) {
            j = cursor;
            continue;
        }
        if (expanded.tokens[j].type == TOKEN_UNKNOWN) {
            continue; // drop poison tokens such as stray backslashes that survive lexing
        }
        if (!pp_token_buffer_append_clone_remap(output, pp, &expanded.tokens[j])) {
            ok = false;
            break;
        }
    }
    pp_token_buffer_destroy(&expanded);
    pp_token_buffer_destroy(chunk);
    pp_token_buffer_init_local(chunk);
    return ok;
}

static bool debug_layout_enabled(void) {
    static int initialized = 0;
    static bool enabled = false;
    if (!initialized) {
        const char* env = getenv("FISICS_DEBUG_LAYOUT");
        enabled = (env && env[0] && env[0] != '0');
        initialized = 1;
    }
    return enabled;
}

bool preprocess_tokens(Preprocessor* pp,
                       const TokenBuffer* input,
                       PPTokenBuffer* output,
                       bool appendEOF) {
    if (!pp || !input || !output) return false;
    CompilerContext* ctx = pp->ctx;
    const char* baselineLayout = ctx ? cc_get_data_layout(ctx) : NULL;
    if (input->count > 0 && input->tokens) {
        const char* filePath = input->tokens[0].location.start.file;
        if (filePath) {
            pp_set_base_file(pp, filePath);
            if (!pp->logicalFile) {
                pp_set_logical_file(pp, filePath);
            }
        }
    }
    PPBuiltinState builtins = {0};
    builtins.baseFile = pp->baseFile;
    builtins.dateString = pp->dateString[0] ? pp->dateString : NULL;
    builtins.timeString = pp->timeString[0] ? pp->timeString : NULL;
    builtins.counter = &pp->counter;
    builtins.logicalFile = (const char**)&pp->logicalFile;
    builtins.lineOffset = &pp->lineOffset;
    builtins.lineRemapActive = &pp->lineRemapActive;
    macro_expander_set_builtins(&pp->expander, builtins);
    PPTokenBuffer chunk = {0};
    PPConditionalFrame* condStack = NULL;
    size_t condDepth = 0;
    size_t condCap = 0;

    for (size_t i = 0; i < input->count; ++i) {
        if (ctx && debug_layout_enabled()) {
            const char* curLayout = cc_get_data_layout(ctx);
            if (curLayout != baselineLayout) {
                const Token* t = &input->tokens[i];
                LOG_WARN("codegen", "[preprocess_tokens] dataLayout changed to %p at token %zu type=%d line=%d val=%s",
                         (void*)curLayout,
                         i,
                         t ? t->type : -1,
                         t ? t->line : -1,
                         (t && t->value) ? t->value : "<null>");
                baselineLayout = curLayout;
            }
        }
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
                        if (!append_directive_line(input->tokens, input->count, i, output, pp)) {
                            pp_token_buffer_reset(&chunk);
                            free(condStack);
                            pp_debug_fail("append_directive_line", &input->tokens[i]);
                            return false;
                        }
                    }
                    ProfilerScope defineScope = profiler_begin("pp_define");
                    if (!process_define(pp, input->tokens, input->count, &i)) {
                        profiler_end(defineScope);
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        pp_debug_fail("process_define", &input->tokens[i]);
                        return false;
                    }
                    profiler_end(defineScope);
                } else {
                    skip_to_line_end(input->tokens, input->count, &i);
                }
                break;
            case TOKEN_INCLUDE:
            case TOKEN_INCLUDE_NEXT:
                if (active) {
                    if (!flush_chunk(pp, &chunk, output)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                    if (pp->preserveDirectives) {
                        if (!append_directive_line(input->tokens, input->count, i, output, pp)) {
                            pp_token_buffer_reset(&chunk);
                            free(condStack);
                            pp_debug_fail("append_directive_line", &input->tokens[i]);
                            return false;
                        }
                    }
                    bool isIncludeNext = tok->type == TOKEN_INCLUDE_NEXT;
                    ProfilerScope includeScope = profiler_begin("pp_include");
                    if (!process_include(pp, input->tokens, input->count, &i, output, isIncludeNext)) {
                        profiler_end(includeScope);
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        pp_debug_fail("process_include", &input->tokens[i]);
                        return false;
                    }
                    profiler_end(includeScope);
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
                        pp_debug_fail("process_undef", &input->tokens[i]);
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
                    pp_debug_fail("flush_chunk", &input->tokens[i]);
                    return false;
                }
                ProfilerScope ifScope = profiler_begin("pp_if");
                if (!process_if(pp, input->tokens, input->count, &i,
                                &condStack, &condDepth, &condCap)) {
                    profiler_end(ifScope);
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("process_if", &input->tokens[i]);
                    return false;
                }
                profiler_end(ifScope);
                break;
            case TOKEN_PP_ELIF:
                if (!flush_chunk(pp, &chunk, output)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("flush_chunk", &input->tokens[i]);
                    return false;
                }
                ProfilerScope elifScope = profiler_begin("pp_elif");
                if (!process_elif(pp, input->tokens, input->count, &i,
                                  condStack, condDepth)) {
                    profiler_end(elifScope);
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("process_elif", &input->tokens[i]);
                    return false;
                }
                profiler_end(elifScope);
                break;
            case TOKEN_PP_ELSE:
                if (!flush_chunk(pp, &chunk, output)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("flush_chunk", &input->tokens[i]);
                    return false;
                }
                if (directive_has_trailing_tokens(input->tokens, input->count, i)) {
                    pp_report_diag(pp,
                                   &input->tokens[i + 1],
                                   DIAG_ERROR,
                                   CDIAG_PREPROCESSOR_GENERIC,
                                   "unexpected tokens after #else directive");
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                if (!process_else(pp, condStack, condDepth, &input->tokens[i])) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("process_else", &input->tokens[i]);
                    return false;
                }
                skip_to_line_end(input->tokens, input->count, &i);
                break;
            case TOKEN_ENDIF:
                if (!flush_chunk(pp, &chunk, output)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("flush_chunk", &input->tokens[i]);
                    return false;
                }
                if (directive_has_trailing_tokens(input->tokens, input->count, i)) {
                    pp_report_diag(pp,
                                   &input->tokens[i + 1],
                                   DIAG_ERROR,
                                   CDIAG_PREPROCESSOR_GENERIC,
                                   "unexpected tokens after #endif directive");
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                if (pp->preserveDirectives && active) {
                    if (!append_directive_line(input->tokens, input->count, i, output, pp)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        pp_debug_fail("append_directive_line", &input->tokens[i]);
                        return false;
                    }
                }
                if (!process_endif(pp, condStack, &condDepth, &input->tokens[i])) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("process_endif", &input->tokens[i]);
                    return false;
                }
                skip_to_line_end(input->tokens, input->count, &i);
                break;
            case TOKEN_IFDEF:
            case TOKEN_IFNDEF: {
                if (!flush_chunk(pp, &chunk, output)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("flush_chunk", &input->tokens[i]);
                    return false;
                }
                bool negate = tok->type == TOKEN_IFNDEF;
                ProfilerScope ifdefScope = profiler_begin("pp_ifdef");
                if (!process_ifdeflike(pp, input->tokens, input->count, &i,
                                       &condStack, &condDepth, &condCap, negate)) {
                    profiler_end(ifdefScope);
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("process_ifdeflike", &input->tokens[i]);
                    return false;
                }
                profiler_end(ifdefScope);
                break;
            }
            case TOKEN_PRAGMA:
                if (active) {
                    if (!flush_chunk(pp, &chunk, output)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                    if (pp->preserveDirectives) {
                        if (!append_directive_line(input->tokens, input->count, i, output, pp)) {
                            pp_token_buffer_reset(&chunk);
                            free(condStack);
                            pp_debug_fail("append_directive_line", &input->tokens[i]);
                            return false;
                        }
                    }
                    if (!process_pragma(pp, input->tokens, input->count, &i)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        pp_debug_fail("process_pragma", &input->tokens[i]);
                        return false;
                    }
                } else {
                    skip_to_line_end(input->tokens, input->count, &i);
                }
                break;
            case TOKEN_PREPROCESSOR_OTHER:
                if (tok->value && strcmp(tok->value, "line") == 0) {
                    if (active) {
                        if (!flush_chunk(pp, &chunk, output)) {
                            pp_token_buffer_reset(&chunk);
                            free(condStack);
                            return false;
                        }
                        if (!process_line_directive(pp, input->tokens, input->count, &i)) {
                            pp_token_buffer_reset(&chunk);
                            free(condStack);
                            pp_debug_fail("process_line_directive", &input->tokens[i]);
                            return false;
                        }
                    } else {
                        skip_to_line_end(input->tokens, input->count, &i);
                    }
                    break;
                } else if (tok->value &&
                           (strcmp(tok->value, "warning") == 0 || strcmp(tok->value, "error") == 0)) {
                    bool isError = (strcmp(tok->value, "error") == 0);
                    if (active) {
                        char buffer[256];
                        buffer[0] = '\0';
                        size_t cursor = i + 1;
                        int directiveLine = tok->line;
                        while (cursor < input->count &&
                               input->tokens[cursor].type != TOKEN_EOF &&
                               input->tokens[cursor].line == directiveLine) {
                            const Token* lt = &input->tokens[cursor];
                            const char* piece = lt->value ? lt->value : "";
                            if (strlen(buffer) + strlen(piece) + 2 < sizeof(buffer)) {
                                if (buffer[0] != '\0') {
                                    strcat(buffer, " ");
                                }
                                strcat(buffer, piece);
                            }
                            cursor++;
                        }
                        DiagKind kind = isError ? DIAG_ERROR : DIAG_WARNING;
                        const char* msg = buffer[0] ? buffer : (isError ? "#error" : "#warning");
                        pp_report_diag(pp, tok, kind, CDIAG_PREPROCESSOR_GENERIC, "%s", msg);
                    }
                    skip_to_line_end(input->tokens, input->count, &i);
                    break;
                }
                if (active) {
                    pp_report_diag(pp,
                                   tok,
                                   DIAG_ERROR,
                                   CDIAG_PREPROCESSOR_GENERIC,
                                   "unknown preprocessing directive #%s",
                                   tok->value ? tok->value : "<unknown>");
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                skip_to_line_end(input->tokens, input->count, &i);
                break;
            case TOKEN_EOF:
                // handled after loop
                break;
            default:
                if (active) {
                    if (skip_pragma_operator(input->tokens, input->count, &i)) {
                        break;
                    }
                    if (!pp_token_buffer_append_clone(&chunk, tok)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                }
                break;
        }
    }

    if (!flush_chunk(pp, &chunk, output)) {
        pp_token_buffer_reset(&chunk);
        free(condStack);
        return false;
    }

    if (appendEOF) {
        Token eof = {0};
        eof.type = TOKEN_EOF;
        if (!pp_token_buffer_append_token(output, eof)) {
            free(condStack);
            return false;
        }
    }

    if (condDepth != 0) {
        pp_report_diag(pp, NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "unclosed #if/#ifdef/#ifndef");
        free(condStack);
        return false;
    }

    free(condStack);
    return true;
}

bool preprocessor_init(Preprocessor* pp,
                       CompilerContext* ctx,
                       bool preserveDirectives,
                       bool lenientMissingIncludes,
                       bool enableTrigraphs,
                       PreprocessMode preprocessMode,
                       const char* const* includePaths,
                       size_t includePathCount,
                       const char* externalPreprocessCmd,
                       const char* externalPreprocessArgs,
                       const char* const* macroDefines,
                       size_t macroDefineCount) {
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
    const char* limitEnv = getenv("FISICS_MACRO_EXP_LIMIT");
    if (limitEnv && limitEnv[0] != '\0') {
        long lim = strtol(limitEnv, NULL, 10);
        if (lim > 0) {
            macro_table_set_expansion_limit(pp->table, (size_t)lim);
        }
    }
    include_graph_init(&pp->includeGraph);
    pp->baseFile = NULL;
    pp->logicalFile = NULL;
    pp->lineOffset = 0;
    pp->lineRemapActive = false;
    pp->counter = 0;
    pp->includeStack.frames = NULL;
    pp->includeStack.depth = 0;
    pp->includeStack.capacity = 0;
    pp->logicalFilePool = NULL;
    pp->logicalFileCount = 0;
    pp->logicalFileCap = 0;
    pp->dateString[0] = '\0';
    pp->timeString[0] = '\0';
    time_t now = time(NULL);
    struct tm* tmNow = now == (time_t)-1 ? NULL : localtime(&now);
    if (tmNow) {
        strftime(pp->dateString, sizeof(pp->dateString), "%b %e %Y", tmNow);
        strftime(pp->timeString, sizeof(pp->timeString), "%H:%M:%S", tmNow);
    }
    pp->preserveDirectives = preserveDirectives;
    pp->lenientMissingIncludes = lenientMissingIncludes;
    pp->enableTrigraphs = enableTrigraphs;
    pp->preprocessMode = preprocessMode;
    pp->externalPreprocessCmd = externalPreprocessCmd;
    pp->externalPreprocessArgs = externalPreprocessArgs;
    pp->includePaths = includePaths;
    pp->includePathCount = includePathCount;
    pp->ctx = ctx;

    // Minimal builtin macros so system headers don't trigger #error on unknown/unsupported targets.
    define_builtin_object(pp, "__APPLE__", "1");
    define_builtin_object(pp, "__MACH__", "1");
    define_builtin_object(pp, "__APPLE_CC__", "1");
    define_builtin_object(pp, "__APPLE_CPP__", "1");
    define_builtin_object(pp, "__arm64__", "1");
    define_builtin_object(pp, "__LP64__", "1");
    define_builtin_object(pp, "__clang__", "1");
    define_builtin_object(pp, "__GNUC__", "4");
    define_builtin_object(pp, "__GNUC_MINOR__", "2");
    define_builtin_object(pp, "__GNUC_PATCHLEVEL__", "1");
    define_builtin_object(pp, "__STDC_HOSTED__", "1");
    {
        long stdc = 199901L;
        if (pp->ctx) {
            stdc = cc_dialect_stdc_version(cc_get_language_dialect(pp->ctx));
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "%ldL", stdc);
        define_builtin_object(pp, "__STDC_VERSION__", buf);
        define_builtin_object(pp, "__FISICS__", "1");
        snprintf(buf, sizeof(buf), "%ldL", stdc);
        define_builtin_object(pp, "__FISICS_DIALECT__", buf);
        if (pp->ctx && cc_extensions_enabled(pp->ctx)) {
            define_builtin_object(pp, "__FISICS_EXTENSIONS__", "1");
        }
    }
    define_builtin_object(pp, "MAC_OS_X_VERSION_MIN_REQUIRED", "130000");
    define_builtin_object(pp, "__FLT_MAX__", "3.402823466e+38F");
    define_builtin_object(pp, "__DBL_MAX__", "1.7976931348623157e+308");
    define_builtin_object(pp, "__LDBL_MAX__", "1.189731495357231765e+4932L");
    define_builtin_object(pp, "__FLT_MIN__", "1.175494351e-38F");
    define_builtin_object(pp, "__DBL_MIN__", "2.2250738585072014e-308");
    define_builtin_object(pp, "__LDBL_MIN__", "3.3621031431120935063e-4932L");
    define_builtin_object(pp, "__FLT_EPSILON__", "1.19209290e-7F");
    define_builtin_object(pp, "__DBL_EPSILON__", "2.2204460492503131e-16");
    define_builtin_object(pp, "__LDBL_EPSILON__", "1.0842021724855044e-19L");
    define_builtin_object(pp, "__ATOMIC_RELAXED", "0");
    define_builtin_object(pp, "__ATOMIC_CONSUME", "1");
    define_builtin_object(pp, "__ATOMIC_ACQUIRE", "2");
    define_builtin_object(pp, "__ATOMIC_RELEASE", "3");
    define_builtin_object(pp, "__ATOMIC_ACQ_REL", "4");
    define_builtin_object(pp, "__ATOMIC_SEQ_CST", "5");
    define_builtin_object(pp, "__ORDER_LITTLE_ENDIAN__", "1234");
    define_builtin_object(pp, "__ORDER_BIG_ENDIAN__", "4321");
    define_builtin_object(pp, "__ORDER_PDP_ENDIAN__", "3412");
    define_builtin_object(pp, "__BYTE_ORDER__", "__ORDER_LITTLE_ENDIAN__");
    define_builtin_object(pp, "__LITTLE_ENDIAN__", "1");
    define_builtin_object(pp, "_LITTLE_ENDIAN", "1");
    define_builtin_object(pp, "LITTLE_ENDIAN", "1234");
    define_builtin_object(pp, "BIG_ENDIAN", "4321");
    define_builtin_object(pp, "BYTE_ORDER", "LITTLE_ENDIAN");

    {
        const char* params[] = {"x"};
        define_builtin_function(pp, "__has_builtin", params, 1, false, "0");
        define_builtin_function(pp, "__has_extension", params, 1, false, "0");
        define_builtin_function(pp, "__has_feature", params, 1, false, "0");
        define_builtin_function(pp, "__has_attribute", params, 1, false, "0");
        define_builtin_function(pp, "__has_c_attribute", params, 1, false, "0");
        define_builtin_function(pp, "__has_declspec_attribute", params, 1, false, "0");
        define_builtin_function(pp, "__has_warning", params, 1, false, "0");
        define_builtin_function(pp, "__is_identifier", params, 1, false, "1");
    }

    // Apple bounds-safety annotation shims: treat as no-ops so newer SDK headers parse.
    // Many appear as attributes or markers; empty object-like macros are enough to erase them.
    const char* libc_bounds_macros[] = {
        "_LIBC_SINGLE_BY_DEFAULT",
        "_LIBC_PTRCHECK_REPLACED",
        "_LIBC_COUNT",
        "_LIBC_COUNT_OR_NULL",
        "_LIBC_SIZE",
        "_LIBC_SIZE_OR_NULL",
        "_LIBC_ENDED_BY",
        "_LIBC_SINGLE",
        "_LIBC_UNSAFE_INDEXABLE",
        "_LIBC_CSTR",
        "_LIBC_NULL_TERMINATED",
        "_LIBC_FLEX_COUNT",
        NULL};
    for (size_t i = 0; libc_bounds_macros[i]; ++i) {
        define_builtin_empty_macro(pp, libc_bounds_macros[i]);
    }

    // Availability/visibility stubs (Apple/Clang) — treat as no-ops.
    const char* availability_macros[] = {
        "__API_AVAILABLE",
        "__API_DEPRECATED",
        "__API_DEPRECATED_WITH_REPLACEMENT",
        "__API_UNAVAILABLE",
        "__API_AVAILABLE_BEGIN",
        "__API_AVAILABLE_END",
        "__OSX_AVAILABLE_STARTING",
        "__OSX_AVAILABLE_BUT_DEPRECATED",
        "__OSX_DEPRECATED",
        "__IOS_AVAILABLE",
        "__IOS_DEPRECATED",
        "__IOS_UNAVAILABLE",
        "__TVOS_AVAILABLE",
        "__TVOS_DEPRECATED",
        "__TVOS_UNAVAILABLE",
        "__WATCHOS_AVAILABLE",
        "__WATCHOS_DEPRECATED",
        "__WATCHOS_UNAVAILABLE",
        "__WATCHOS_PROHIBITED",
        "__TVOS_PROHIBITED",
        "__IOS_PROHIBITED",
        "__MAC_OS_X_VERSION_MIN_REQUIRED",
        "__MAC_OS_X_VERSION_MAX_ALLOWED",
        NULL};
    for (size_t i = 0; availability_macros[i]; ++i) {
        define_builtin_empty_macro(pp, availability_macros[i]);
    }

    for (size_t i = 0; i < macroDefineCount; ++i) {
        const char* def = macroDefines ? macroDefines[i] : NULL;
        if (!def || def[0] == '\0') continue;
        if (def[0] == '-' && def[1] == 'D') {
            def += 2;
            if (def[0] == '\0') continue;
        }
        const char* eq = strchr(def, '=');
        char* name = NULL;
        const char* value = NULL;
        if (eq) {
            size_t nameLen = (size_t)(eq - def);
            name = (char*)malloc(nameLen + 1);
            if (!name) continue;
            memcpy(name, def, nameLen);
            name[nameLen] = '\0';
            value = eq + 1;
        } else {
            name = strdup(def);
            if (!name) continue;
        }

        char* normalizedValue = normalize_cli_macro_value(value);
        const char* valueForLex = normalizedValue ? normalizedValue : value;
        PPTokenBuffer body = {0};
        if (valueForLex && valueForLex[0] != '\0' && try_tokenize_cli_quoted_string(valueForLex, &body)) {
            /* already tokenized as a quoted string replacement */
        } else if (!tokenize_macro_replacement(valueForLex, &body)) {
            free(normalizedValue);
            free(name);
            continue;
        }

        macro_table_define_object(pp->table,
                                  name,
                                  body.tokens,
                                  body.count,
                                  (SourceRange){0});
        pp_token_buffer_reset(&body);
        free(normalizedValue);

        free(name);
    }
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
    free(pp->includeStack.frames);
    pp->includeStack.frames = NULL;
    pp->includeStack.depth = 0;
    pp->includeStack.capacity = 0;
    for (size_t i = 0; i < pp->logicalFileCount; ++i) {
        free(pp->logicalFilePool[i]);
    }
    free(pp->logicalFilePool);
    free(pp->baseFile);
    pp->logicalFilePool = NULL;
    pp->logicalFileCount = 0;
    pp->logicalFileCap = 0;
    pp->baseFile = NULL;
    pp->logicalFile = NULL;
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

bool preprocessor_run(Preprocessor* pp,
                      const TokenBuffer* input,
                      PPTokenBuffer* output) {
    if (!pp || !input || !output) return false;
    pp_token_buffer_init_local(output);
    const char* rootPath = (input->count > 0 && input->tokens)
                               ? token_file(&input->tokens[0])
                               : NULL;
    IncludeSearchOrigin origin = INCLUDE_SEARCH_RAW;
    size_t originIndex = (size_t)-1;
    if (rootPath && pp->resolver) {
        const IncludeFile* info = include_resolver_lookup(pp->resolver, rootPath);
        if (info) {
            origin = info->origin;
            originIndex = info->originIndex;
        }
    }
    bool pushed = false;
    if (rootPath) {
        pushed = pp_include_stack_push(pp, rootPath, origin, originIndex);
    }
    bool ok = preprocess_tokens(pp, input, output, true);
    if (!ok && output && output->count > 0) {
        Token eof = {0};
        eof.type = TOKEN_EOF;
        pp_token_buffer_append_token(output, eof);
    }
    if (pushed) {
        pp_include_stack_pop(pp);
    }
    return ok;
}
