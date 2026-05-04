// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/preprocessor.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Preprocessor/pp_internal.h"

#include "Lexer/tokens.h"
#include "Compiler/compiler_context.h"
#include "Utils/logging.h"
#include "Utils/profiler.h"

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
    size_t activeTokenCount = 0;
    size_t inactiveTokenCount = 0;
    bool nestedIncludeBody = pp && pp->includeStack.depth > 1;
    const PPIncludeFrame* includeFrame = pp_include_stack_top(pp);
    bool includePathBody = nestedIncludeBody &&
                           includeFrame &&
                           includeFrame->origin == INCLUDE_SEARCH_INCLUDE_PATH;

    for (size_t i = 0; i < input->count; ++i) {
        if (ctx && pp_debug_layout_enabled()) {
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
        if (tok->type != TOKEN_EOF) {
            size_t tokenCost = 1;
            bool directiveLike = false;
            switch (tok->type) {
                case TOKEN_DEFINE:
                case TOKEN_INCLUDE:
                case TOKEN_INCLUDE_NEXT:
                case TOKEN_UNDEF:
                case TOKEN_PP_IF:
                case TOKEN_PP_ELIF:
                case TOKEN_PP_ELSE:
                case TOKEN_ENDIF:
                case TOKEN_IFDEF:
                case TOKEN_IFNDEF:
                case TOKEN_PRAGMA:
                case TOKEN_PREPROCESSOR_OTHER:
                    tokenCost = pp_count_line_tokens_from_cursor(input->tokens, input->count, i);
                    directiveLike = true;
                    break;
                default:
                    break;
            }
            if (active) activeTokenCount += tokenCost;
            else inactiveTokenCount += tokenCost;
            if (includePathBody) {
                if (active) {
                    profiler_record_value("pp_count_include_path_active_tokens_scanned", tokenCost);
                    if (directiveLike) {
                        profiler_record_value("pp_count_include_path_active_directive_tokens_scanned", tokenCost);
                    } else {
                        profiler_record_value("pp_count_include_path_active_raw_tokens_scanned", tokenCost);
                    }
                } else {
                    profiler_record_value("pp_count_include_path_inactive_tokens_scanned", tokenCost);
                    if (directiveLike) {
                        profiler_record_value("pp_count_include_path_inactive_directive_tokens_scanned", tokenCost);
                    } else {
                        profiler_record_value("pp_count_include_path_inactive_raw_tokens_scanned", tokenCost);
                    }
                }
            }
        }
        switch (tok->type) {
            case TOKEN_DEFINE:
                if (active) {
                    if (!pp_flush_chunk_profiled(pp, &chunk, output, nestedIncludeBody, includePathBody)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                    if (pp->preserveDirectives) {
                        if (!pp_append_directive_line(input->tokens, input->count, i, output, pp)) {
                            pp_token_buffer_reset(&chunk);
                            free(condStack);
                            pp_debug_fail("append_directive_line", &input->tokens[i]);
                            return false;
                        }
                    }
                    ProfilerScope recurseDefineScope = {0};
                    ProfilerScope includePathDefineScope = {0};
                    if (nestedIncludeBody) {
                        recurseDefineScope = profiler_begin("pp_recurse_define");
                    }
                    if (includePathBody) {
                        includePathDefineScope = profiler_begin("pp_recurse_include_path_define");
                    }
                    ProfilerScope defineScope = profiler_begin("pp_define");
                    if (!process_define(pp, input->tokens, input->count, &i)) {
                        profiler_end(defineScope);
                        if (includePathBody) profiler_end(includePathDefineScope);
                        if (nestedIncludeBody) profiler_end(recurseDefineScope);
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        pp_debug_fail("process_define", &input->tokens[i]);
                        return false;
                    }
                    profiler_end(defineScope);
                    if (includePathBody) profiler_end(includePathDefineScope);
                    if (nestedIncludeBody) profiler_end(recurseDefineScope);
                } else {
                    ProfilerScope inactiveSkipScope = {0};
                    if (includePathBody) {
                        profiler_record_value("pp_count_include_path_inactive_skip_lines", 1);
                        inactiveSkipScope = profiler_begin("pp_recurse_include_path_inactive_skip");
                    }
                    skip_to_line_end(input->tokens, input->count, &i);
                    if (includePathBody) profiler_end(inactiveSkipScope);
                }
                break;
            case TOKEN_INCLUDE:
            case TOKEN_INCLUDE_NEXT:
                if (active) {
                    if (!pp_flush_chunk_profiled(pp, &chunk, output, nestedIncludeBody, includePathBody)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                    if (pp->preserveDirectives) {
                        if (!pp_append_directive_line(input->tokens, input->count, i, output, pp)) {
                            pp_token_buffer_reset(&chunk);
                            free(condStack);
                            pp_debug_fail("append_directive_line", &input->tokens[i]);
                            return false;
                        }
                    }
                    bool isIncludeNext = tok->type == TOKEN_INCLUDE_NEXT;
                    ProfilerScope recurseIncludeScope = {0};
                    ProfilerScope includePathNestedIncludeScope = {0};
                    if (nestedIncludeBody) {
                        recurseIncludeScope = profiler_begin("pp_recurse_nested_include");
                    }
                    if (includePathBody) {
                        includePathNestedIncludeScope = profiler_begin("pp_recurse_include_path_nested_include");
                    }
                    ProfilerScope includeScope = profiler_begin("pp_include");
                    if (!process_include(pp, input->tokens, input->count, &i, output, isIncludeNext)) {
                        profiler_end(includeScope);
                        if (includePathBody) profiler_end(includePathNestedIncludeScope);
                        if (nestedIncludeBody) profiler_end(recurseIncludeScope);
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        pp_debug_fail("process_include", &input->tokens[i]);
                        return false;
                    }
                    profiler_end(includeScope);
                    if (includePathBody) profiler_end(includePathNestedIncludeScope);
                    if (nestedIncludeBody) profiler_end(recurseIncludeScope);
                } else {
                    ProfilerScope inactiveSkipScope = {0};
                    if (includePathBody) {
                        profiler_record_value("pp_count_include_path_inactive_skip_lines", 1);
                        inactiveSkipScope = profiler_begin("pp_recurse_include_path_inactive_skip");
                    }
                    skip_to_line_end(input->tokens, input->count, &i);
                    if (includePathBody) profiler_end(inactiveSkipScope);
                }
                break;
            case TOKEN_UNDEF:
                if (active) {
                    if (!pp_flush_chunk_profiled(pp, &chunk, output, nestedIncludeBody, includePathBody)) {
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
                    ProfilerScope inactiveSkipScope = {0};
                    if (includePathBody) {
                        profiler_record_value("pp_count_include_path_inactive_skip_lines", 1);
                        inactiveSkipScope = profiler_begin("pp_recurse_include_path_inactive_skip");
                    }
                    skip_to_line_end(input->tokens, input->count, &i);
                    if (includePathBody) profiler_end(inactiveSkipScope);
                }
                break;
            case TOKEN_PP_IF:
                if (!pp_flush_chunk_profiled(pp, &chunk, output, nestedIncludeBody, includePathBody)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("flush_chunk", &input->tokens[i]);
                    return false;
                }
                ProfilerScope recurseIfScope = {0};
                ProfilerScope includePathConditionalScope = {0};
                if (nestedIncludeBody) {
                    recurseIfScope = profiler_begin("pp_recurse_conditional");
                }
                if (includePathBody) {
                    includePathConditionalScope = profiler_begin("pp_recurse_include_path_conditional");
                }
                ProfilerScope ifScope = profiler_begin("pp_if");
                if (!process_if(pp, input->tokens, input->count, &i,
                                &condStack, &condDepth, &condCap)) {
                    profiler_end(ifScope);
                    if (includePathBody) profiler_end(includePathConditionalScope);
                    if (nestedIncludeBody) profiler_end(recurseIfScope);
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("process_if", &input->tokens[i]);
                    return false;
                }
                profiler_end(ifScope);
                if (includePathBody) profiler_end(includePathConditionalScope);
                if (nestedIncludeBody) profiler_end(recurseIfScope);
                break;
            case TOKEN_PP_ELIF:
                if (!pp_flush_chunk_profiled(pp, &chunk, output, nestedIncludeBody, includePathBody)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("flush_chunk", &input->tokens[i]);
                    return false;
                }
                ProfilerScope recurseElifScope = {0};
                ProfilerScope includePathElifScope = {0};
                if (nestedIncludeBody) {
                    recurseElifScope = profiler_begin("pp_recurse_conditional");
                }
                if (includePathBody) {
                    includePathElifScope = profiler_begin("pp_recurse_include_path_conditional");
                }
                ProfilerScope elifScope = profiler_begin("pp_elif");
                if (!process_elif(pp, input->tokens, input->count, &i,
                                  condStack, condDepth)) {
                    profiler_end(elifScope);
                    if (includePathBody) profiler_end(includePathElifScope);
                    if (nestedIncludeBody) profiler_end(recurseElifScope);
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("process_elif", &input->tokens[i]);
                    return false;
                }
                profiler_end(elifScope);
                if (includePathBody) profiler_end(includePathElifScope);
                if (nestedIncludeBody) profiler_end(recurseElifScope);
                break;
            case TOKEN_PP_ELSE:
                if (!pp_flush_chunk_profiled(pp, &chunk, output, nestedIncludeBody, includePathBody)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("flush_chunk", &input->tokens[i]);
                    return false;
                }
                ProfilerScope recurseElseScope = {0};
                ProfilerScope includePathElseScope = {0};
                if (nestedIncludeBody) {
                    recurseElseScope = profiler_begin("pp_recurse_conditional");
                }
                if (includePathBody) {
                    includePathElseScope = profiler_begin("pp_recurse_include_path_conditional");
                }
                if (pp_directive_has_trailing_tokens(input->tokens, input->count, i)) {
                    pp_report_diag(pp,
                                   &input->tokens[i + 1],
                                   DIAG_ERROR,
                                   CDIAG_PREPROCESSOR_GENERIC,
                                   "unexpected tokens after #else directive");
                    if (includePathBody) profiler_end(includePathElseScope);
                    if (nestedIncludeBody) profiler_end(recurseElseScope);
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                if (!process_else(pp, condStack, condDepth, &input->tokens[i])) {
                    if (includePathBody) profiler_end(includePathElseScope);
                    if (nestedIncludeBody) profiler_end(recurseElseScope);
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("process_else", &input->tokens[i]);
                    return false;
                }
                if (includePathBody) profiler_end(includePathElseScope);
                if (nestedIncludeBody) profiler_end(recurseElseScope);
                skip_to_line_end(input->tokens, input->count, &i);
                break;
            case TOKEN_ENDIF:
                if (!pp_flush_chunk_profiled(pp, &chunk, output, nestedIncludeBody, includePathBody)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("flush_chunk", &input->tokens[i]);
                    return false;
                }
                ProfilerScope recurseEndifScope = {0};
                ProfilerScope includePathEndifScope = {0};
                if (nestedIncludeBody) {
                    recurseEndifScope = profiler_begin("pp_recurse_conditional");
                }
                if (includePathBody) {
                    includePathEndifScope = profiler_begin("pp_recurse_include_path_conditional");
                }
                if (pp_directive_has_trailing_tokens(input->tokens, input->count, i)) {
                    pp_report_diag(pp,
                                   &input->tokens[i + 1],
                                   DIAG_ERROR,
                                   CDIAG_PREPROCESSOR_GENERIC,
                                   "unexpected tokens after #endif directive");
                    if (includePathBody) profiler_end(includePathEndifScope);
                    if (nestedIncludeBody) profiler_end(recurseEndifScope);
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    return false;
                }
                if (pp->preserveDirectives && active) {
                    if (!pp_append_directive_line(input->tokens, input->count, i, output, pp)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        pp_debug_fail("append_directive_line", &input->tokens[i]);
                        return false;
                    }
                }
                if (!process_endif(pp, condStack, &condDepth, &input->tokens[i])) {
                    if (includePathBody) profiler_end(includePathEndifScope);
                    if (nestedIncludeBody) profiler_end(recurseEndifScope);
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("process_endif", &input->tokens[i]);
                    return false;
                }
                if (includePathBody) profiler_end(includePathEndifScope);
                if (nestedIncludeBody) profiler_end(recurseEndifScope);
                skip_to_line_end(input->tokens, input->count, &i);
                break;
            case TOKEN_IFDEF:
            case TOKEN_IFNDEF: {
                if (!pp_flush_chunk_profiled(pp, &chunk, output, nestedIncludeBody, includePathBody)) {
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("flush_chunk", &input->tokens[i]);
                    return false;
                }
                bool negate = tok->type == TOKEN_IFNDEF;
                ProfilerScope recurseIfdefScope = {0};
                ProfilerScope includePathIfdefScope = {0};
                if (nestedIncludeBody) {
                    recurseIfdefScope = profiler_begin("pp_recurse_conditional");
                }
                if (includePathBody) {
                    includePathIfdefScope = profiler_begin("pp_recurse_include_path_conditional");
                }
                ProfilerScope ifdefScope = profiler_begin("pp_ifdef");
                if (!process_ifdeflike(pp, input->tokens, input->count, &i,
                                       &condStack, &condDepth, &condCap, negate)) {
                    profiler_end(ifdefScope);
                    if (includePathBody) profiler_end(includePathIfdefScope);
                    if (nestedIncludeBody) profiler_end(recurseIfdefScope);
                    pp_token_buffer_reset(&chunk);
                    free(condStack);
                    pp_debug_fail("process_ifdeflike", &input->tokens[i]);
                    return false;
                }
                profiler_end(ifdefScope);
                if (includePathBody) profiler_end(includePathIfdefScope);
                if (nestedIncludeBody) profiler_end(recurseIfdefScope);
                break;
            }
            case TOKEN_PRAGMA:
                if (active) {
                    if (!pp_flush_chunk_profiled(pp, &chunk, output, nestedIncludeBody, includePathBody)) {
                        pp_token_buffer_reset(&chunk);
                        free(condStack);
                        return false;
                    }
                    if (pp->preserveDirectives) {
                        if (!pp_append_directive_line(input->tokens, input->count, i, output, pp)) {
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
                    ProfilerScope inactiveSkipScope = {0};
                    if (includePathBody) {
                        profiler_record_value("pp_count_include_path_inactive_skip_lines", 1);
                        inactiveSkipScope = profiler_begin("pp_recurse_include_path_inactive_skip");
                    }
                    skip_to_line_end(input->tokens, input->count, &i);
                    if (includePathBody) profiler_end(inactiveSkipScope);
                }
                break;
            case TOKEN_PREPROCESSOR_OTHER:
                if (tok->value && strcmp(tok->value, "line") == 0) {
                    if (active) {
                        if (!pp_flush_chunk_profiled(pp, &chunk, output, nestedIncludeBody, includePathBody)) {
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
                        ProfilerScope inactiveSkipScope = {0};
                        if (includePathBody) {
                            profiler_record_value("pp_count_include_path_inactive_skip_lines", 1);
                            inactiveSkipScope = profiler_begin("pp_recurse_include_path_inactive_skip");
                        }
                        skip_to_line_end(input->tokens, input->count, &i);
                        if (includePathBody) profiler_end(inactiveSkipScope);
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
                    ProfilerScope inactiveSkipScope = {0};
                    if (includePathBody) {
                        profiler_record_value("pp_count_include_path_inactive_skip_lines", 1);
                        inactiveSkipScope = profiler_begin("pp_recurse_include_path_inactive_skip");
                    }
                    skip_to_line_end(input->tokens, input->count, &i);
                    if (includePathBody) profiler_end(inactiveSkipScope);
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
                ProfilerScope inactiveSkipScope = {0};
                if (includePathBody) {
                    profiler_record_value("pp_count_include_path_inactive_skip_lines", 1);
                    inactiveSkipScope = profiler_begin("pp_recurse_include_path_inactive_skip");
                }
                skip_to_line_end(input->tokens, input->count, &i);
                if (includePathBody) profiler_end(inactiveSkipScope);
                break;
            case TOKEN_EOF:
                // handled after loop
                break;
            default:
                if (active) {
                    if (pp_skip_pragma_operator(input->tokens, input->count, &i)) {
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

    if (!pp_flush_chunk_profiled(pp, &chunk, output, nestedIncludeBody, includePathBody)) {
        pp_token_buffer_reset(&chunk);
        free(condStack);
        return false;
    }
    if (activeTokenCount > 0) {
        profiler_record_value("pp_count_active_tokens_scanned", activeTokenCount);
    }
    if (inactiveTokenCount > 0) {
        profiler_record_value("pp_count_inactive_tokens_scanned", inactiveTokenCount);
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

    pp_define_predefined_macros(pp, macroDefines, macroDefineCount);
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
