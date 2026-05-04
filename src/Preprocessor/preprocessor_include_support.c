// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/preprocessor_include_support.h"

#include <stdlib.h>
#include <string.h>

#include "Utils/profiler.h"

static void strip_include_spaces(char* name) {
    if (!name) return;
    char* out = name;
    for (char* p = name; *p; ++p) {
        if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
            continue;
        }
        *out++ = *p;
    }
    *out = '\0';
}

bool parse_include_operand(const Token* tokens,
                           size_t count,
                           size_t* cursor,
                           char** outName,
                           bool* outIsSystem) {
    size_t i = *cursor + 1;
    int directiveLine = tokens[*cursor].line;
    if (i >= count) return false;

    const Token* tok = &tokens[i];
    if (tok->type == TOKEN_STRING) {
        *outName = pp_strdup_local(tok->value);
        *outIsSystem = false;
        if (!*outName) return false;
        strip_include_spaces(*outName);
        i++;
    } else if (tok->type == TOKEN_LESS) {
        char buffer[1024];
        size_t len = 0;
        i++;
        bool closed = false;
        while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
            if (tokens[i].type == TOKEN_GREATER) {
                closed = true;
                i++;
                break;
            }
            const char* piece = tokens[i].value ? tokens[i].value : "";
            size_t pieceLen = strlen(piece);
            if (len + pieceLen + 1 >= sizeof(buffer)) {
                return false;
            }
            memcpy(buffer + len, piece, pieceLen);
            len += pieceLen;
            i++;
        }
        if (!closed) return false;
        buffer[len] = '\0';
        *outName = pp_strdup_local(buffer);
        *outIsSystem = true;
        if (!*outName) return false;
        strip_include_spaces(*outName);
    } else {
        return false;
    }

    if (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
        return false;
    }

    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
        i++;
    }
    *cursor = (i == 0) ? 0 : i - 1;
    return true;
}

bool include_name_is_suspicious(const char* name) {
    if (!name || !name[0]) return true;
    for (const unsigned char* p = (const unsigned char*)name; *p; ++p) {
        unsigned char c = *p;
        bool ok =
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_' || c == '-' || c == '.' || c == '/' || c == '+';
        if (!ok) return true;
    }
    return false;
}

const char* detect_include_guard(const TokenBuffer* buffer) {
    if (!buffer || buffer->count < 3) return NULL;
    const Token* tokens = buffer->tokens;
    if (tokens[0].type != TOKEN_IFNDEF) return NULL;
    if (tokens[1].type != TOKEN_IDENTIFIER) return NULL;
    const char* guard = tokens[1].value;
    size_t i = 2;
    int firstLine = tokens[0].line;
    while (i < buffer->count && tokens[i].type != TOKEN_EOF && tokens[i].line == firstLine) {
        i++;
    }
    if (i >= buffer->count || tokens[i].type == TOKEN_EOF) return NULL;
    int defineLine = tokens[i].line;
    if (tokens[i].type != TOKEN_DEFINE) return NULL;
    i++;
    if (i >= buffer->count || tokens[i].type == TOKEN_EOF || tokens[i].line != defineLine) return NULL;
    if (tokens[i].type != TOKEN_IDENTIFIER) return NULL;
    if (strcmp(tokens[i].value, guard) != 0) return NULL;
    return guard;
}

static size_t skip_directive_line_cursor(const Token* tokens,
                                         size_t count,
                                         size_t cursor) {
    int directiveLine = tokens[cursor].line;
    while (cursor < count &&
           tokens[cursor].type != TOKEN_EOF &&
           tokens[cursor].line == directiveLine) {
        cursor++;
    }
    return cursor;
}

static bool pragma_is_once_directive(const Token* tokens,
                                     size_t count,
                                     size_t cursor) {
    int directiveLine = tokens[cursor].line;
    size_t i = cursor + 1;
    if (i >= count || tokens[i].line != directiveLine || tokens[i].type != TOKEN_ONCE) {
        return false;
    }
    i++;
    return i >= count || tokens[i].type == TOKEN_EOF || tokens[i].line != directiveLine;
}

