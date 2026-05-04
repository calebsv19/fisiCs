// SPDX-License-Identifier: Apache-2.0

#ifndef PREPROCESSOR_INCLUDE_SUPPORT_H
#define PREPROCESSOR_INCLUDE_SUPPORT_H

#include "Preprocessor/pp_internal.h"

bool parse_include_operand(const Token* tokens,
                           size_t count,
                           size_t* cursor,
                           char** outName,
                           bool* outIsSystem);
bool include_name_is_suspicious(const char* name);
const char* detect_include_guard(const TokenBuffer* buffer);
bool classify_include_summary_router_only(const TokenBuffer* buffer,
                                          const IncludeSummaryAction* actions,
                                          size_t actionCount,
                                          const char* guard,
                                          IncludeSummaryProbe* probe);
bool classify_include_summary_router_raw_tail(const TokenBuffer* buffer,
                                              const IncludeSummaryAction* actions,
                                              size_t actionCount,
                                              const char* guard,
                                              IncludeSummaryProbe* probe);
IncludeHeaderBehaviorClass classify_include_summary_behavior_class(Preprocessor* pp,
                                                                  const TokenBuffer* buffer,
                                                                  const IncludeSummaryAction* actions,
                                                                  size_t actionCount,
                                                                  const char* guard,
                                                                  IncludeSummaryProbe* probe);
IncludeSummaryProbe analyze_include_summary_probe(const TokenBuffer* buffer,
                                                  IncludeSummaryAction** actionsOut,
                                                  size_t* actionCountOut);
void pp_profile_event(const char* name);
void pp_profile_summary_probe(const IncludeSummaryProbe* probe);
void pp_profile_behavior_class(const IncludeSummaryProbe* probe);
void pp_profile_conditional_scaffold_shape(Preprocessor* pp,
                                           const TokenBuffer* buffer,
                                           const IncludeSummaryProbe* probe,
                                           const IncludeSummaryAction* actions,
                                           size_t actionCount,
                                           const char* guard);
void pp_profile_summary_probe_scan_result(const IncludeSummaryProbe* probe);
bool pp_summary_replay_experiment_enabled(void);
const char* nested_recurse_scope_name(bool repeatSeen, IncludeSearchOrigin origin);

#endif /* PREPROCESSOR_INCLUDE_SUPPORT_H */
