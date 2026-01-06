#include "Preprocessor/preprocessor.h"

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Preprocessor/pp_internal.h"

#include "Lexer/tokens.h"
#include "Compiler/diagnostics.h"
#include "Utils/logging.h"

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

static void define_builtin_object(Preprocessor* pp, const char* name, const char* value) {
    if (!pp || !name) return;
    Token tok = {0};
    PPTokenBuffer body = {0};
    if (value && value[0]) {
        tok.type = isdigit((unsigned char)value[0]) ? TOKEN_NUMBER : TOKEN_IDENTIFIER;
        tok.value = strdup(value);
        tok.location = (SourceRange){0};
        tok.macroCallSite = (SourceRange){0};
        tok.macroDefinition = (SourceRange){0};
        if (tok.value) {
            body.tokens = &tok;
            body.count = 1;
        }
    }
    macro_table_define_object(pp->table, name, body.tokens, body.count, (SourceRange){0});
    free(tok.value);
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
    if (!macro_expander_expand(&pp->expander, chunk->tokens, chunk->count, &expanded)) {
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
        if (!pp_token_buffer_append_clone(output, &expanded.tokens[j])) {
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
                    if (!process_define(pp, input->tokens, input->count, &i)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        pp_debug_fail("process_define", &input->tokens[i]);
                        return false;
                    }
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
                    if (!process_include(pp, input->tokens, input->count, &i, output, isIncludeNext)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        pp_debug_fail("process_include", &input->tokens[i]);
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
                if (!process_if(pp, input->tokens, input->count, &i,
                                &condStack, &condDepth, &condCap)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("process_if", &input->tokens[i]);
                    return false;
                }
                break;
            case TOKEN_PP_ELIF:
                if (!flush_chunk(pp, &chunk, output)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("flush_chunk", &input->tokens[i]);
                    return false;
                }
                if (!process_elif(pp, input->tokens, input->count, &i,
                                  condStack, condDepth)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("process_elif", &input->tokens[i]);
                    return false;
                }
                break;
            case TOKEN_PP_ELSE:
                if (!flush_chunk(pp, &chunk, output)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("flush_chunk", &input->tokens[i]);
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
                if (!process_ifdeflike(pp, input->tokens, input->count, &i,
                                       &condStack, &condDepth, &condCap, negate)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("process_ifdeflike", &input->tokens[i]);
                    return false;
                }
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
                // fall through to default handling for other unknown directives
            case TOKEN_EOF:
                // handled after loop
                break;
            default:
                if (active) {
                    if (skip_pragma_operator(input->tokens, input->count, &i)) {
                        break;
                    }
                    if (!pp_token_buffer_append_clone_remap(&chunk, pp, tok)) {
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
                       const char* const* includePaths,
                       size_t includePathCount,
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
    pp->ctx = ctx;

    // Minimal builtin macros so system headers don't trigger #error on unknown/unsupported targets.
    define_builtin_object(pp, "__APPLE__", "1");
    define_builtin_object(pp, "__MACH__", "1");
    define_builtin_object(pp, "__arm64__", "1");
    define_builtin_object(pp, "__LP64__", "1");
    define_builtin_object(pp, "__clang__", "1");
    define_builtin_object(pp, "__GNUC__", "4");
    define_builtin_object(pp, "__GNUC_MINOR__", "2");
    define_builtin_object(pp, "__GNUC_PATCHLEVEL__", "1");
    define_builtin_object(pp, "__STDC_HOSTED__", "1");
    define_builtin_object(pp, "__STDC_VERSION__", "199901L");

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
        const char* eq = strchr(def, '=');
        char* name = NULL;
        char* value = NULL;
        if (eq) {
            size_t nameLen = (size_t)(eq - def);
            name = (char*)malloc(nameLen + 1);
            if (!name) continue;
            memcpy(name, def, nameLen);
            name[nameLen] = '\0';
            value = strdup(eq + 1);
            if (!value) {
                free(name);
                continue;
            }
        } else {
            name = strdup(def);
            if (!name) continue;
        }

        Token tok = {0};
        PPTokenBuffer body = {0};
        if (value && value[0]) {
            tok.type = isdigit((unsigned char)value[0]) ? TOKEN_NUMBER : TOKEN_IDENTIFIER;
            tok.value = strdup(value);
            tok.location = (SourceRange){0};
            tok.macroCallSite = (SourceRange){0};
            tok.macroDefinition = (SourceRange){0};
            if (tok.value) {
                body.tokens = &tok;
                body.count = 1;
            }
        }

        macro_table_define_object(pp->table,
                                  name,
                                  body.tokens,
                                  body.count,
                                  (SourceRange){0});

        free(name);
        if (value) free(value);
        free(tok.value);
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
    if (pushed) {
        pp_include_stack_pop(pp);
    }
    return ok;
}