static bool include_summary_action_append(IncludeSummaryAction** actions,
                                          size_t* count,
                                          size_t* capacity,
                                          IncludeSummaryAction action) {
    if (!actions || !count || !capacity) return false;
    if (*count == *capacity) {
        size_t newCapacity = *capacity ? (*capacity * 2) : 16;
        IncludeSummaryAction* grown =
            (IncludeSummaryAction*)realloc(*actions, newCapacity * sizeof(IncludeSummaryAction));
        if (!grown) return false;
        *actions = grown;
        *capacity = newCapacity;
    }
    (*actions)[(*count)++] = action;
    return true;
}

static bool directive_line_matches_single_identifier(const Token* tokens,
                                                     size_t count,
                                                     size_t cursor,
                                                     const char* ident) {
    if (!tokens || !ident || cursor >= count) return false;
    int directiveLine = tokens[cursor].line;
    size_t i = cursor + 1;
    if (i >= count || tokens[i].line != directiveLine || tokens[i].type != TOKEN_IDENTIFIER || !tokens[i].value) {
        return false;
    }
    if (strcmp(tokens[i].value, ident) != 0) {
        return false;
    }
    ++i;
    return i >= count || tokens[i].type == TOKEN_EOF || tokens[i].line != directiveLine;
}

static bool directive_line_matches_empty_guard_define(const Token* tokens,
                                                      size_t count,
                                                      size_t cursor,
                                                      const char* ident) {
    if (!tokens || !ident || cursor >= count) return false;
    int directiveLine = tokens[cursor].line;
    size_t i = cursor + 1;
    if (i >= count || tokens[i].line != directiveLine || tokens[i].type != TOKEN_IDENTIFIER || !tokens[i].value) {
        return false;
    }
    if (strcmp(tokens[i].value, ident) != 0) {
        return false;
    }
    ++i;
    return i >= count || tokens[i].type == TOKEN_EOF || tokens[i].line != directiveLine;
}

bool classify_include_summary_router_only(const TokenBuffer* buffer,
                                          const IncludeSummaryAction* actions,
                                          size_t actionCount,
                                          const char* guard,
                                          IncludeSummaryProbe* probe) {
    if (!buffer || !buffer->tokens || !actions || actionCount == 0 || !guard || !guard[0] || !probe) {
        return false;
    }

    bool sawIfndef = false;
    bool sawDefine = false;
    bool sawEndif = false;
    bool sawInclude = false;

    for (size_t i = 0; i < actionCount; ++i) {
        const IncludeSummaryAction* action = &actions[i];
        switch (action->kind) {
            case INCLUDE_SUMMARY_ACTION_PRAGMA:
                if (!pragma_is_once_directive(buffer->tokens, buffer->count, action->start)) {
                    return false;
                }
                probe->routerHasPragmaOnce = true;
                break;
            case INCLUDE_SUMMARY_ACTION_IFNDEF:
                if (sawIfndef || sawDefine || sawEndif) {
                    return false;
                }
                if (!directive_line_matches_single_identifier(buffer->tokens, buffer->count, action->start, guard)) {
                    return false;
                }
                sawIfndef = true;
                break;
            case INCLUDE_SUMMARY_ACTION_DEFINE:
                if (!sawIfndef || sawDefine || sawEndif) {
                    return false;
                }
                if (!directive_line_matches_empty_guard_define(buffer->tokens,
                                                               buffer->count,
                                                               action->start,
                                                               guard)) {
                    return false;
                }
                sawDefine = true;
                break;
            case INCLUDE_SUMMARY_ACTION_INCLUDE:
            case INCLUDE_SUMMARY_ACTION_INCLUDE_NEXT:
                if (!sawIfndef || !sawDefine || sawEndif) {
                    return false;
                }
                sawInclude = true;
                break;
            case INCLUDE_SUMMARY_ACTION_ENDIF:
                if (!sawIfndef || !sawDefine || sawEndif || !sawInclude) {
                    return false;
                }
                if (i + 1 != actionCount) {
                    return false;
                }
                sawEndif = true;
                break;
            default:
                return false;
        }
    }

    return sawIfndef && sawDefine && sawInclude && sawEndif;
}

