// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/pp_internal.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Utils/logging.h"
#include "Utils/profiler.h"

typedef enum {
    PP_INCLUDE_REPLAY_KIND_ROUTER = 0,
    PP_INCLUDE_REPLAY_KIND_SCAFFOLD,
    PP_INCLUDE_REPLAY_KIND_SUMMARY,
} PPIncludeReplayKind;

static bool pp_stagea_diagnostic_profile_enabled(void) {
    static int initialized = 0;
    static bool enabled = false;
    if (!initialized) {
        const char* env = getenv("FISICS_PP_STAGEA_DIAGNOSTIC_PROFILE");
        enabled = (env && env[0] && env[0] != '0');
        initialized = 1;
    }
    return enabled;
}

static const char* pp_include_replay_nested_dispatch_counter_name(PPIncludeReplayKind kind) {
    switch (kind) {
        case PP_INCLUDE_REPLAY_KIND_ROUTER:
            return "pp_count_include_path_router_replay_nested_include_dispatch";
        case PP_INCLUDE_REPLAY_KIND_SCAFFOLD:
            return "pp_count_include_path_scaffold_replay_nested_include_dispatch";
        case PP_INCLUDE_REPLAY_KIND_SUMMARY:
            return "pp_count_include_path_summary_replay_nested_include_dispatch";
    }
    return "pp_count_include_path_summary_replay_nested_include_dispatch";
}

static const char* pp_include_replay_nested_dispatch_scope_name(PPIncludeReplayKind kind) {
    switch (kind) {
        case PP_INCLUDE_REPLAY_KIND_ROUTER:
            return "pp_recurse_include_path_router_replay_nested_include_dispatch";
        case PP_INCLUDE_REPLAY_KIND_SCAFFOLD:
            return "pp_recurse_include_path_scaffold_replay_nested_include_dispatch";
        case PP_INCLUDE_REPLAY_KIND_SUMMARY:
            return "pp_recurse_include_path_summary_replay_nested_include_dispatch";
    }
    return "pp_recurse_include_path_summary_replay_nested_include_dispatch";
}

static void pp_record_include_path_replay_used(PPIncludeReplayKind kind) {
    if (!pp_stagea_diagnostic_profile_enabled()) return;
    profiler_record_value("pp_count_include_path_replay_used", 1);
    switch (kind) {
        case PP_INCLUDE_REPLAY_KIND_ROUTER:
            profiler_record_value("pp_count_include_path_router_replay_used", 1);
            break;
        case PP_INCLUDE_REPLAY_KIND_SCAFFOLD:
            profiler_record_value("pp_count_include_path_scaffold_replay_used", 1);
            break;
        case PP_INCLUDE_REPLAY_KIND_SUMMARY:
            profiler_record_value("pp_count_include_path_summary_replay_used", 1);
            break;
    }
}

static long pp_summary_env_long(const char* name, long fallback) {
    const char* env = getenv(name);
    if (!env || !env[0]) return fallback;
    char* end = NULL;
    long value = strtol(env, &end, 10);
    if (!end || *end != '\0') return fallback;
    return value;
}

static bool pp_router_replay_trace_enabled(void) {
    const char* env = getenv("FISICS_PP_SUMMARY_REPLAY_ROUTER_TRACE");
    return env && env[0] && env[0] != '0';
}

static bool pp_router_replay_allowed(const char* filePath,
                                     const IncludeSummaryProbe* probe,
                                     size_t actionCount) {
    static long replaySeen = 0;
    long limit = pp_summary_env_long("FISICS_PP_SUMMARY_REPLAY_ROUTER_LIMIT", LONG_MAX);
    replaySeen++;
    bool allowed = replaySeen <= limit;
    if (pp_router_replay_trace_enabled()) {
        fprintf(stderr,
                "[pp-router-replay] seen=%ld allowed=%d raw_tail=%d actions=%zu file=%s\n",
                replaySeen,
                allowed ? 1 : 0,
                probe && probe->routerRawTail ? 1 : 0,
                actionCount,
                filePath ? filePath : "<unknown>");
    }
    return allowed;
}

static bool pp_summary_append_raw_range(Preprocessor* pp,
                                        const Token* tokens,
                                        size_t count,
                                        size_t start,
                                        size_t end,
                                        PPTokenBuffer* chunk) {
    (void)pp;
    if (!tokens || !chunk) return false;
    size_t i = start;
    while (i < end && i < count) {
        const Token* tok = &tokens[i];
        if (tok->type == TOKEN_EOF) break;
        size_t cursor = i;
        if (pp_skip_pragma_operator(tokens, count, &cursor)) {
            i = cursor + 1;
            continue;
        }
        if (tok->type != TOKEN_UNKNOWN) {
            if (!pp_token_buffer_append_clone(chunk, tok)) {
                return false;
            }
        }
        i++;
    }
    return true;
}

static bool router_mark_pragma_once(Preprocessor* pp,
                                    const TokenBuffer* input,
                                    const IncludeSummaryAction* action) {
    if (!pp || !pp->resolver || !input || !input->tokens || !action) {
        return false;
    }
    if (action->start >= input->count) {
        return false;
    }
    const char* path = token_file(&input->tokens[action->start]);
    if (!path) {
        return false;
    }
    include_resolver_mark_pragma_once(pp->resolver, path);
    return true;
}

