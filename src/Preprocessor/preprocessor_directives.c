// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/pp_internal.h"
#include "Preprocessor/preprocessor_external.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "Lexer/token_buffer.h"
#include "Lexer/tokens.h"
#include "Utils/profiler.h"

typedef struct {
    char** names;
    size_t count;
    size_t capacity;
    bool variadic;
    bool hasVaOpt;
} MacroParamParse;

static void macro_param_parse_destroy(MacroParamParse* params) {
    if (!params) return;
    for (size_t i = 0; i < params->count; ++i) {
        free(params->names[i]);
    }
    free(params->names);
    params->names = NULL;
    params->count = 0;
    params->capacity = 0;
    params->variadic = false;
    params->hasVaOpt = false;
}

static bool macro_param_append(MacroParamParse* params, const char* name) {
    if (!params) return false;
    if (params->count == params->capacity) {
        size_t newCapacity = params->capacity ? params->capacity * 2 : 4;
        char** names = (char**)realloc(params->names, newCapacity * sizeof(char*));
        if (!names) return false;
        params->names = names;
        params->capacity = newCapacity;
    }
    params->names[params->count] = pp_strdup_local(name);
    if (!params->names[params->count]) {
        return false;
    }
    params->count++;
    return true;
}

static bool tokens_adjacent(const Token* left, const Token* right) {
    if (!left || !right) return false;
    if (left->location.end.line != right->location.start.line) return false;
    return right->location.start.column <= left->location.end.column;
}

static bool token_is_macro_identifier(const Token* tok) {
    if (!tok || !tok->value) return false;
    if (tok->type == TOKEN_IDENTIFIER) return true;
    return tok->type < TOKEN_IDENTIFIER; // keyword tokens remain valid macro names in PP context
}

static bool parse_macro_parameters(const Token* tokens,
                                   size_t count,
                                   size_t* cursor,
                                   MacroParamParse* params) {
    size_t i = *cursor;
    if (i >= count || tokens[i].type != TOKEN_LPAREN) {
        return false;
    }
    i++; // consume '('
    if (i < count && tokens[i].type == TOKEN_RPAREN) {
        i++; // consume ')'
        *cursor = i;
        return true;
    }
    bool expectParam = true;
    bool variadicDetected = false;

    while (i < count) {
        const Token* tok = &tokens[i];
        if (tok->type == TOKEN_RPAREN) {
            i++;
            break;
        }
        if (!expectParam) {
            if (tok->type != TOKEN_COMMA) {
                return false;
            }
            expectParam = true;
            i++;
            continue;
        }
        if (tok->type == TOKEN_IDENTIFIER) {
            if (!macro_param_append(params, tok->value)) {
                return false;
            }
            expectParam = false;
            i++;
            continue;
        }
        if (tok->type == TOKEN_ELLIPSIS) {
            params->variadic = true;
            variadicDetected = true;
            expectParam = false;
            i++;
            continue;
        }
        return false;
    }

    if (i > count) {
        return false;
    }
    if (expectParam && !variadicDetected) {
        return false;
    }
    *cursor = i;
    return true;
}

static bool collect_macro_body(const Token* tokens,
                               size_t count,
                               size_t* cursor,
                               int directiveLine,
                               PPTokenBuffer* body,
                               Preprocessor* pp) {
    size_t i = *cursor;
    while (i < count) {
        const Token* tok = &tokens[i];
        if (tok->type == TOKEN_EOF) {
            break;
        }
        if (tok->line != directiveLine) {
            break;
        }
        if (!pp_token_buffer_append_clone_remap(body, pp, tok)) {
            return false;
        }
        i++;
    }
    *cursor = i;
    return true;
}

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