static bool raw_range_starts_with_typedef(const TokenBuffer* buffer,
                                          const IncludeSummaryAction* action) {
    if (!buffer || !buffer->tokens || !action) return false;
    for (size_t i = action->start; i < action->end && i < buffer->count; ++i) {
        TokenType type = buffer->tokens[i].type;
        if (type == TOKEN_EOF || type == TOKEN_UNKNOWN) {
            continue;
        }
        return type == TOKEN_TYPEDEF;
    }
    return false;
}

bool classify_include_summary_router_raw_tail(const TokenBuffer* buffer,
                                              const IncludeSummaryAction* actions,
                                              size_t actionCount,
                                              const char* guard,
                                              IncludeSummaryProbe* probe) {
    if (!buffer || !buffer->tokens || !actions || actionCount == 0 || !guard || !guard[0] || !probe) {
        return false;
    }

    bool sawIfndef = false;
    bool sawDefine = false;
    bool sawEndif = false;
    bool sawInclude = false;
    bool sawRawTail = false;

    for (size_t i = 0; i < actionCount; ++i) {
        const IncludeSummaryAction* action = &actions[i];
        switch (action->kind) {
            case INCLUDE_SUMMARY_ACTION_PRAGMA:
                if (!pragma_is_once_directive(buffer->tokens, buffer->count, action->start)) {
                    return false;
                }
                probe->routerHasPragmaOnce = true;
                break;
            case INCLUDE_SUMMARY_ACTION_IFNDEF:
                if (sawIfndef || sawDefine || sawEndif) {
                    return false;
                }
                if (!directive_line_matches_single_identifier(buffer->tokens, buffer->count, action->start, guard)) {
                    return false;
                }
                sawIfndef = true;
                break;
            case INCLUDE_SUMMARY_ACTION_DEFINE:
                if (!sawIfndef || sawDefine || sawEndif) {
                    return false;
                }
                if (!directive_line_matches_empty_guard_define(buffer->tokens,
                                                               buffer->count,
                                                               action->start,
                                                               guard)) {
                    return false;
                }
                sawDefine = true;
                break;
            case INCLUDE_SUMMARY_ACTION_INCLUDE:
            case INCLUDE_SUMMARY_ACTION_INCLUDE_NEXT:
                if (!sawIfndef || !sawDefine || sawEndif) {
                    return false;
                }
                sawInclude = true;
                break;
            case INCLUDE_SUMMARY_ACTION_RAW_RANGE:
                if (!sawIfndef || !sawDefine || sawEndif) {
                    return false;
                }
                if (!raw_range_starts_with_typedef(buffer, action)) {
                    return false;
                }
                sawRawTail = true;
                break;
            case INCLUDE_SUMMARY_ACTION_ENDIF:
                if (!sawIfndef || !sawDefine || sawEndif || !sawInclude) {
                    return false;
                }
                if (i + 1 != actionCount) {
                    return false;
                }
                sawEndif = true;
                break;
            default:
                return false;
        }
    }

    return sawIfndef && sawDefine && sawInclude && sawEndif && sawRawTail;
}