static bool pp_summary_identifier_requires_expansion(const char* name) {
    if (!name || !name[0]) return false;
    return strcmp(name, "_Pragma") == 0 ||
           strcmp(name, "__LINE__") == 0 ||
           strcmp(name, "__COUNTER__") == 0 ||
           strcmp(name, "__FILE__") == 0 ||
           strcmp(name, "__BASE_FILE__") == 0 ||
           strcmp(name, "__DATE__") == 0 ||
           strcmp(name, "__TIME__") == 0 ||
           strcmp(name, "__has_builtin") == 0 ||
           strcmp(name, "__has_extension") == 0 ||
           strcmp(name, "__has_feature") == 0 ||
           strcmp(name, "__has_attribute") == 0 ||
           strcmp(name, "__has_c_attribute") == 0 ||
           strcmp(name, "__has_declspec_attribute") == 0 ||
           strcmp(name, "__has_warning") == 0 ||
           strcmp(name, "__is_identifier") == 0;
}

bool pp_summary_raw_range_can_clone_direct(Preprocessor* pp,
                                           const Token* tokens,
                                           size_t count,
                                           size_t start,
                                           size_t end) {
    if (!pp || !tokens) return false;
    bool sawTypedef = false;
    for (size_t i = start; i < end && i < count; ++i) {
        const Token* tok = &tokens[i];
        if (tok->type == TOKEN_EOF) break;
        switch (tok->type) {
            case TOKEN_UNKNOWN:
                return false;
            case TOKEN_TYPEDEF:
                sawTypedef = true;
                break;
            case TOKEN_INCLUDE:
            case TOKEN_INCLUDE_NEXT:
            case TOKEN_DEFINE:
            case TOKEN_UNDEF:
            case TOKEN_IFDEF:
            case TOKEN_IFNDEF:
            case TOKEN_ENDIF:
            case TOKEN_PRAGMA:
            case TOKEN_PREPROCESSOR_OTHER:
            case TOKEN_PP_IF:
            case TOKEN_PP_ELIF:
            case TOKEN_PP_ELSE:
            case TOKEN_HASH:
            case TOKEN_DOUBLE_HASH:
                return false;
            case TOKEN_IDENTIFIER:
                if (!tok->value ||
                    pp_summary_identifier_requires_expansion(tok->value) ||
                    macro_table_lookup(pp->table, tok->value) != NULL) {
                    return false;
                }
                break;
            default:
                break;
        }
    }
    return sawTypedef;
}

static bool pp_summary_action_raw_range_can_clone_direct(Preprocessor* pp,
                                                         const TokenBuffer* input,
                                                         IncludeSummaryAction* action) {
    if (!pp || !input || !input->tokens || !action || action->kind != INCLUDE_SUMMARY_ACTION_RAW_RANGE) {
        return false;
    }
    uint64_t macroSerial = macro_table_mutation_serial(pp->table);
    if (action->directCloneMemoValid && action->directCloneMacroSerial == macroSerial) {
        return action->directCloneMemoResult;
    }
    bool result = pp_summary_raw_range_can_clone_direct(pp,
                                                        input->tokens,
                                                        input->count,
                                                        action->start,
                                                        action->end);
    action->directCloneMacroSerial = macroSerial;
    action->directCloneMemoValid = true;
    action->directCloneMemoResult = result;
    return result;
}

static bool pp_summary_append_raw_range_direct(Preprocessor* pp,
                                               const TokenBuffer* input,
                                               IncludeSummaryAction* action,
                                               PPTokenBuffer* output) {
    if (!pp || !input || !input->tokens || !action || !output) return false;
    if (!pp_summary_action_raw_range_can_clone_direct(pp, input, action)) {
        return false;
    }
    size_t start = action->start;
    size_t spanEnd = action->end < input->count ? action->end : input->count;
    for (size_t i = start; i < spanEnd; ++i) {
        if (input->tokens[i].type == TOKEN_EOF) {
            spanEnd = i;
            break;
        }
    }
    return pp_token_buffer_append_clone_remap_span(output, pp, input->tokens + start, spanEnd - start);
}

static bool pp_router_replay_can_run(Preprocessor* pp,
                                     const TokenBuffer* input,
                                     const IncludeSummaryProbe* probe,
                                     IncludeSummaryAction* actions,
                                     size_t actionCount) {
    if (!pp || !input || !probe || !actions || actionCount == 0) {
        return false;
    }
    for (size_t actionIndex = 0; actionIndex < actionCount; ++actionIndex) {
        IncludeSummaryAction* action = &actions[actionIndex];
        switch (action->kind) {
            case INCLUDE_SUMMARY_ACTION_PRAGMA:
            case INCLUDE_SUMMARY_ACTION_IFNDEF:
            case INCLUDE_SUMMARY_ACTION_ENDIF:
            case INCLUDE_SUMMARY_ACTION_DEFINE:
            case INCLUDE_SUMMARY_ACTION_INCLUDE:
            case INCLUDE_SUMMARY_ACTION_INCLUDE_NEXT:
                break;
            case INCLUDE_SUMMARY_ACTION_RAW_RANGE:
                if (!probe->routerRawTail ||
                    !pp_summary_action_raw_range_can_clone_direct(pp, input, action)) {
                    return false;
                }
                break;
            default:
                return false;
        }
    }
    return true;
}