static bool parse_include_operand(const Token* tokens,
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

static bool include_name_is_suspicious(const char* name) {
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

static const char* detect_include_guard(const TokenBuffer* buffer) {
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

static bool classify_include_summary_router_only(const TokenBuffer* buffer,
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

static bool classify_include_summary_router_raw_tail(const TokenBuffer* buffer,
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

static IncludeHeaderBehaviorClass classify_include_summary_behavior_class(Preprocessor* pp,
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

static IncludeSummaryProbe analyze_include_summary_probe(const TokenBuffer* buffer,
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

static void pp_profile_event(const char* name) {
    if (!name) return;
    ProfilerScope scope = profiler_begin(name);
    profiler_end(scope);
}

static void pp_profile_summary_probe(const IncludeSummaryProbe* probe) {
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

static void pp_profile_behavior_class(const IncludeSummaryProbe* probe) {
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

static void pp_profile_conditional_scaffold_shape(Preprocessor* pp,
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

static void pp_profile_summary_probe_scan_result(const IncludeSummaryProbe* probe) {
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

static bool pp_summary_replay_experiment_enabled(void) {
    static int initialized = 0;
    static bool enabled = false;
    if (!initialized) {
        const char* env = getenv("FISICS_PP_SUMMARY_REPLAY_EXPERIMENT");
        enabled = (env && env[0] && env[0] != '0');
        initialized = 1;
    }
    return enabled;
}

static const char* nested_recurse_scope_name(bool repeatSeen, IncludeSearchOrigin origin) {
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

bool process_define(Preprocessor* pp,
                    const Token* tokens,
                    size_t count,
                    size_t* cursor) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    i++;
    if (i >= count || tokens[i].line != directiveLine || !token_is_macro_identifier(&tokens[i])) {
        DiagKind kind = pp && pp->lenientMissingIncludes ? DIAG_WARNING : DIAG_ERROR;
        pp_report_diag(pp, tokens ? &tokens[i] : NULL, kind, CDIAG_PREPROCESSOR_GENERIC, "expected identifier after #define");
        if (pp && pp->lenientMissingIncludes) {
            skip_to_line_end(tokens, count, &i);
            *cursor = (i == 0) ? 0 : i - 1;
            return true;
        }
        return false;
    }
    const Token* nameTok = &tokens[i];
    i++;

    bool isFunction = false;
    MacroParamParse params = {0};
    PPTokenBuffer body = {0};
    bool ok = false;

    if (i < count &&
        tokens[i].type == TOKEN_LPAREN &&
        tokens[i].line == nameTok->line &&
        tokens_adjacent(nameTok, &tokens[i])) {
        isFunction = true;
        if (!parse_macro_parameters(tokens, count, &i, &params)) {
            DiagKind kind = pp->lenientMissingIncludes ? DIAG_WARNING : DIAG_ERROR;
            pp_report_diag(pp, nameTok, kind, CDIAG_PREPROCESSOR_GENERIC, "invalid parameter list in #define %s", nameTok->value ? nameTok->value : "");
            if (pp->lenientMissingIncludes) {
                while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
                    i++;
                }
                *cursor = (i == 0) ? 0 : i - 1;
                macro_param_parse_destroy(&params);
                pp_token_buffer_reset(&body);
                return true;
            }
            goto cleanup;
        }
    }

    if (!collect_macro_body(tokens, count, &i, directiveLine, &body, pp)) {
        DiagKind kind = pp->lenientMissingIncludes ? DIAG_WARNING : DIAG_ERROR;
        pp_report_diag(pp, nameTok, kind, CDIAG_PREPROCESSOR_GENERIC, "failed to collect macro body for %s", nameTok->value ? nameTok->value : "");
        if (pp->lenientMissingIncludes) {
            skip_to_line_end(tokens, count, &i);
            *cursor = (i == 0) ? 0 : i - 1;
            macro_param_parse_destroy(&params);
            pp_token_buffer_reset(&body);
            return true;
        }
        goto cleanup;
    }

    if (isFunction && params.variadic) {
        for (size_t b = 0; b < body.count; ++b) {
            const Token* btok = &body.tokens[b];
            if (btok->type == TOKEN_IDENTIFIER &&
                btok->value &&
                strcmp(btok->value, "__VA_OPT__") == 0) {
                params.hasVaOpt = true;
                break;
            }
        }
    }

    if (isFunction) {
        ok = macro_table_define_function(pp->table,
                                         nameTok->value,
                                         (const char* const*)params.names,
                                         params.count,
                                         params.variadic,
                                         params.hasVaOpt,
                                         body.tokens,
                                         body.count,
                                         tokens[*cursor].location);
    } else {
        ok = macro_table_define_object(pp->table,
                                       nameTok->value,
                                       body.tokens,
                                       body.count,
                                       tokens[*cursor].location);
    }

    if (!ok) {
        DiagKind kind = pp->lenientMissingIncludes ? DIAG_WARNING : DIAG_ERROR;
        pp_report_diag(pp, nameTok, kind, CDIAG_PREPROCESSOR_GENERIC, "failed to record macro %s", nameTok->value ? nameTok->value : "");
        if (pp->lenientMissingIncludes) {
            skip_to_line_end(tokens, count, &i);
            *cursor = (i == 0) ? 0 : i - 1;
            macro_param_parse_destroy(&params);
            pp_token_buffer_reset(&body);
            return true;
        }
        goto cleanup;
    }

    *cursor = (i == 0) ? 0 : i - 1;
cleanup:
    macro_param_parse_destroy(&params);
    pp_token_buffer_reset(&body);
    return ok;
}

bool process_undef(Preprocessor* pp,
                   const Token* tokens,
                   size_t count,
                   size_t* cursor) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    i++;
    if (i >= count || tokens[i].line != directiveLine || !token_is_macro_identifier(&tokens[i])) {
        pp_report_diag(pp, tokens ? &tokens[i] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "expected identifier after #undef");
        return false;
    }
    macro_table_undef(pp->table, tokens[i].value);
    i++;
    if (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
        pp_report_diag(pp,
                       &tokens[i],
                       DIAG_ERROR,
                       CDIAG_PREPROCESSOR_GENERIC,
                       "unexpected tokens after #undef directive");
        return false;
    }
    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) i++;
    *cursor = (i == 0) ? 0 : i - 1;
    return true;
}

bool process_include(Preprocessor* pp,
                     const Token* tokens,
                     size_t count,
                     size_t* cursor,
                     PPTokenBuffer* output,
                     bool isIncludeNext) {
    if (!pp || !pp->resolver) {
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "include resolver not initialized");
        return false;
    }
    char* name = NULL;
    bool isSystem = false;
    size_t lineStart = *cursor;
    int directiveLine = tokens && count > 0 ? tokens[lineStart].line : 0;
    size_t lineEnd = lineStart + 1;
    while (lineEnd < count && tokens[lineEnd].type != TOKEN_EOF && tokens[lineEnd].line == directiveLine) {
        lineEnd++;
    }

    size_t tempCursor = lineStart;
    if (parse_include_operand(tokens, count, &tempCursor, &name, &isSystem)) {
        pp_profile_event("pp_include_operand_direct");
        *cursor = tempCursor;
    } else {
        ProfilerScope operandExpandScope = profiler_begin("pp_include_operand_expand_path");
        PPTokenBuffer expanded = {0};
        size_t span = (lineEnd > lineStart + 1) ? (lineEnd - (lineStart + 1)) : 0;
        profiler_record_value("pp_count_macro_expand_calls", 1);
        profiler_record_value("pp_count_macro_expand_input_tokens", span);
        if (!macro_expander_expand(&pp->expander, tokens + lineStart + 1, span, &expanded)) {
            profiler_end(operandExpandScope);
            pp_token_buffer_destroy(&expanded);
            pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "failed to expand #include operand");
            return false;
        }

        size_t tempCount = expanded.count + 2;
        Token* tempTokens = (Token*)calloc(tempCount, sizeof(Token));
        if (!tempTokens) {
            profiler_end(operandExpandScope);
            pp_token_buffer_destroy(&expanded);
            pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "out of memory parsing #include");
            return false;
        }
        tempTokens[0] = tokens[*cursor];
        for (size_t i = 0; i < expanded.count; ++i) {
            tempTokens[i + 1] = expanded.tokens[i];
            tempTokens[i + 1].line = directiveLine;
            tempTokens[i + 1].location.start.line = directiveLine;
            tempTokens[i + 1].location.end.line = directiveLine;
        }
        tempTokens[tempCount - 1].type = TOKEN_EOF;
        tempTokens[tempCount - 1].line = directiveLine;

        tempCursor = 0;
        if (!parse_include_operand(tempTokens, tempCount, &tempCursor, &name, &isSystem)) {
            profiler_end(operandExpandScope);
            free(tempTokens);
            pp_token_buffer_destroy(&expanded);
            pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "invalid #include operand");
            return false;
        }
        free(tempTokens);
        pp_token_buffer_destroy(&expanded);
        profiler_end(operandExpandScope);
        *cursor = (lineEnd == 0) ? 0 : lineEnd - 1;
    }

    const char* parentFile = token_file(&tokens[*cursor]);
    IncludeSearchOrigin origin = INCLUDE_SEARCH_RAW;
    size_t originIndex = (size_t)-1;
    ProfilerScope resolveScope = profiler_begin("pp_include_resolve");
    const IncludeFile* incPtr = include_resolver_load(pp->resolver,
                                                      parentFile,
                                                      name,
                                                      isSystem,
                                                      isIncludeNext,
                                                      &origin,
                                                      &originIndex);
    profiler_end(resolveScope);
    IncludeFile incValue;
    bool haveInc = false;
    if (incPtr) {
        incValue = *incPtr;
        incPtr = NULL;
        haveInc = true;
    }
    IncludeSummaryProbe summaryProbe = {0};
    const IncludeSummaryAction* summaryActions = NULL;
    size_t summaryActionCount = 0;
    if (haveInc) {
        summaryProbe = incValue.summaryProbe;
        summaryActions = incValue.summaryActions;
        summaryActionCount = incValue.summaryActionCount;
    }

    FisicsInclude incRec = {0};
    incRec.name = name;
    incRec.kind = isSystem ? FISICS_INCLUDE_SYSTEM : FISICS_INCLUDE_LOCAL;
    incRec.line = tokens ? tokens[*cursor].line : 0;
    incRec.column = tokens ? tokens[*cursor].location.start.column : 0;
    incRec.resolved = haveInc;
    incRec.resolved_path = haveInc ? incValue.path : NULL;
    if (!haveInc) {
        incRec.origin = FISICS_INCLUDE_UNRESOLVED;
    } else {
        switch (origin) {
            case INCLUDE_SEARCH_SAME_DIR:
                incRec.origin = FISICS_INCLUDE_PROJECT;
                break;
            case INCLUDE_SEARCH_INCLUDE_PATH:
                incRec.origin = isSystem ? FISICS_INCLUDE_SYSTEM_ORIGIN : FISICS_INCLUDE_PROJECT;
                break;
            case INCLUDE_SEARCH_RAW:
            default:
                incRec.origin = isSystem ? FISICS_INCLUDE_SYSTEM_ORIGIN : FISICS_INCLUDE_EXTERNAL;
                break;
        }
    }
    if (pp->ctx) {
        cc_append_include(pp->ctx, &incRec);
    }
    if (!haveInc) {
        bool warnOnly = isSystem || pp->lenientMissingIncludes;
        bool suspiciousSystemName = isSystem && include_name_is_suspicious(name);
        if (warnOnly) {
            if (suspiciousSystemName) {
                // Some SDK macro-expanded include operands are non-header expressions
                // (e.g. "(*__error()).h"); ignore in lenient/system mode.
                free(name);
                return true;
            }
            pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_WARNING, CDIAG_PREPROCESSOR_GENERIC,
                           "skipping missing %s include '%s'", isSystem ? "system" : "local", name ? name : "<null>");
            free(name);
            return true;
        }
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "could not resolve include '%s'", name ? name : "<null>");
        free(name);
        return false;
    }
    free(name);

    include_graph_add(&pp->includeGraph,
                      parentFile ? parentFile : "<unknown>",
                      incValue.path ? incValue.path : "<unknown>");

    bool nestedIncludeDispatch = pp->includeStack.depth > 1;
    bool wasIncludedBefore = include_resolver_was_included(pp->resolver, incValue.path);
    profiler_record_value(wasIncludedBefore
                              ? "pp_count_repeat_include_attempts"
                              : "pp_count_first_include_attempts",
                          1);
    if (nestedIncludeDispatch) {
        pp_profile_event(wasIncludedBefore
                             ? "pp_nested_include_repeat_seen"
                             : "pp_nested_include_first_seen");
    }

    if (pp->preprocessMode == PREPROCESS_HYBRID && isSystem) {
        const char* cmd = pp->externalPreprocessCmd;
        if (!cmd || !cmd[0]) {
            const char* env = getenv("FISICS_EXTERNAL_CPP");
            cmd = (env && env[0]) ? env : "clang";
        }
        const char* args = pp->externalPreprocessArgs;
        if (!args || !args[0]) {
            const char* env = getenv("FISICS_EXTERNAL_CPP_ARGS");
            args = (env && env[0]) ? env : NULL;
        }
        char* extBuf = NULL;
        size_t extLen = 0;
        ProfilerScope externalScope = profiler_begin("pp_include_external");
        bool externalOk = pp_run_external_preprocessor(cmd,
                                                       args,
                                                       incValue.path,
                                                       pp->includePaths,
                                                       pp->includePathCount,
                                                       &extBuf,
                                                       &extLen);
        profiler_end(externalScope);
        if (!externalOk) {
            bool warnOnly = pp->lenientMissingIncludes;
            if (warnOnly) {
                pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_WARNING, CDIAG_PREPROCESSOR_GENERIC,
                               "external preprocessing failed for system include '%s'", incValue.path ? incValue.path : "<unknown>");
                free(extBuf);
                return true;
            }
            pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                           "external preprocessing failed for system include '%s'", incValue.path ? incValue.path : "<unknown>");
            free(extBuf);
            return false;
        }

        ProfilerScope externalLexScope = profiler_begin("pp_include_external_lex");
        Lexer extLexer;
        initLexer(&extLexer, extBuf, incValue.path, pp->enableTrigraphs);
        TokenBuffer extTokens;
        token_buffer_init(&extTokens);
        bool lexOk = token_buffer_fill_from_lexer(&extTokens, &extLexer);
        destroyLexer(&extLexer);
        if (lexOk) {
            for (size_t i = 0; i < extTokens.count; ++i) {
                if (extTokens.tokens[i].type == TOKEN_EOF) break;
                if (!pp_token_buffer_append_clone(output, &extTokens.tokens[i])) {
                    lexOk = false;
                    break;
                }
            }
        }
        token_buffer_destroy(&extTokens);
        profiler_end(externalLexScope);
        free(extBuf);
        include_resolver_mark_included(pp->resolver, incValue.path);
        return lexOk;
    }

    if (incValue.pragmaOnce && wasIncludedBefore) {
        profiler_record_value("pp_count_include_short_circuit_pragma_once", 1);
        if (nestedIncludeDispatch) {
            pp_profile_event("pp_nested_include_repeat_pragma_once_skip");
        }
        return true;
    }

    const char* cachedGuard = include_resolver_get_cached_guard(pp->resolver, incValue.path);
    if (cachedGuard && macro_table_lookup(pp->table, cachedGuard) != NULL) {
        profiler_record_value("pp_count_include_short_circuit_cached_guard", 1);
        if (nestedIncludeDispatch) {
            pp_profile_event(wasIncludedBefore
                                 ? "pp_nested_include_repeat_cached_guard_skip"
                                 : "pp_nested_include_first_cached_guard_skip");
        }
        return true;
    }

    bool includeWouldRecurse = pp_include_stack_contains(pp, incValue.path);

    int savedOffset = pp->lineOffset;
    char* savedLogical = pp->logicalFile;
    bool savedRemap = pp->lineRemapActive;
    pp->lineOffset = 0;
    pp->lineRemapActive = false;
    pp_set_logical_file(pp, incValue.path);

    ProfilerScope lexScope = profiler_begin("pp_include_lex");
    Lexer lexer;
    initLexer(&lexer, incValue.contents, incValue.path, pp->enableTrigraphs);
    TokenBuffer buffer;
    token_buffer_init(&buffer);
    if (!token_buffer_fill_from_lexer(&buffer, &lexer)) {
        token_buffer_destroy(&buffer);
        destroyLexer(&lexer);
        profiler_end(lexScope);
        pp->lineOffset = savedOffset;
        pp->logicalFile = savedLogical;
        pp->lineRemapActive = savedRemap;
        return false;
    }
    destroyLexer(&lexer);
    profiler_end(lexScope);

    ProfilerScope guardScope = profiler_begin("pp_include_guard_check");
    const char* guard = detect_include_guard(&buffer);
    profiler_end(guardScope);
    if (guard) {
        profiler_record_value("pp_count_guarded_headers_detected", 1);
        include_resolver_cache_guard(pp->resolver, incValue.path, guard);
    }
    if (origin == INCLUDE_SEARCH_INCLUDE_PATH &&
        summaryProbe.status == INCLUDE_SUMMARY_PROBE_UNKNOWN) {
        IncludeSummaryAction* builtSummaryActions = NULL;
        size_t builtSummaryActionCount = 0;
        ProfilerScope summaryProbeScope = profiler_begin("pp_include_summary_probe_scan");
        summaryProbe = analyze_include_summary_probe(&buffer,
                                                    &builtSummaryActions,
                                                    &builtSummaryActionCount);
        if (summaryProbe.status == INCLUDE_SUMMARY_PROBE_CANDIDATE) {
            summaryProbe.routerOnly = classify_include_summary_router_only(&buffer,
                                                                           builtSummaryActions,
                                                                           builtSummaryActionCount,
                                                                           guard,
                                                                           &summaryProbe);
            if (!summaryProbe.routerOnly) {
                summaryProbe.routerRawTail = classify_include_summary_router_raw_tail(&buffer,
                                                                                      builtSummaryActions,
                                                                                      builtSummaryActionCount,
                                                                                      guard,
                                                                                      &summaryProbe);
            }
            summaryProbe.behaviorClass =
                classify_include_summary_behavior_class(pp,
                                                        &buffer,
                                                        builtSummaryActions,
                                                        builtSummaryActionCount,
                                                        guard,
                                                        &summaryProbe);
        }
        profiler_end(summaryProbeScope);
        include_resolver_cache_summary_probe(pp->resolver, incValue.path, summaryProbe);
        if (summaryProbe.status == INCLUDE_SUMMARY_PROBE_CANDIDATE) {
            include_resolver_cache_summary_actions(pp->resolver,
                                                   incValue.path,
                                                   builtSummaryActions,
                                                   builtSummaryActionCount);
        }
        free(builtSummaryActions);
        summaryActions = include_resolver_get_cached_summary_actions(pp->resolver,
                                                                     incValue.path,
                                                                     &summaryActionCount);
        pp_profile_summary_probe_scan_result(&summaryProbe);
    }
    if (origin == INCLUDE_SEARCH_INCLUDE_PATH) {
        pp_profile_summary_probe(&summaryProbe);
        pp_profile_behavior_class(&summaryProbe);
        if (summaryProbe.status == INCLUDE_SUMMARY_PROBE_CANDIDATE &&
            summaryProbe.behaviorClass == INCLUDE_HEADER_BEHAVIOR_CONDITIONAL_SCAFFOLD) {
            profiler_record_value("pp_conditional_scaffold_profile_entered", 1);
            profiler_record_value("pp_conditional_scaffold_action_count_seen", (long long)summaryActionCount);
            if (!summaryActions || summaryActionCount == 0) {
                profiler_record_value("pp_conditional_scaffold_missing_actions", 1);
            }
        }
        pp_profile_conditional_scaffold_shape(pp,
                                              &buffer,
                                              &summaryProbe,
                                              summaryActions,
                                              summaryActionCount,
                                              guard);
    }
    if (guard && macro_table_lookup(pp->table, guard) != NULL) {
        profiler_record_value("pp_count_include_short_circuit_guard_after_lex", 1);
        if (nestedIncludeDispatch) {
            pp_profile_event(wasIncludedBefore
                                 ? "pp_nested_include_repeat_guard_skip"
                                 : "pp_nested_include_first_guard_skip");
        }
        token_buffer_destroy(&buffer);
        include_resolver_mark_included(pp->resolver, incValue.path);
        pp->lineOffset = savedOffset;
        pp->logicalFile = savedLogical;
        pp->lineRemapActive = savedRemap;
        return true;
    }

    if (includeWouldRecurse) {
        if (nestedIncludeDispatch) {
            pp_profile_event("pp_nested_include_recursive_cycle");
        }
        token_buffer_destroy(&buffer);
        pp->lineOffset = savedOffset;
        pp->logicalFile = savedLogical;
        pp->lineRemapActive = savedRemap;
        if (pp->lenientMissingIncludes) {
            return true;
        }
        pp_report_diag(pp,
                       tokens ? &tokens[*cursor] : NULL,
                       DIAG_ERROR,
                       CDIAG_PREPROCESSOR_GENERIC,
                       "detected recursive include of '%s'",
                       incValue.path ? incValue.path : "<unknown>");
        return false;
    }

    bool pushedFrame = pp_include_stack_push(pp, incValue.path, origin, originIndex);

    ProfilerScope recurseScope = profiler_begin("pp_include_recurse");
    ProfilerScope nestedOriginScope = {0};
    if (nestedIncludeDispatch) {
        nestedOriginScope = profiler_begin(nested_recurse_scope_name(wasIncludedBefore, origin));
    }
    bool ok = false;
    if (origin == INCLUDE_SEARCH_INCLUDE_PATH &&
        pp_summary_replay_experiment_enabled() &&
        summaryProbe.status == INCLUDE_SUMMARY_PROBE_CANDIDATE &&
        (summaryProbe.behaviorClass == INCLUDE_HEADER_BEHAVIOR_ROUTER_ONLY ||
         summaryProbe.behaviorClass == INCLUDE_HEADER_BEHAVIOR_ROUTER_RAW_TAIL)) {
        PPSummaryReplayResult replayResult =
            preprocess_tokens_router_replay(pp,
                                            &buffer,
                                            &summaryProbe,
                                            summaryActions,
                                            summaryActionCount,
                                            output,
                                            false);
        if (replayResult == PP_SUMMARY_REPLAY_USED) {
            pp_profile_event("pp_include_router_replay_used");
            if (summaryProbe.routerRawTail) {
                pp_profile_event("pp_include_router_raw_tail_replay_used");
            }
            ok = true;
        } else if (replayResult == PP_SUMMARY_REPLAY_UNSUPPORTED) {
            pp_profile_event("pp_include_router_replay_fallback");
            ok = preprocess_tokens(pp, &buffer, output, false);
        } else {
            pp_profile_event("pp_include_router_replay_error");
            ok = false;
        }
    } else if (origin == INCLUDE_SEARCH_INCLUDE_PATH &&
               pp_summary_replay_experiment_enabled() &&
               summaryProbe.status == INCLUDE_SUMMARY_PROBE_CANDIDATE &&
               summaryProbe.behaviorClass == INCLUDE_HEADER_BEHAVIOR_INCLUDE_DEFINE_SCAFFOLD) {
        PPSummaryReplayResult replayResult =
            preprocess_tokens_scaffold_replay(pp,
                                              &buffer,
                                              &summaryProbe,
                                              summaryActions,
                                              summaryActionCount,
                                              output,
                                              false);
        if (replayResult == PP_SUMMARY_REPLAY_USED) {
            profiler_record_value("pp_include_behavior_class_scaffold_replay", 1);
            ok = true;
        } else if (replayResult == PP_SUMMARY_REPLAY_UNSUPPORTED) {
            profiler_record_value("pp_include_behavior_class_scaffold_fallback", 1);
            ok = preprocess_tokens(pp, &buffer, output, false);
        } else {
            pp_profile_event("pp_include_scaffold_replay_error");
            ok = false;
        }
    } else if (origin == INCLUDE_SEARCH_INCLUDE_PATH &&
        summaryProbe.status == INCLUDE_SUMMARY_PROBE_CANDIDATE &&
        pp_summary_replay_experiment_enabled()) {
        PPSummaryReplayResult replayResult =
            preprocess_tokens_summary_replay(pp,
                                             &buffer,
                                             summaryActions,
                                             summaryActionCount,
                                             output,
                                             false);
        if (replayResult == PP_SUMMARY_REPLAY_USED) {
            pp_profile_event("pp_include_summary_replay_used");
            ok = true;
        } else if (replayResult == PP_SUMMARY_REPLAY_UNSUPPORTED) {
            pp_profile_event("pp_include_summary_replay_fallback");
            ok = preprocess_tokens(pp, &buffer, output, false);
        } else {
            pp_profile_event("pp_include_summary_replay_error");
            ok = false;
        }
    } else {
        ok = preprocess_tokens(pp, &buffer, output, false);
    }
    if (nestedIncludeDispatch) {
        profiler_end(nestedOriginScope);
    }
    profiler_end(recurseScope);
    if (!ok) {
        pp_debug_fail("process_include_body", tokens ? &tokens[*cursor] : NULL);
    }
    token_buffer_destroy(&buffer);
    pp->lineOffset = savedOffset;
    pp->logicalFile = savedLogical;
    pp->lineRemapActive = savedRemap;
    include_resolver_mark_included(pp->resolver, incValue.path);
    if (pushedFrame) pp_include_stack_pop(pp);
    return ok;
}