static bool classify_include_summary_include_define_scaffold(Preprocessor* pp,
                                                             const TokenBuffer* buffer,
                                                             const IncludeSummaryAction* actions,
                                                             size_t actionCount,
                                                             const char* guard,
                                                             IncludeSummaryProbe* probe) {
    if (!pp || !buffer || !buffer->tokens || !actions || actionCount == 0 || !guard || !guard[0] || !probe) {
        return false;
    }

    bool sawIfndef = false;
    bool sawGuardDefine = false;
    bool sawEndif = false;
    bool sawBodyAction = false;

    for (size_t i = 0; i < actionCount; ++i) {
        const IncludeSummaryAction* action = &actions[i];
        switch (action->kind) {
            case INCLUDE_SUMMARY_ACTION_PRAGMA:
                if (!pragma_is_once_directive(buffer->tokens, buffer->count, action->start)) {
                    return false;
                }
                probe->routerHasPragmaOnce = true;
                break;
            case INCLUDE_SUMMARY_ACTION_IFNDEF:
                if (sawIfndef || sawGuardDefine || sawBodyAction || sawEndif) {
                    return false;
                }
                if (!directive_line_matches_single_identifier(buffer->tokens, buffer->count, action->start, guard)) {
                    return false;
                }
                sawIfndef = true;
                break;
            case INCLUDE_SUMMARY_ACTION_DEFINE:
                if (!sawIfndef || sawEndif) {
                    return false;
                }
                if (!sawGuardDefine) {
                    if (!directive_line_matches_empty_guard_define(buffer->tokens,
                                                                   buffer->count,
                                                                   action->start,
                                                                   guard)) {
                        return false;
                    }
                    sawGuardDefine = true;
                    break;
                }
                sawBodyAction = true;
                break;
            case INCLUDE_SUMMARY_ACTION_INCLUDE:
            case INCLUDE_SUMMARY_ACTION_INCLUDE_NEXT:
                if (!sawIfndef || !sawGuardDefine || sawEndif) {
                    return false;
                }
                sawBodyAction = true;
                break;
            case INCLUDE_SUMMARY_ACTION_RAW_RANGE:
                if (!sawIfndef || !sawGuardDefine || sawEndif) {
                    return false;
                }
                if (!pp_summary_raw_range_can_clone_direct(pp,
                                                           buffer->tokens,
                                                           buffer->count,
                                                           action->start,
                                                           action->end)) {
                    return false;
                }
                sawBodyAction = true;
                break;
            case INCLUDE_SUMMARY_ACTION_ENDIF:
                if (!sawIfndef || !sawGuardDefine || sawEndif || !sawBodyAction) {
                    return false;
                }
                if (i + 1 != actionCount) {
                    return false;
                }
                sawEndif = true;
                break;
            default:
                return false;
        }
    }

    return sawIfndef && sawGuardDefine && sawBodyAction && sawEndif;
}

static bool classify_include_summary_conditional_scaffold(const IncludeSummaryAction* actions,
                                                          size_t actionCount) {
    if (!actions || actionCount == 0) {
        return false;
    }

    bool sawConditionalBody = false;
    for (size_t i = 0; i < actionCount; ++i) {
        switch (actions[i].kind) {
            case INCLUDE_SUMMARY_ACTION_RAW_RANGE:
            case INCLUDE_SUMMARY_ACTION_DEFINE:
            case INCLUDE_SUMMARY_ACTION_INCLUDE:
            case INCLUDE_SUMMARY_ACTION_INCLUDE_NEXT:
            case INCLUDE_SUMMARY_ACTION_IF:
            case INCLUDE_SUMMARY_ACTION_IFDEF:
            case INCLUDE_SUMMARY_ACTION_IFNDEF:
            case INCLUDE_SUMMARY_ACTION_ELIF:
            case INCLUDE_SUMMARY_ACTION_ELSE:
            case INCLUDE_SUMMARY_ACTION_ENDIF:
            case INCLUDE_SUMMARY_ACTION_PRAGMA:
                if (actions[i].kind == INCLUDE_SUMMARY_ACTION_IF ||
                    actions[i].kind == INCLUDE_SUMMARY_ACTION_IFDEF ||
                    actions[i].kind == INCLUDE_SUMMARY_ACTION_ELIF ||
                    actions[i].kind == INCLUDE_SUMMARY_ACTION_ELSE) {
                    sawConditionalBody = true;
                }
                break;
            default:
                return false;
        }
    }

    return sawConditionalBody;
}

IncludeHeaderBehaviorClass classify_include_summary_behavior_class(Preprocessor* pp,
                                                                  const TokenBuffer* buffer,
                                                                  const IncludeSummaryAction* actions,
                                                                  size_t actionCount,
                                                                  const char* guard,
                                                                  IncludeSummaryProbe* probe) {
    if (!probe || probe->status != INCLUDE_SUMMARY_PROBE_CANDIDATE) {
        return INCLUDE_HEADER_BEHAVIOR_UNKNOWN;
    }
    if (probe->routerOnly) {
        return INCLUDE_HEADER_BEHAVIOR_ROUTER_ONLY;
    }
    if (probe->routerRawTail) {
        return INCLUDE_HEADER_BEHAVIOR_ROUTER_RAW_TAIL;
    }
    if (classify_include_summary_include_define_scaffold(pp, buffer, actions, actionCount, guard, probe)) {
        return INCLUDE_HEADER_BEHAVIOR_INCLUDE_DEFINE_SCAFFOLD;
    }
    if (classify_include_summary_conditional_scaffold(actions, actionCount)) {
        return INCLUDE_HEADER_BEHAVIOR_CONDITIONAL_SCAFFOLD;
    }
    return INCLUDE_HEADER_BEHAVIOR_GENERAL_FALLBACK;
}

