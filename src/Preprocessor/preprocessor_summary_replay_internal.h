// SPDX-License-Identifier: Apache-2.0

#ifndef PREPROCESSOR_SUMMARY_REPLAY_INTERNAL_H
#define PREPROCESSOR_SUMMARY_REPLAY_INTERNAL_H

#include "Preprocessor/pp_internal.h"

typedef enum {
    PP_INCLUDE_REPLAY_KIND_ROUTER = 0,
    PP_INCLUDE_REPLAY_KIND_SCAFFOLD,
    PP_INCLUDE_REPLAY_KIND_SUMMARY,
} PPIncludeReplayKind;

bool pp_stagea_diagnostic_profile_enabled(void);
const char* pp_include_replay_nested_dispatch_counter_name(PPIncludeReplayKind kind);
const char* pp_include_replay_nested_dispatch_scope_name(PPIncludeReplayKind kind);
void pp_record_include_path_replay_used(PPIncludeReplayKind kind);
bool pp_router_replay_allowed(const char* filePath,
                              const IncludeSummaryProbe* probe,
                              size_t actionCount);
bool pp_summary_append_raw_range(Preprocessor* pp,
                                 const Token* tokens,
                                 size_t count,
                                 size_t start,
                                 size_t end,
                                 PPTokenBuffer* chunk);
bool pp_summary_action_raw_range_can_clone_direct(Preprocessor* pp,
                                                  const TokenBuffer* input,
                                                  IncludeSummaryAction* action);
bool pp_summary_append_raw_range_direct(Preprocessor* pp,
                                        const TokenBuffer* input,
                                        IncludeSummaryAction* action,
                                        PPTokenBuffer* output);
bool pp_router_mark_pragma_once(Preprocessor* pp,
                                const TokenBuffer* input,
                                const IncludeSummaryAction* action);

#endif // PREPROCESSOR_SUMMARY_REPLAY_INTERNAL_H