PPSummaryReplayResult preprocess_tokens_router_replay(Preprocessor* pp,
                                                      const TokenBuffer* input,
                                                      const IncludeSummaryProbe* probe,
                                                      IncludeSummaryAction* actions,
                                                      size_t actionCount,
                                                      PPTokenBuffer* output,
                                                      bool appendEOF) {
    if (!pp || !input || !probe || !output) return PP_SUMMARY_REPLAY_ERROR;
    if ((!probe->routerOnly && !probe->routerRawTail) ||
        !actions ||
        actionCount == 0 ||
        pp->preserveDirectives) {
        return PP_SUMMARY_REPLAY_UNSUPPORTED;
    }

    if (input->count > 0 && input->tokens) {
        const char* filePath = input->tokens[0].location.start.file;
        if (filePath) {
            pp_set_base_file(pp, filePath);
            if (!pp->logicalFile) {
                pp_set_logical_file(pp, filePath);
            }
        }
    }
    const char* replayFilePath =
        (input && input->count > 0 && input->tokens) ? input->tokens[0].location.start.file : NULL;
    if (!pp_router_replay_allowed(replayFilePath, probe, actionCount)) {
        return PP_SUMMARY_REPLAY_UNSUPPORTED;
    }
    if (!pp_router_replay_can_run(pp, input, probe, actions, actionCount)) {
        return PP_SUMMARY_REPLAY_UNSUPPORTED;
    }

    bool timingEnabled = profiler_timing_enabled();
    ProfilerScope replayScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_include_router_replay");
    bool nestedIncludeBody = pp && pp->includeStack.depth > 1;
    const PPIncludeFrame* includeFrame = pp_include_stack_top(pp);
    bool includePathBody = nestedIncludeBody &&
                           includeFrame &&
                           includeFrame->origin == INCLUDE_SEARCH_INCLUDE_PATH;
    PPSummaryReplayResult result = PP_SUMMARY_REPLAY_USED;

    for (size_t actionIndex = 0; actionIndex < actionCount; ++actionIndex) {
        IncludeSummaryAction* action = &actions[actionIndex];
        size_t cursor = action->start;
        switch (action->kind) {
            case INCLUDE_SUMMARY_ACTION_PRAGMA:
                if (!router_mark_pragma_once(pp, input, action)) {
                    result = PP_SUMMARY_REPLAY_UNSUPPORTED;
                    goto cleanup;
                }
                break;
            case INCLUDE_SUMMARY_ACTION_IFNDEF:
            case INCLUDE_SUMMARY_ACTION_ENDIF:
                break;
            case INCLUDE_SUMMARY_ACTION_RAW_RANGE:
                if (!probe->routerRawTail ||
                    !pp_summary_append_raw_range_direct(pp, input, action, output)) {
                    result = PP_SUMMARY_REPLAY_UNSUPPORTED;
                    goto cleanup;
                }
                break;
            case INCLUDE_SUMMARY_ACTION_DEFINE: {
                ProfilerScope recurseDefineScope = {0};
                ProfilerScope includePathDefineScope = {0};
                if (nestedIncludeBody) recurseDefineScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_define");
                if (includePathBody) includePathDefineScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_include_path_define");
                ProfilerScope defineScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_define");
                bool ok = process_define(pp, input->tokens, input->count, &cursor);
                pp_profiler_end_if_enabled(timingEnabled, defineScope);
                if (includePathBody) pp_profiler_end_if_enabled(timingEnabled, includePathDefineScope);
                if (nestedIncludeBody) pp_profiler_end_if_enabled(timingEnabled, recurseDefineScope);
                if (!ok) {
                    result = PP_SUMMARY_REPLAY_ERROR;
                    goto cleanup;
                }
                break;
            }
            case INCLUDE_SUMMARY_ACTION_INCLUDE:
            case INCLUDE_SUMMARY_ACTION_INCLUDE_NEXT: {
                bool isIncludeNext = action->kind == INCLUDE_SUMMARY_ACTION_INCLUDE_NEXT;
                ProfilerScope recurseIncludeScope = {0};
                ProfilerScope includePathNestedIncludeScope = {0};
                ProfilerScope replayDispatchScope = {0};
                if (nestedIncludeBody) recurseIncludeScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_nested_include");
                if (includePathBody) includePathNestedIncludeScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_include_path_nested_include");
                if (includePathBody && pp_stagea_diagnostic_profile_enabled()) {
                    profiler_record_value("pp_count_include_path_replay_nested_include_dispatch", 1);
                    profiler_record_value(pp_include_replay_nested_dispatch_counter_name(PP_INCLUDE_REPLAY_KIND_ROUTER), 1);
                    replayDispatchScope = pp_profiler_begin_if_enabled(timingEnabled,
                        pp_include_replay_nested_dispatch_scope_name(PP_INCLUDE_REPLAY_KIND_ROUTER));
                }
                ProfilerScope includeScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_include");
                bool ok = process_include(pp, input->tokens, input->count, &cursor, output, isIncludeNext);
                pp_profiler_end_if_enabled(timingEnabled, includeScope);
                if (includePathBody && pp_stagea_diagnostic_profile_enabled()) pp_profiler_end_if_enabled(timingEnabled, replayDispatchScope);
                if (includePathBody) pp_profiler_end_if_enabled(timingEnabled, includePathNestedIncludeScope);
                if (nestedIncludeBody) pp_profiler_end_if_enabled(timingEnabled, recurseIncludeScope);
                if (!ok) {
                    result = PP_SUMMARY_REPLAY_ERROR;
                    goto cleanup;
                }
                break;
            }
            default:
                result = PP_SUMMARY_REPLAY_UNSUPPORTED;
                goto cleanup;
        }
    }

    if (appendEOF) {
        Token eof = {0};
        eof.type = TOKEN_EOF;
        if (!pp_token_buffer_append_token(output, eof)) {
            result = PP_SUMMARY_REPLAY_ERROR;
            goto cleanup;
        }
    }

cleanup:
    pp_profiler_end_if_enabled(timingEnabled, replayScope);
    if (result == PP_SUMMARY_REPLAY_USED && includePathBody) {
        pp_record_include_path_replay_used(PP_INCLUDE_REPLAY_KIND_ROUTER);
    }
    return result;
}