IncludeSummaryProbe analyze_include_summary_probe(const TokenBuffer* buffer,
                                                  IncludeSummaryAction** actionsOut,
                                                  size_t* actionCountOut) {
    IncludeSummaryProbe probe = {
        .status = INCLUDE_SUMMARY_PROBE_CANDIDATE
    };
    IncludeSummaryAction* actions = NULL;
    size_t actionCount = 0;
    size_t actionCapacity = 0;
    size_t rawStart = 0;
    size_t eofIndex = buffer ? buffer->count : 0;

    if (actionsOut) *actionsOut = NULL;
    if (actionCountOut) *actionCountOut = 0;
    if (!buffer || !buffer->tokens) {
        probe.status = INCLUDE_SUMMARY_PROBE_REJECT_UNSUPPORTED_DIRECTIVE;
        return probe;
    }

    for (size_t i = 0; i < buffer->count; ++i) {
        if (buffer->tokens[i].type == TOKEN_EOF) {
            eofIndex = i;
            break;
        }
    }

    for (size_t i = 0; i < eofIndex; ++i) {
        const Token* tok = &buffer->tokens[i];
        IncludeSummaryActionKind actionKind;

        switch (tok->type) {
            case TOKEN_INCLUDE:
            case TOKEN_INCLUDE_NEXT: {
                size_t operandCursor = i;
                char* includeName = NULL;
                bool isSystem = false;
                probe.directiveCount++;
                probe.includeCount++;
                actionKind = tok->type == TOKEN_INCLUDE
                    ? INCLUDE_SUMMARY_ACTION_INCLUDE
                    : INCLUDE_SUMMARY_ACTION_INCLUDE_NEXT;
                if (!parse_include_operand(buffer->tokens,
                                           buffer->count,
                                           &operandCursor,
                                           &includeName,
                                           &isSystem)) {
                    probe.status = INCLUDE_SUMMARY_PROBE_REJECT_NON_LITERAL_INCLUDE;
                    free(actions);
                    return probe;
                }
                if (rawStart < i &&
                    !include_summary_action_append(&actions,
                                                   &actionCount,
                                                   &actionCapacity,
                                                   (IncludeSummaryAction){
                                                       .kind = INCLUDE_SUMMARY_ACTION_RAW_RANGE,
                                                       .start = rawStart,
                                                       .end = i
                                                   })) {
                    probe.status = INCLUDE_SUMMARY_PROBE_UNKNOWN;
                    free(includeName);
                    free(actions);
                    return probe;
                }
                if (!include_summary_action_append(&actions,
                                                   &actionCount,
                                                   &actionCapacity,
                                                   (IncludeSummaryAction){
                                                       .kind = actionKind,
                                                       .start = i,
                                                       .end = operandCursor + 1
                                                   })) {
                    probe.status = INCLUDE_SUMMARY_PROBE_UNKNOWN;
                    free(includeName);
                    free(actions);
                    return probe;
                }
                free(includeName);
                i = operandCursor;
                rawStart = operandCursor + 1;
                break;
            }
            case TOKEN_DEFINE:
                probe.directiveCount++;
                probe.defineCount++;
                actionKind = INCLUDE_SUMMARY_ACTION_DEFINE;
                goto append_directive_action;
            case TOKEN_PP_IF:
                actionKind = INCLUDE_SUMMARY_ACTION_IF;
                goto append_conditional_action;
            case TOKEN_IFDEF:
                actionKind = INCLUDE_SUMMARY_ACTION_IFDEF;
                goto append_conditional_action;
            case TOKEN_IFNDEF:
                actionKind = INCLUDE_SUMMARY_ACTION_IFNDEF;
                goto append_conditional_action;
            case TOKEN_PP_ELIF:
                actionKind = INCLUDE_SUMMARY_ACTION_ELIF;
                goto append_conditional_action;
            case TOKEN_PP_ELSE:
                actionKind = INCLUDE_SUMMARY_ACTION_ELSE;
                goto append_conditional_action;
            case TOKEN_ENDIF:
                actionKind = INCLUDE_SUMMARY_ACTION_ENDIF;
append_conditional_action:
                probe.directiveCount++;
                probe.conditionalCount++;
                goto append_directive_action;
            case TOKEN_PRAGMA:
                if (!pragma_is_once_directive(buffer->tokens, buffer->count, i)) {
                    probe.status = INCLUDE_SUMMARY_PROBE_REJECT_UNSUPPORTED_DIRECTIVE;
                    free(actions);
                    return probe;
                }
                probe.directiveCount++;
                actionKind = INCLUDE_SUMMARY_ACTION_PRAGMA;
append_directive_action: {
                size_t lineEnd = skip_directive_line_cursor(buffer->tokens, eofIndex, i);
                if (rawStart < i &&
                    !include_summary_action_append(&actions,
                                                   &actionCount,
                                                   &actionCapacity,
                                                   (IncludeSummaryAction){
                                                       .kind = INCLUDE_SUMMARY_ACTION_RAW_RANGE,
                                                       .start = rawStart,
                                                       .end = i
                                                   })) {
                    probe.status = INCLUDE_SUMMARY_PROBE_UNKNOWN;
                    free(actions);
                    return probe;
                }
                if (!include_summary_action_append(&actions,
                                                   &actionCount,
                                                   &actionCapacity,
                                                   (IncludeSummaryAction){
                                                       .kind = actionKind,
                                                       .start = i,
                                                       .end = lineEnd
                                                   })) {
                    probe.status = INCLUDE_SUMMARY_PROBE_UNKNOWN;
                    free(actions);
                    return probe;
                }
                i = (lineEnd > 0) ? (lineEnd - 1) : i;
                rawStart = lineEnd;
                break;
            }
            case TOKEN_UNDEF:
            case TOKEN_PREPROCESSOR_OTHER:
                probe.status = INCLUDE_SUMMARY_PROBE_REJECT_UNSUPPORTED_DIRECTIVE;
                free(actions);
                return probe;
            default:
                break;
        }
    }

    if (probe.status == INCLUDE_SUMMARY_PROBE_CANDIDATE && rawStart < eofIndex) {
        if (!include_summary_action_append(&actions,
                                           &actionCount,
                                           &actionCapacity,
                                           (IncludeSummaryAction){
                                               .kind = INCLUDE_SUMMARY_ACTION_RAW_RANGE,
                                               .start = rawStart,
                                               .end = eofIndex
                                           })) {
            probe.status = INCLUDE_SUMMARY_PROBE_UNKNOWN;
            free(actions);
            return probe;
        }
    }

    if (probe.status == INCLUDE_SUMMARY_PROBE_CANDIDATE) {
        if (actionsOut) *actionsOut = actions;
        if (actionCountOut) *actionCountOut = actionCount;
    } else {
        free(actions);
    }

    return probe;
}

