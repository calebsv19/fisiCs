#include "Preprocessor/pp_internal.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Lexer/token_buffer.h"
#include "Lexer/tokens.h"

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

static bool macro_body_uses_gnu_comma_variadic(const PPTokenBuffer* body) {
    if (!body || body->count < 3) return false;
    for (size_t i = 0; i + 2 < body->count; ++i) {
        const Token* comma = &body->tokens[i];
        const Token* hashhash = &body->tokens[i + 1];
        const Token* vaArgs = &body->tokens[i + 2];
        if (comma->type != TOKEN_COMMA) continue;
        if (hashhash->type != TOKEN_DOUBLE_HASH) continue;
        if (vaArgs->type != TOKEN_IDENTIFIER || !vaArgs->value) continue;
        if (strcmp(vaArgs->value, "__VA_ARGS__") == 0) {
            return true;
        }
    }
    return false;
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

static bool args_has_flag(const char* args, const char* flag) {
    if (!args || !flag) return false;
    const char* hit = strstr(args, flag);
    if (!hit) return false;
    if (hit != args && hit[-1] != ' ') return false;
    const char next = hit[strlen(flag)];
    return next == '\0' || next == ' ';
}

static char* shell_quote(const char* text) {
    if (!text) return NULL;
    size_t len = 2;
    for (const char* p = text; *p; ++p) {
        if (*p == '\'') {
            len += 4;
        } else {
            len += 1;
        }
    }
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    char* w = out;
    *w++ = '\'';
    for (const char* p = text; *p; ++p) {
        if (*p == '\'') {
            memcpy(w, "'\\''", 4);
            w += 4;
        } else {
            *w++ = *p;
        }
    }
    *w++ = '\'';
    *w = '\0';
    return out;
}

static bool cmd_append(char** buf, size_t* len, size_t* cap, const char* text) {
    if (!buf || !len || !cap || !text) return false;
    size_t add = strlen(text);
    size_t extra = add + ((*len > 0) ? 1 : 0);
    if (*len + extra + 1 > *cap) {
        size_t newCap = (*cap > 0) ? *cap : 128;
        while (newCap < *len + extra + 1) {
            newCap *= 2;
        }
        char* grown = (char*)realloc(*buf, newCap);
        if (!grown) return false;
        *buf = grown;
        *cap = newCap;
    }
    if (*len > 0) {
        (*buf)[(*len)++] = ' ';
    }
    memcpy(*buf + *len, text, add);
    *len += add;
    (*buf)[*len] = '\0';
    return true;
}

static char* build_external_preprocess_command(const char* cmd,
                                               const char* args,
                                               const char* inputPath,
                                               const char* const* includePaths,
                                               size_t includePathCount) {
    if (!cmd || !inputPath) return NULL;
    char* quotedCmd = shell_quote(cmd);
    if (!quotedCmd) return NULL;

    char* buffer = NULL;
    size_t len = 0;
    size_t cap = 0;
    bool ok = cmd_append(&buffer, &len, &cap, quotedCmd);
    free(quotedCmd);
    if (!ok) {
        free(buffer);
        return NULL;
    }

    if (args && args[0]) {
        if (!cmd_append(&buffer, &len, &cap, args)) {
            free(buffer);
            return NULL;
        }
    }

    if (!args_has_flag(args, "-E")) {
        if (!cmd_append(&buffer, &len, &cap, "-E")) {
            free(buffer);
            return NULL;
        }
    }
    if (!args_has_flag(args, "-P")) {
        if (!cmd_append(&buffer, &len, &cap, "-P")) {
            free(buffer);
            return NULL;
        }
    }

    for (size_t i = 0; i < includePathCount; ++i) {
        const char* path = includePaths[i];
        if (!path || !path[0]) continue;
        size_t argLen = strlen(path) + 3;
        char* includeArg = (char*)malloc(argLen);
        if (!includeArg) {
            free(buffer);
            return NULL;
        }
        snprintf(includeArg, argLen, "-I%s", path);
        char* quotedArg = shell_quote(includeArg);
        free(includeArg);
        if (!quotedArg) {
            free(buffer);
            return NULL;
        }
        ok = cmd_append(&buffer, &len, &cap, quotedArg);
        free(quotedArg);
        if (!ok) {
            free(buffer);
            return NULL;
        }
    }

    char* quotedInput = shell_quote(inputPath);
    if (!quotedInput) {
        free(buffer);
        return NULL;
    }
    ok = cmd_append(&buffer, &len, &cap, quotedInput);
    free(quotedInput);
    if (!ok) {
        free(buffer);
        return NULL;
    }
    return buffer;
}

static bool run_external_preprocessor(const char* cmd,
                                      const char* args,
                                      const char* inputPath,
                                      const char* const* includePaths,
                                      size_t includePathCount,
                                      char** outBuffer,
                                      size_t* outLength) {
    if (!outBuffer || !outLength) return false;
    *outBuffer = NULL;
    *outLength = 0;

    char* command = build_external_preprocess_command(cmd, args, inputPath, includePaths, includePathCount);
    if (!command) {
        return false;
    }

    FILE* fp = popen(command, "r");
    free(command);
    if (!fp) {
        return false;
    }

    size_t cap = 0;
    size_t len = 0;
    char* buffer = NULL;
    char chunk[4096];
    while (!feof(fp)) {
        size_t read = fread(chunk, 1, sizeof(chunk), fp);
        if (read == 0) {
            if (ferror(fp)) {
                break;
            }
            continue;
        }
        if (len + read + 1 > cap) {
            size_t newCap = cap ? cap * 2 : 8192;
            while (newCap < len + read + 1) {
                newCap *= 2;
            }
            char* grown = (char*)realloc(buffer, newCap);
            if (!grown) {
                free(buffer);
                pclose(fp);
                return false;
            }
            buffer = grown;
            cap = newCap;
        }
        memcpy(buffer + len, chunk, read);
        len += read;
    }
    if (buffer) {
        buffer[len] = '\0';
    }
    int status = pclose(fp);
    if (status != 0) {
        free(buffer);
        return false;
    }
    if (!buffer) {
        buffer = strdup("");
        if (!buffer) return false;
    }
    // Strip leftover preprocessor directives (e.g., #pragma) that would confuse the parser.
    {
        char* filtered = (char*)malloc(len + 1);
        if (filtered) {
            size_t w = 0;
            size_t i = 0;
            while (i < len) {
                size_t lineStart = i;
                size_t lineEnd = i;
                while (lineEnd < len && buffer[lineEnd] != '\n') {
                    lineEnd++;
                }
                size_t k = lineStart;
                while (k < lineEnd && (buffer[k] == ' ' || buffer[k] == '\t')) {
                    k++;
                }
                if (k < lineEnd && buffer[k] == '#') {
                    if (lineEnd < len) {
                        filtered[w++] = '\n';
                    }
                } else {
                    size_t lineLen = lineEnd - lineStart;
                    if (lineLen > 0) {
                        memcpy(filtered + w, buffer + lineStart, lineLen);
                        w += lineLen;
                    }
                    if (lineEnd < len) {
                        filtered[w++] = '\n';
                    }
                }
                i = (lineEnd < len) ? (lineEnd + 1) : lineEnd;
            }
            filtered[w] = '\0';
            free(buffer);
            buffer = filtered;
            len = w;
        }
    }
    *outBuffer = buffer;
    *outLength = len;
    return true;
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
        if (macro_body_uses_gnu_comma_variadic(&body)) {
            pp_report_diag(pp,
                           nameTok,
                           DIAG_ERROR,
                           CDIAG_PREPROCESSOR_GENERIC,
                           "GNU ', ##__VA_ARGS__' extension is not supported");
            goto cleanup;
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

    PPTokenBuffer expanded = {0};
    size_t span = (lineEnd > lineStart + 1) ? (lineEnd - (lineStart + 1)) : 0;
    if (!macro_expander_expand(&pp->expander, tokens + lineStart + 1, span, &expanded)) {
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "failed to expand #include operand");
        return false;
    }

    size_t tempCount = expanded.count + 2;
    Token* tempTokens = (Token*)calloc(tempCount, sizeof(Token));
    if (!tempTokens) {
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

    size_t tempCursor = 0;
    if (!parse_include_operand(tempTokens, tempCount, &tempCursor, &name, &isSystem)) {
        free(tempTokens);
        pp_token_buffer_destroy(&expanded);
        pp_report_diag(pp, tokens ? &tokens[*cursor] : NULL, DIAG_ERROR, CDIAG_PREPROCESSOR_GENERIC, "invalid #include operand");
        return false;
    }
    free(tempTokens);
    pp_token_buffer_destroy(&expanded);
    *cursor = (lineEnd == 0) ? 0 : lineEnd - 1;

    const char* parentFile = token_file(&tokens[*cursor]);
    IncludeSearchOrigin origin = INCLUDE_SEARCH_RAW;
    size_t originIndex = (size_t)-1;
    const IncludeFile* incPtr = include_resolver_load(pp->resolver,
                                                      parentFile,
                                                      name,
                                                      isSystem,
                                                      isIncludeNext,
                                                      &origin,
                                                      &originIndex);
    IncludeFile incValue;
    bool haveInc = false;
    if (incPtr) {
        incValue = *incPtr;
        incPtr = NULL;
        haveInc = true;
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
        if (!run_external_preprocessor(cmd,
                                       args,
                                       incValue.path,
                                       pp->includePaths,
                                       pp->includePathCount,
                                       &extBuf,
                                       &extLen)) {
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
        free(extBuf);
        include_resolver_mark_included(pp->resolver, incValue.path);
        return lexOk;
    }

    if (incValue.pragmaOnce && include_resolver_was_included(pp->resolver, incValue.path)) {
        return true;
    }

    if (pp_include_stack_contains(pp, incValue.path)) {
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

    int savedOffset = pp->lineOffset;
    char* savedLogical = pp->logicalFile;
    bool savedRemap = pp->lineRemapActive;
    pp->lineOffset = 0;
    pp->lineRemapActive = false;
    pp_set_logical_file(pp, incValue.path);

    Lexer lexer;
    initLexer(&lexer, incValue.contents, incValue.path, pp->enableTrigraphs);
    TokenBuffer buffer;
    token_buffer_init(&buffer);
    bool pushedFrame = pp_include_stack_push(pp, incValue.path, origin, originIndex);

    if (!token_buffer_fill_from_lexer(&buffer, &lexer)) {
        token_buffer_destroy(&buffer);
        destroyLexer(&lexer);
        pp->lineOffset = savedOffset;
        pp->logicalFile = savedLogical;
        pp->lineRemapActive = savedRemap;
        if (pushedFrame) pp_include_stack_pop(pp);
        return false;
    }
    destroyLexer(&lexer);

    const char* guard = detect_include_guard(&buffer);
    if (guard && macro_table_lookup(pp->table, guard) != NULL) {
        token_buffer_destroy(&buffer);
        include_resolver_mark_included(pp->resolver, incValue.path);
        pp->lineOffset = savedOffset;
        pp->logicalFile = savedLogical;
        pp->lineRemapActive = savedRemap;
        if (pushedFrame) pp_include_stack_pop(pp);
        return true;
    }

    bool ok = preprocess_tokens(pp, &buffer, output, false);
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