PPSummaryReplayResult preprocess_tokens_scaffold_replay(Preprocessor* pp,
                                                        const TokenBuffer* input,
                                                        const IncludeSummaryProbe* probe,
                                                        IncludeSummaryAction* actions,
                                                        size_t actionCount,
                                                        PPTokenBuffer* output,
                                                        bool appendEOF) {
    if (!pp || !input || !probe || !output) return PP_SUMMARY_REPLAY_ERROR;
    if (probe->behaviorClass != INCLUDE_HEADER_BEHAVIOR_INCLUDE_DEFINE_SCAFFOLD ||
        !actions ||
        actionCount == 0 ||
        pp->preserveDirectives) {
        return PP_SUMMARY_REPLAY_UNSUPPORTED;
    }

    if (input->count > 0 && input->tokens) {
        const char* filePath = input->tokens[0].location.start.file;
        if (filePath) {
            pp_set_base_file(pp, filePath);
            if (!pp->logicalFile) {
                pp_set_logical_file(pp, filePath);
            }
        }
    }

    bool timingEnabled = profiler_timing_enabled();
    ProfilerScope replayScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_include_scaffold_replay");
    bool nestedIncludeBody = pp && pp->includeStack.depth > 1;
    const PPIncludeFrame* includeFrame = pp_include_stack_top(pp);
    bool includePathBody = nestedIncludeBody &&
                           includeFrame &&
                           includeFrame->origin == INCLUDE_SEARCH_INCLUDE_PATH;
    PPSummaryReplayResult result = PP_SUMMARY_REPLAY_USED;

    for (size_t actionIndex = 0; actionIndex < actionCount; ++actionIndex) {
        IncludeSummaryAction* action = &actions[actionIndex];
        size_t cursor = action->start;
        switch (action->kind) {
            case INCLUDE_SUMMARY_ACTION_PRAGMA:
                if (!router_mark_pragma_once(pp, input, action)) {
                    result = PP_SUMMARY_REPLAY_UNSUPPORTED;
                    goto cleanup;
                }
                break;
            case INCLUDE_SUMMARY_ACTION_IFNDEF:
            case INCLUDE_SUMMARY_ACTION_ENDIF:
                break;
            case INCLUDE_SUMMARY_ACTION_RAW_RANGE:
                if (!pp_summary_append_raw_range_direct(pp, input, action, output)) {
                    result = PP_SUMMARY_REPLAY_UNSUPPORTED;
                    goto cleanup;
                }
                break;
            case INCLUDE_SUMMARY_ACTION_DEFINE: {
                ProfilerScope recurseDefineScope = {0};
                ProfilerScope includePathDefineScope = {0};
                if (nestedIncludeBody) recurseDefineScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_define");
                if (includePathBody) includePathDefineScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_include_path_define");
                ProfilerScope defineScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_define");
                bool ok = process_define(pp, input->tokens, input->count, &cursor);
                pp_profiler_end_if_enabled(timingEnabled, defineScope);
                if (includePathBody) pp_profiler_end_if_enabled(timingEnabled, includePathDefineScope);
                if (nestedIncludeBody) pp_profiler_end_if_enabled(timingEnabled, recurseDefineScope);
                if (!ok) {
                    result = PP_SUMMARY_REPLAY_ERROR;
                    goto cleanup;
                }
                break;
            }
            case INCLUDE_SUMMARY_ACTION_INCLUDE:
            case INCLUDE_SUMMARY_ACTION_INCLUDE_NEXT: {
                bool isIncludeNext = action->kind == INCLUDE_SUMMARY_ACTION_INCLUDE_NEXT;
                ProfilerScope recurseIncludeScope = {0};
                ProfilerScope includePathNestedIncludeScope = {0};
                ProfilerScope replayDispatchScope = {0};
                if (nestedIncludeBody) recurseIncludeScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_nested_include");
                if (includePathBody) includePathNestedIncludeScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_include_path_nested_include");
                if (includePathBody && pp_stagea_diagnostic_profile_enabled()) {
                    profiler_record_value("pp_count_include_path_replay_nested_include_dispatch", 1);
                    profiler_record_value(pp_include_replay_nested_dispatch_counter_name(PP_INCLUDE_REPLAY_KIND_SCAFFOLD), 1);
                    replayDispatchScope = pp_profiler_begin_if_enabled(timingEnabled,
                        pp_include_replay_nested_dispatch_scope_name(PP_INCLUDE_REPLAY_KIND_SCAFFOLD));
                }
                ProfilerScope includeScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_include");
                bool ok = process_include(pp, input->tokens, input->count, &cursor, output, isIncludeNext);
                pp_profiler_end_if_enabled(timingEnabled, includeScope);
                if (includePathBody && pp_stagea_diagnostic_profile_enabled()) pp_profiler_end_if_enabled(timingEnabled, replayDispatchScope);
                if (includePathBody) pp_profiler_end_if_enabled(timingEnabled, includePathNestedIncludeScope);
                if (nestedIncludeBody) pp_profiler_end_if_enabled(timingEnabled, recurseIncludeScope);
                if (!ok) {
                    result = PP_SUMMARY_REPLAY_ERROR;
                    goto cleanup;
                }
                break;
            }
            default:
                result = PP_SUMMARY_REPLAY_UNSUPPORTED;
                goto cleanup;
        }
    }

    if (appendEOF) {
        Token eof = {0};
        eof.type = TOKEN_EOF;
        if (!pp_token_buffer_append_token(output, eof)) {
            result = PP_SUMMARY_REPLAY_ERROR;
            goto cleanup;
        }
    }

cleanup:
    pp_profiler_end_if_enabled(timingEnabled, replayScope);
    if (result == PP_SUMMARY_REPLAY_USED && includePathBody) {
        pp_record_include_path_replay_used(PP_INCLUDE_REPLAY_KIND_SCAFFOLD);
    }
    return result;
}