bool process_pragma(Preprocessor* pp,
                    const Token* tokens,
                    size_t count,
                    size_t* cursor) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    i++;
    if (i < count && tokens[i].type == TOKEN_ONCE) {
        profiler_record_value("pp_count_pragma_once_headers_detected", 1);
        const char* path = token_file(&tokens[i]);
        if (!path) path = token_file(&tokens[*cursor]);
        if (path && pp && pp->resolver) {
            include_resolver_mark_pragma_once(pp->resolver, path);
        }
    } else if (i < count && tokens[i].type == TOKEN_IDENTIFIER && tokens[i].value &&
               strcmp(tokens[i].value, "STDC") == 0) {
        const char* kind = NULL;
        if ((i + 1) < count &&
            tokens[i + 1].line == directiveLine &&
            tokens[i + 1].type == TOKEN_IDENTIFIER &&
            tokens[i + 1].value) {
            const char* val = tokens[i + 1].value;
            if (strcmp(val, "FP_CONTRACT") == 0 ||
                strcmp(val, "CX_LIMITED_RANGE") == 0 ||
                strcmp(val, "FENV_ACCESS") == 0) {
                kind = val;
            }
        }
        if (kind) {
            pp_report_diag(pp, &tokens[i], DIAG_WARNING, CDIAG_PREPROCESSOR_GENERIC,
                           "ignored #pragma STDC %s", kind);
        }
    }
    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == directiveLine) {
        i++;
    }
    *cursor = (i == 0) ? 0 : i - 1;
    return true;
}

