// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/preprocessor_summary_replay_internal.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool pp_stagea_diagnostic_profile_enabled(void) {
    static int initialized = 0;
    static bool enabled = false;
    if (!initialized) {
        const char* env = getenv("FISICS_PP_STAGEA_DIAGNOSTIC_PROFILE");
        enabled = (env && env[0] && env[0] != '0');
        initialized = 1;
    }
    return enabled;
}

const char* pp_include_replay_nested_dispatch_counter_name(PPIncludeReplayKind kind) {
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

const char* pp_include_replay_nested_dispatch_scope_name(PPIncludeReplayKind kind) {
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

void pp_record_include_path_replay_used(PPIncludeReplayKind kind) {
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

bool pp_router_replay_allowed(const char* filePath,
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

bool pp_summary_append_raw_range(Preprocessor* pp,
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

bool pp_router_mark_pragma_once(Preprocessor* pp,
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
    for (size_t i = start; i < end && i < count; ++i) {
        const Token* tok = &tokens[i];
        if (tok->type == TOKEN_EOF) break;
        switch (tok->type) {
            case TOKEN_UNKNOWN:
                return false;
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
    return end > start;
}

bool pp_summary_action_raw_range_can_clone_direct(Preprocessor* pp,
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

bool pp_summary_append_raw_range_direct(Preprocessor* pp,
                                        const TokenBuffer* input,
                                        IncludeSummaryAction* action,
                                        PPTokenBuffer* output) {
    if (!pp || !input || !input->tokens || !action || !output) return false;
    if (!pp_summary_action_raw_range_can_clone_direct(pp, input, action)) {
        return false;
    }
    size_t start = action->start;
    size_t spanEnd = action->end < input->count ? action->end : input->count;
    return pp_token_buffer_append_clone_remap_span(output, pp, input->tokens + start, spanEnd - start);
}