PPSummaryReplayResult preprocess_tokens_summary_replay(Preprocessor* pp,
                                                       const TokenBuffer* input,
                                                       IncludeSummaryAction* actions,
                                                       size_t actionCount,
                                                       PPTokenBuffer* output,
                                                       bool appendEOF) {
    if (!pp || !input || !output) return PP_SUMMARY_REPLAY_ERROR;

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

    if (!actions || actionCount == 0) {
        return PP_SUMMARY_REPLAY_UNSUPPORTED;
    }

    bool timingEnabled = profiler_timing_enabled();
    ProfilerScope replayScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_include_summary_replay");
    PPTokenBuffer chunk = {0};
    PPConditionalFrame* condStack = NULL;
    size_t condDepth = 0;
    size_t condCap = 0;
    bool active = true;
    bool nestedIncludeBody = pp && pp->includeStack.depth > 1;
    const PPIncludeFrame* includeFrame = pp_include_stack_top(pp);
    bool includePathBody = nestedIncludeBody &&
                           includeFrame &&
                           includeFrame->origin == INCLUDE_SEARCH_INCLUDE_PATH;
    uint64_t replayRawRangeNs = 0;
    uint64_t replayDefineNs = 0;
    uint64_t replayIncludeNs = 0;
    uint64_t replayConditionalNs = 0;
    uint64_t replayOtherDirectiveNs = 0;

    PPSummaryReplayResult result = PP_SUMMARY_REPLAY_USED;

    for (size_t actionIndex = 0; actionIndex < actionCount; ++actionIndex) {
        IncludeSummaryAction* action = &actions[actionIndex];
        size_t cursor = action->start;

        if (ctx && pp_debug_layout_enabled()) {
            const char* curLayout = cc_get_data_layout(ctx);
            if (curLayout != baselineLayout) {
                const Token* t = (action->start < input->count) ? &input->tokens[action->start] : NULL;
                LOG_WARN("codegen", "[preprocess_tokens_summary_replay] dataLayout changed to %p at action %zu type=%d line=%d val=%s",
                         (void*)curLayout,
                         actionIndex,
                         t ? t->type : -1,
                         t ? t->line : -1,
                         (t && t->value) ? t->value : "<null>");
                baselineLayout = curLayout;
            }
        }

        switch (action->kind) {
            case INCLUDE_SUMMARY_ACTION_RAW_RANGE:
                if (active) {
                    uint64_t actionStartNs = timingEnabled ? profiler_now_ns() : 0;
                    if (pp_summary_action_raw_range_can_clone_direct(pp, input, action)) {
                        if (!pp_flush_chunk_profiled(pp,
                                                     &chunk,
                                                     output,
                                                     nestedIncludeBody,
                                                     includePathBody)) {
                            result = PP_SUMMARY_REPLAY_ERROR;
                            goto cleanup;
                        }
                        if (!pp_summary_append_raw_range_direct(pp, input, action, output)) {
                            result = PP_SUMMARY_REPLAY_ERROR;
                            goto cleanup;
                        }
                    } else {
                        if (!pp_summary_append_raw_range(pp,
                                                         input->tokens,
                                                         input->count,
                                                         action->start,
                                                         action->end,
                                                         &chunk)) {
                            result = PP_SUMMARY_REPLAY_ERROR;
                            goto cleanup;
                        }
                    }
                    if (actionStartNs != 0) replayRawRangeNs += profiler_now_ns() - actionStartNs;
                }
                break;
            case INCLUDE_SUMMARY_ACTION_DEFINE:
                if (active) {
                    uint64_t actionStartNs = timingEnabled ? profiler_now_ns() : 0;
                    if (!pp_flush_chunk_profiled(pp,
                                                 &chunk,
                                                 output,
                                                 nestedIncludeBody,
                                                 includePathBody)) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    if (pp->preserveDirectives &&
                        !pp_append_directive_span(input->tokens, action->start, action->end, output, pp)) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    ProfilerScope recurseDefineScope = {0};
                    ProfilerScope includePathDefineScope = {0};
                    if (nestedIncludeBody) recurseDefineScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_define");
                    if (includePathBody) includePathDefineScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_include_path_define");
                    ProfilerScope defineScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_define");
                    bool ok = process_define(pp, input->tokens, input->count, &cursor);
                    pp_profiler_end_if_enabled(timingEnabled, defineScope);
                    if (includePathBody) pp_profiler_end_if_enabled(timingEnabled, includePathDefineScope);
                    if (nestedIncludeBody) pp_profiler_end_if_enabled(timingEnabled, recurseDefineScope);
                    if (!ok) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    if (actionStartNs != 0) replayDefineNs += profiler_now_ns() - actionStartNs;
                }
                break;
            case INCLUDE_SUMMARY_ACTION_UNDEF:
                if (active) {
                    uint64_t actionStartNs = timingEnabled ? profiler_now_ns() : 0;
                    if (!pp_flush_chunk_profiled(pp,
                                                 &chunk,
                                                 output,
                                                 nestedIncludeBody,
                                                 includePathBody)) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    if (pp->preserveDirectives &&
                        !pp_append_directive_span(input->tokens, action->start, action->end, output, pp)) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    ProfilerScope recurseUndefScope = {0};
                    ProfilerScope includePathUndefScope = {0};
                    if (nestedIncludeBody) recurseUndefScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_undef");
                    if (includePathBody) includePathUndefScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_include_path_undef");
                    ProfilerScope undefScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_undef");
                    bool ok = process_undef(pp, input->tokens, input->count, &cursor);
                    pp_profiler_end_if_enabled(timingEnabled, undefScope);
                    if (includePathBody) pp_profiler_end_if_enabled(timingEnabled, includePathUndefScope);
                    if (nestedIncludeBody) pp_profiler_end_if_enabled(timingEnabled, recurseUndefScope);
                    if (!ok) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    if (actionStartNs != 0) replayOtherDirectiveNs += profiler_now_ns() - actionStartNs;
                }
                break;
            case INCLUDE_SUMMARY_ACTION_LINE:
                if (active) {
                    uint64_t actionStartNs = timingEnabled ? profiler_now_ns() : 0;
                    if (!pp_flush_chunk_profiled(pp,
                                                 &chunk,
                                                 output,
                                                 nestedIncludeBody,
                                                 includePathBody)) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    if (pp->preserveDirectives &&
                        !pp_append_directive_span(input->tokens, action->start, action->end, output, pp)) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    ProfilerScope recurseLineScope = {0};
                    ProfilerScope includePathLineScope = {0};
                    if (nestedIncludeBody) recurseLineScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_line");
                    if (includePathBody) includePathLineScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_include_path_line");
                    ProfilerScope lineScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_line");
                    bool ok = process_line_directive(pp, input->tokens, input->count, &cursor);
                    pp_profiler_end_if_enabled(timingEnabled, lineScope);
                    if (includePathBody) pp_profiler_end_if_enabled(timingEnabled, includePathLineScope);
                    if (nestedIncludeBody) pp_profiler_end_if_enabled(timingEnabled, recurseLineScope);
                    if (!ok) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    if (actionStartNs != 0) replayOtherDirectiveNs += profiler_now_ns() - actionStartNs;
                }
                break;
            case INCLUDE_SUMMARY_ACTION_DIAGNOSTIC:
                if (active) {
                    uint64_t actionStartNs = timingEnabled ? profiler_now_ns() : 0;
                    if (!pp_flush_chunk_profiled(pp,
                                                 &chunk,
                                                 output,
                                                 nestedIncludeBody,
                                                 includePathBody)) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    if (pp->preserveDirectives &&
                        !pp_append_directive_span(input->tokens, action->start, action->end, output, pp)) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    const Token* tok = &input->tokens[cursor];
                    bool isError = tok->value && strcmp(tok->value, "error") == 0;
                    char buffer[256];
                    buffer[0] = '\0';
                    size_t diagCursor = cursor + 1;
                    int directiveLine = tok->line;
                    while (diagCursor < action->end &&
                           diagCursor < input->count &&
                           input->tokens[diagCursor].type != TOKEN_EOF &&
                           input->tokens[diagCursor].line == directiveLine) {
                        const Token* lt = &input->tokens[diagCursor];
                        const char* piece = lt->value ? lt->value : "";
                        if (strlen(buffer) + strlen(piece) + 2 < sizeof(buffer)) {
                            if (buffer[0] != '\0') {
                                strcat(buffer, " ");
                            }
                            strcat(buffer, piece);
                        }
                        diagCursor++;
                    }
                    DiagKind kind = isError ? DIAG_ERROR : DIAG_WARNING;
                    const char* msg = buffer[0] ? buffer : (isError ? "#error" : "#warning");
                    pp_report_diag(pp, tok, kind, CDIAG_PREPROCESSOR_GENERIC, "%s", msg);
                    cursor = (diagCursor == 0) ? 0 : diagCursor - 1;
                    if (actionStartNs != 0) replayOtherDirectiveNs += profiler_now_ns() - actionStartNs;
                }
                break;
            case INCLUDE_SUMMARY_ACTION_INCLUDE:
            case INCLUDE_SUMMARY_ACTION_INCLUDE_NEXT:
                if (active) {
                    uint64_t actionStartNs = timingEnabled ? profiler_now_ns() : 0;
                    if (!pp_flush_chunk_profiled(pp,
                                                 &chunk,
                                                 output,
                                                 nestedIncludeBody,
                                                 includePathBody)) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    if (pp->preserveDirectives &&
                        !pp_append_directive_span(input->tokens, action->start, action->end, output, pp)) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    bool isIncludeNext = action->kind == INCLUDE_SUMMARY_ACTION_INCLUDE_NEXT;
                    ProfilerScope recurseIncludeScope = {0};
                    ProfilerScope includePathNestedIncludeScope = {0};
                    ProfilerScope replayDispatchScope = {0};
                    if (nestedIncludeBody) recurseIncludeScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_nested_include");
                    if (includePathBody) includePathNestedIncludeScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_include_path_nested_include");
                    if (includePathBody && pp_stagea_diagnostic_profile_enabled()) {
                        profiler_record_value("pp_count_include_path_replay_nested_include_dispatch", 1);
                        profiler_record_value(pp_include_replay_nested_dispatch_counter_name(PP_INCLUDE_REPLAY_KIND_SUMMARY), 1);
                        replayDispatchScope = pp_profiler_begin_if_enabled(timingEnabled,
                            pp_include_replay_nested_dispatch_scope_name(PP_INCLUDE_REPLAY_KIND_SUMMARY));
                    }
                    ProfilerScope includeScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_include");
                    bool ok = process_include(pp, input->tokens, input->count, &cursor, output, isIncludeNext);
                    pp_profiler_end_if_enabled(timingEnabled, includeScope);
                    if (includePathBody && pp_stagea_diagnostic_profile_enabled()) {
                        pp_profiler_end_if_enabled(timingEnabled, replayDispatchScope);
                    }
                    if (includePathBody) pp_profiler_end_if_enabled(timingEnabled, includePathNestedIncludeScope);
                    if (nestedIncludeBody) pp_profiler_end_if_enabled(timingEnabled, recurseIncludeScope);
                    if (!ok) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    if (actionStartNs != 0) replayIncludeNs += profiler_now_ns() - actionStartNs;
                }
                break;
            case INCLUDE_SUMMARY_ACTION_IF: {
                uint64_t actionStartNs = timingEnabled ? profiler_now_ns() : 0;
                if (!pp_flush_chunk_profiled(pp,
                                             &chunk,
                                             output,
                                             nestedIncludeBody,
                                             includePathBody)) {
                    result = PP_SUMMARY_REPLAY_ERROR;
                    goto cleanup;
                }
                ProfilerScope recurseIfScope = {0};
                ProfilerScope includePathConditionalScope = {0};
                if (nestedIncludeBody) recurseIfScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_conditional");
                if (includePathBody) includePathConditionalScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_include_path_conditional");
                ProfilerScope ifScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_if");
                bool ok = process_if(pp, input->tokens, input->count, &cursor,
                                     &condStack, &condDepth, &condCap);
                pp_profiler_end_if_enabled(timingEnabled, ifScope);
                if (includePathBody) pp_profiler_end_if_enabled(timingEnabled, includePathConditionalScope);
                if (nestedIncludeBody) pp_profiler_end_if_enabled(timingEnabled, recurseIfScope);
                if (!ok) {
                    result = PP_SUMMARY_REPLAY_ERROR;
                    goto cleanup;
                }
                active = conditional_stack_is_active(condStack, condDepth);
                if (actionStartNs != 0) replayConditionalNs += profiler_now_ns() - actionStartNs;
                break;
            }
            case INCLUDE_SUMMARY_ACTION_IFDEF:
            case INCLUDE_SUMMARY_ACTION_IFNDEF: {
                uint64_t actionStartNs = timingEnabled ? profiler_now_ns() : 0;
                if (!pp_flush_chunk_profiled(pp,
                                             &chunk,
                                             output,
                                             nestedIncludeBody,
                                             includePathBody)) {
                    result = PP_SUMMARY_REPLAY_ERROR;
                    goto cleanup;
                }
                bool negate = action->kind == INCLUDE_SUMMARY_ACTION_IFNDEF;
                ProfilerScope recurseIfdefScope = {0};
                ProfilerScope includePathIfdefScope = {0};
                if (nestedIncludeBody) recurseIfdefScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_conditional");
                if (includePathBody) includePathIfdefScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_include_path_conditional");
                ProfilerScope ifdefScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_ifdef");
                bool ok = process_ifdeflike(pp, input->tokens, input->count, &cursor,
                                            &condStack, &condDepth, &condCap, negate);
                pp_profiler_end_if_enabled(timingEnabled, ifdefScope);
                if (includePathBody) pp_profiler_end_if_enabled(timingEnabled, includePathIfdefScope);
                if (nestedIncludeBody) pp_profiler_end_if_enabled(timingEnabled, recurseIfdefScope);
                if (!ok) {
                    result = PP_SUMMARY_REPLAY_ERROR;
                    goto cleanup;
                }
                active = conditional_stack_is_active(condStack, condDepth);
                if (actionStartNs != 0) replayConditionalNs += profiler_now_ns() - actionStartNs;
                break;
            }
            case INCLUDE_SUMMARY_ACTION_ELIF: {
                uint64_t actionStartNs = timingEnabled ? profiler_now_ns() : 0;
                if (!pp_flush_chunk_profiled(pp,
                                             &chunk,
                                             output,
                                             nestedIncludeBody,
                                             includePathBody)) {
                    result = PP_SUMMARY_REPLAY_ERROR;
                    goto cleanup;
                }
                ProfilerScope recurseElifScope = {0};
                ProfilerScope includePathElifScope = {0};
                if (nestedIncludeBody) recurseElifScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_conditional");
                if (includePathBody) includePathElifScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_include_path_conditional");
                ProfilerScope elifScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_elif");
                bool ok = process_elif(pp, input->tokens, input->count, &cursor, condStack, condDepth);
                pp_profiler_end_if_enabled(timingEnabled, elifScope);
                if (includePathBody) pp_profiler_end_if_enabled(timingEnabled, includePathElifScope);
                if (nestedIncludeBody) pp_profiler_end_if_enabled(timingEnabled, recurseElifScope);
                if (!ok) {
                    result = PP_SUMMARY_REPLAY_ERROR;
                    goto cleanup;
                }
                active = conditional_stack_is_active(condStack, condDepth);
                if (actionStartNs != 0) replayConditionalNs += profiler_now_ns() - actionStartNs;
                break;
            }
            case INCLUDE_SUMMARY_ACTION_ELSE: {
                uint64_t actionStartNs = timingEnabled ? profiler_now_ns() : 0;
                if (!pp_flush_chunk_profiled(pp,
                                             &chunk,
                                             output,
                                             nestedIncludeBody,
                                             includePathBody)) {
                    result = PP_SUMMARY_REPLAY_ERROR;
                    goto cleanup;
                }
                ProfilerScope recurseElseScope = {0};
                ProfilerScope includePathElseScope = {0};
                if (nestedIncludeBody) recurseElseScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_conditional");
                if (includePathBody) includePathElseScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_include_path_conditional");
                bool ok = (action->flags & INCLUDE_SUMMARY_ACTION_FLAG_NO_TRAILING_TOKENS) != 0 &&
                          process_else(pp, condStack, condDepth, &input->tokens[cursor]);
                if (includePathBody) pp_profiler_end_if_enabled(timingEnabled, includePathElseScope);
                if (nestedIncludeBody) pp_profiler_end_if_enabled(timingEnabled, recurseElseScope);
                if (!ok) {
                    result = PP_SUMMARY_REPLAY_ERROR;
                    goto cleanup;
                }
                active = conditional_stack_is_active(condStack, condDepth);
                if (actionStartNs != 0) replayConditionalNs += profiler_now_ns() - actionStartNs;
                break;
            }
            case INCLUDE_SUMMARY_ACTION_ENDIF: {
                uint64_t actionStartNs = timingEnabled ? profiler_now_ns() : 0;
                if (!pp_flush_chunk_profiled(pp,
                                             &chunk,
                                             output,
                                             nestedIncludeBody,
                                             includePathBody)) {
                    result = PP_SUMMARY_REPLAY_ERROR;
                    goto cleanup;
                }
                ProfilerScope recurseEndifScope = {0};
                ProfilerScope includePathEndifScope = {0};
                if (nestedIncludeBody) recurseEndifScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_conditional");
                if (includePathBody) includePathEndifScope = pp_profiler_begin_if_enabled(timingEnabled, "pp_recurse_include_path_conditional");
                bool ok = (action->flags & INCLUDE_SUMMARY_ACTION_FLAG_NO_TRAILING_TOKENS) != 0;
                if (ok && pp->preserveDirectives && active) {
                    ok = pp_append_directive_span(input->tokens, action->start, action->end, output, pp);
                }
                if (ok) {
                    ok = process_endif(pp, condStack, &condDepth, &input->tokens[cursor]);
                }
                if (includePathBody) pp_profiler_end_if_enabled(timingEnabled, includePathEndifScope);
                if (nestedIncludeBody) pp_profiler_end_if_enabled(timingEnabled, recurseEndifScope);
                if (!ok) {
                    result = PP_SUMMARY_REPLAY_ERROR;
                    goto cleanup;
                }
                active = conditional_stack_is_active(condStack, condDepth);
                if (actionStartNs != 0) replayConditionalNs += profiler_now_ns() - actionStartNs;
                break;
            }
            case INCLUDE_SUMMARY_ACTION_PRAGMA:
                if (active) {
                    uint64_t actionStartNs = timingEnabled ? profiler_now_ns() : 0;
                    if (!pp_flush_chunk_profiled(pp,
                                                 &chunk,
                                                 output,
                                                 nestedIncludeBody,
                                                 includePathBody)) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    if (pp->preserveDirectives &&
                        !pp_append_directive_span(input->tokens, action->start, action->end, output, pp)) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    if (!process_pragma(pp, input->tokens, input->count, &cursor)) {
                        result = PP_SUMMARY_REPLAY_ERROR;
                        goto cleanup;
                    }
                    if (actionStartNs != 0) replayOtherDirectiveNs += profiler_now_ns() - actionStartNs;
                }
                break;
        }
    }

    if (!pp_flush_chunk_profiled(pp,
                                 &chunk,
                                 output,
                                 nestedIncludeBody,
                                 includePathBody)) {
        result = PP_SUMMARY_REPLAY_ERROR;
        goto cleanup;
    }

    if (appendEOF) {
        Token eof = {0};
        eof.type = TOKEN_EOF;
        if (!pp_token_buffer_append_token(output, eof)) {
            result = PP_SUMMARY_REPLAY_ERROR;
            goto cleanup;
        }
    }

    if (condDepth != 0) {
        pp_report_diag(pp, NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "unclosed #if/#ifdef/#ifndef");
        result = PP_SUMMARY_REPLAY_ERROR;
    }

cleanup:
    profiler_end(replayScope);
    if (timingEnabled) {
        profiler_record_duration("pp_include_summary_replay_raw_range", replayRawRangeNs);
        profiler_record_duration("pp_include_summary_replay_define", replayDefineNs);
        profiler_record_duration("pp_include_summary_replay_include", replayIncludeNs);
        profiler_record_duration("pp_include_summary_replay_conditional", replayConditionalNs);
        profiler_record_duration("pp_include_summary_replay_other_directive", replayOtherDirectiveNs);
    }
    if (result == PP_SUMMARY_REPLAY_USED && includePathBody) {
        pp_record_include_path_replay_used(PP_INCLUDE_REPLAY_KIND_SUMMARY);
    }
    pp_token_buffer_reset(&chunk);
    free(condStack);
    return result;
}