bool process_line_directive(Preprocessor* pp,
                            const Token* tokens,
                            size_t count,
                            size_t* cursor) {
    size_t i = *cursor;
    int directiveLine = tokens[i].line;
    size_t lineStart = i;
    size_t lineEnd = lineStart + 1;
    while (lineEnd < count && tokens[lineEnd].type != TOKEN_EOF && tokens[lineEnd].line == directiveLine) {
        lineEnd++;
    }
    size_t span = (lineEnd > lineStart + 1) ? (lineEnd - (lineStart + 1)) : 0;

    PPTokenBuffer expanded = {0};
    profiler_record_value("pp_count_macro_expand_calls", 1);
    profiler_record_value("pp_count_macro_expand_input_tokens", span);
    if (!macro_expander_expand(&pp->expander, tokens + lineStart + 1, span, &expanded)) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                       "failed to expand #line directive");
        return false;
    }

    size_t cursorExp = 0;
    while (cursorExp < expanded.count && expanded.tokens[cursorExp].type == TOKEN_EOF) {
        cursorExp++;
    }
    if (cursorExp >= expanded.count || expanded.tokens[cursorExp].type != TOKEN_NUMBER) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                       "expected line number after #line");
        return false;
    }
    const char* lineText = expanded.tokens[cursorExp].value ? expanded.tokens[cursorExp].value : "";
    if (!lineText[0]) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                       "expected line number after #line");
        return false;
    }
    for (const unsigned char* p = (const unsigned char*)lineText; *p; ++p) {
        if (!isdigit(*p)) {
            pp_token_buffer_destroy(&expanded);
            pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                           "line number after #line must use decimal digits");
            return false;
        }
    }
    errno = 0;
    char* end = NULL;
    unsigned long parsedLine = strtoul(lineText, &end, 10);
    if (errno == ERANGE || !end || *end != '\0' || parsedLine > (unsigned long)INT_MAX) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                       "line number after #line is out of range");
        return false;
    }
    if (parsedLine == 0UL) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                       "line number after #line must be positive");
        return false;
    }
    long newLine = (long)parsedLine;
    cursorExp++;

    const char* newFile = NULL;
    while (cursorExp < expanded.count && expanded.tokens[cursorExp].type == TOKEN_EOF) {
        cursorExp++;
    }
    if (cursorExp < expanded.count && expanded.tokens[cursorExp].type == TOKEN_STRING) {
        newFile = expanded.tokens[cursorExp].value;
        cursorExp++;
    }
    while (cursorExp < expanded.count && expanded.tokens[cursorExp].type == TOKEN_EOF) {
        cursorExp++;
    }
    if (cursorExp < expanded.count) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC,
                       "unexpected tokens after #line directive");
        return false;
    }

    int nextPhysical = directiveLine + 1;
    pp->lineOffset = (int)newLine - nextPhysical;
    pp->lineRemapActive = true;
    if (newFile && newFile[0]) {
        pp_set_logical_file(pp, newFile);
    }

    pp_token_buffer_destroy(&expanded);

    *cursor = (lineEnd == 0) ? 0 : lineEnd - 1;
    return true;
}