void pp_profile_event(const char* name) {
    if (!name) return;
    ProfilerScope scope = profiler_begin(name);
    profiler_end(scope);
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

static bool summary_probe_has_guard_wrapper(const TokenBuffer* buffer,
                                            const IncludeSummaryAction* actions,
                                            size_t actionCount,
                                            const char* guard) {
    if (!buffer || !buffer->tokens || !actions || actionCount < 3 || !guard || !guard[0]) {
        return false;
    }

    size_t index = 0;
    while (index < actionCount && actions[index].kind == INCLUDE_SUMMARY_ACTION_PRAGMA) {
        if (!pragma_is_once_directive(buffer->tokens, buffer->count, actions[index].start)) {
            return false;
        }
        index++;
    }

    if (index + 2 >= actionCount) {
        return false;
    }
    if (actions[index].kind != INCLUDE_SUMMARY_ACTION_IFNDEF ||
        !directive_line_matches_single_identifier(buffer->tokens, buffer->count, actions[index].start, guard)) {
        return false;
    }
    index++;
    if (actions[index].kind != INCLUDE_SUMMARY_ACTION_DEFINE ||
        !directive_line_matches_empty_guard_define(buffer->tokens, buffer->count, actions[index].start, guard)) {
        return false;
    }
    return actions[actionCount - 1].kind == INCLUDE_SUMMARY_ACTION_ENDIF;
}

void pp_profile_conditional_scaffold_shape(Preprocessor* pp,
                                           const TokenBuffer* buffer,
                                           const IncludeSummaryProbe* probe,
                                           const IncludeSummaryAction* actions,
                                           size_t actionCount,
                                           const char* guard) {
    (void)pp;
    if (!probe ||
        probe->status != INCLUDE_SUMMARY_PROBE_CANDIDATE ||
        probe->behaviorClass != INCLUDE_HEADER_BEHAVIOR_CONDITIONAL_SCAFFOLD ||
        !actions ||
        actionCount == 0) {
        return;
    }

    bool hasExprIf = false;
    bool hasElif = false;
    bool hasElse = false;
    bool hasRawRange = false;
    bool hasBodyDefine = false;
    bool hasBodyInclude = false;
    bool guardWrapped = summary_probe_has_guard_wrapper(buffer, actions, actionCount, guard);
    size_t includeActions = 0;
    size_t defineActions = 0;
    size_t conditionalActions = 0;

    for (size_t i = 0; i < actionCount; ++i) {
        switch (actions[i].kind) {
            case INCLUDE_SUMMARY_ACTION_INCLUDE:
            case INCLUDE_SUMMARY_ACTION_INCLUDE_NEXT:
                hasBodyInclude = true;
                includeActions++;
                break;
            case INCLUDE_SUMMARY_ACTION_DEFINE:
                defineActions++;
                if (!(guardWrapped &&
                      i > 0 &&
                      actions[i - 1].kind == INCLUDE_SUMMARY_ACTION_IFNDEF &&
                      directive_line_matches_empty_guard_define(buffer->tokens,
                                                                buffer->count,
                                                                actions[i].start,
                                                                guard))) {
                    hasBodyDefine = true;
                }
                break;
            case INCLUDE_SUMMARY_ACTION_RAW_RANGE:
                hasRawRange = true;
                break;
            case INCLUDE_SUMMARY_ACTION_IF:
                hasExprIf = true;
                conditionalActions++;
                break;
            case INCLUDE_SUMMARY_ACTION_IFDEF:
            case INCLUDE_SUMMARY_ACTION_IFNDEF:
            case INCLUDE_SUMMARY_ACTION_ENDIF:
                conditionalActions++;
                break;
            case INCLUDE_SUMMARY_ACTION_ELIF:
                hasExprIf = true;
                hasElif = true;
                conditionalActions++;
                break;
            case INCLUDE_SUMMARY_ACTION_ELSE:
                hasElse = true;
                conditionalActions++;
                break;
            case INCLUDE_SUMMARY_ACTION_PRAGMA:
                break;
        }
    }

    profiler_record_value("pp_conditional_scaffold_headers", 1);
    if (guardWrapped) profiler_record_value("pp_conditional_scaffold_guard_wrapped", 1);
    if (hasExprIf) profiler_record_value("pp_conditional_scaffold_has_expr_if", 1);
    if (hasElif) profiler_record_value("pp_conditional_scaffold_has_elif", 1);
    if (hasElse) profiler_record_value("pp_conditional_scaffold_has_else", 1);
    if (hasRawRange) profiler_record_value("pp_conditional_scaffold_has_raw_range", 1);
    if (hasBodyDefine) profiler_record_value("pp_conditional_scaffold_has_body_define", 1);
    if (hasBodyInclude) profiler_record_value("pp_conditional_scaffold_has_body_include", 1);
    profiler_record_value("pp_conditional_scaffold_include_actions_total", (long long)includeActions);
    profiler_record_value("pp_conditional_scaffold_define_actions_total", (long long)defineActions);
    profiler_record_value("pp_conditional_scaffold_conditional_actions_total", (long long)conditionalActions);

    if (guardWrapped &&
        hasBodyInclude &&
        !hasExprIf &&
        !hasElif &&
        !hasRawRange) {
        profiler_record_value("pp_conditional_scaffold_simple_ifdef_candidate", 1);
        profiler_record_value("pp_conditional_scaffold_simple_ifdef_candidate_include_actions_total",
                              (long long)includeActions);
        profiler_record_value("pp_conditional_scaffold_simple_ifdef_candidate_define_actions_total",
                              (long long)defineActions);
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
