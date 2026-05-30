// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/preprocessor_include_support.h"

#include <stdlib.h>

#include "Utils/profiler.h"

void pp_profile_event(const char* name) {
    if (!name || !profiler_counters_enabled()) return;
    profiler_record_value(name, 1);
}

void pp_profile_summary_probe(const IncludeSummaryProbe* probe) {
    if (!probe) return;
    if (probe->status == INCLUDE_SUMMARY_PROBE_CANDIDATE && probe->routerOnly) {
        pp_profile_event("pp_include_summary_probe_router_only");
    }
    if (probe->status == INCLUDE_SUMMARY_PROBE_CANDIDATE && probe->routerRawTail) {
        pp_profile_event("pp_include_summary_probe_router_raw_tail");
    }
    switch (probe->status) {
        case INCLUDE_SUMMARY_PROBE_CANDIDATE:
            pp_profile_event("pp_include_summary_probe_candidate");
            break;
        case INCLUDE_SUMMARY_PROBE_REJECT_NON_LITERAL_INCLUDE:
            pp_profile_event("pp_include_summary_probe_reject_non_literal_include");
            break;
        case INCLUDE_SUMMARY_PROBE_REJECT_UNSUPPORTED_DIRECTIVE:
            pp_profile_event("pp_include_summary_probe_reject_unsupported_directive");
            break;
        case INCLUDE_SUMMARY_PROBE_UNKNOWN:
        default:
            pp_profile_event("pp_include_summary_probe_unknown");
            break;
    }
}

void pp_profile_behavior_class(const IncludeSummaryProbe* probe) {
    if (!probe || probe->status != INCLUDE_SUMMARY_PROBE_CANDIDATE) return;
    switch (probe->behaviorClass) {
        case INCLUDE_HEADER_BEHAVIOR_ROUTER_ONLY:
            profiler_record_value("pp_include_behavior_class_router_only", 1);
            break;
        case INCLUDE_HEADER_BEHAVIOR_ROUTER_RAW_TAIL:
            profiler_record_value("pp_include_behavior_class_router_raw_tail", 1);
            break;
        case INCLUDE_HEADER_BEHAVIOR_INCLUDE_DEFINE_SCAFFOLD:
            profiler_record_value("pp_include_behavior_class_include_define_scaffold", 1);
            break;
        case INCLUDE_HEADER_BEHAVIOR_CONDITIONAL_SCAFFOLD:
            profiler_record_value("pp_include_behavior_class_conditional_scaffold", 1);
            break;
        case INCLUDE_HEADER_BEHAVIOR_GENERAL_FALLBACK:
            profiler_record_value("pp_include_behavior_class_general_fallback", 1);
            break;
        case INCLUDE_HEADER_BEHAVIOR_UNKNOWN:
        default:
            break;
    }
}

void pp_profile_summary_probe_scan_result(const IncludeSummaryProbe* probe) {
    if (!probe) return;
    if (probe->status == INCLUDE_SUMMARY_PROBE_CANDIDATE && probe->routerOnly) {
        pp_profile_event("pp_include_summary_probe_scan_router_only");
    }
    if (probe->status == INCLUDE_SUMMARY_PROBE_CANDIDATE && probe->routerRawTail) {
        pp_profile_event("pp_include_summary_probe_scan_router_raw_tail");
    }
    switch (probe->status) {
        case INCLUDE_SUMMARY_PROBE_CANDIDATE:
            pp_profile_event("pp_include_summary_probe_scan_candidate");
            break;
        case INCLUDE_SUMMARY_PROBE_REJECT_NON_LITERAL_INCLUDE:
            pp_profile_event("pp_include_summary_probe_scan_reject_non_literal_include");
            break;
        case INCLUDE_SUMMARY_PROBE_REJECT_UNSUPPORTED_DIRECTIVE:
            pp_profile_event("pp_include_summary_probe_scan_reject_unsupported_directive");
            break;
        case INCLUDE_SUMMARY_PROBE_UNKNOWN:
        default:
            pp_profile_event("pp_include_summary_probe_scan_unknown");
            break;
    }
}

bool pp_summary_replay_experiment_enabled(void) {
    static int initialized = 0;
    static bool enabled = false;
    if (!initialized) {
        const char* env = getenv("FISICS_PP_SUMMARY_REPLAY_EXPERIMENT");
        enabled = (env && env[0] && env[0] != '0');
        initialized = 1;
    }
    return enabled;
}

static bool pp_env_toggle_enabled(const char* name) {
    const char* env = getenv(name);
    return env && env[0] && env[0] != '0';
}

bool pp_summary_replay_router_enabled(void) {
    return pp_env_toggle_enabled("FISICS_PP_SUMMARY_REPLAY_ROUTER") ||
           pp_summary_replay_experiment_enabled();
}

bool pp_summary_replay_scaffold_enabled(void) {
    return pp_env_toggle_enabled("FISICS_PP_SUMMARY_REPLAY_SCAFFOLD") ||
           pp_summary_replay_experiment_enabled();
}

bool pp_summary_replay_general_enabled(void) {
    return pp_env_toggle_enabled("FISICS_PP_SUMMARY_REPLAY_GENERAL") ||
           pp_summary_replay_experiment_enabled();
}

const char* nested_recurse_scope_name(bool repeatSeen, IncludeSearchOrigin origin) {
    switch (origin) {
        case INCLUDE_SEARCH_SAME_DIR:
            return repeatSeen
                ? "pp_recurse_nested_include_repeat_same_dir"
                : "pp_recurse_nested_include_first_same_dir";
        case INCLUDE_SEARCH_INCLUDE_PATH:
            return repeatSeen
                ? "pp_recurse_nested_include_repeat_include_path"
                : "pp_recurse_nested_include_first_include_path";
        case INCLUDE_SEARCH_RAW:
        default:
            return repeatSeen
                ? "pp_recurse_nested_include_repeat_raw"
                : "pp_recurse_nested_include_first_raw";
    }
}
