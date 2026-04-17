// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/preprocessor_external.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool args_has_flag(const char* args, const char* flag) {
    if (!args || !flag) return false;
    const char* hit = strstr(args, flag);
    if (!hit) return false;
    if (hit != args && hit[-1] != ' ') return false;
    {
        const char next = hit[strlen(flag)];
        return next == '\0' || next == ' ';
    }
}

static char* shell_quote(const char* text) {
    if (!text) return NULL;
    size_t len = 2;
    for (const char* p = text; *p; ++p) {
        len += (*p == '\'') ? 4 : 1;
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
    {
        size_t add = strlen(text);
        size_t extra = add + ((*len > 0) ? 1 : 0);
        if (*len + extra + 1 > *cap) {
            size_t newCap = (*cap > 0) ? *cap : 128;
            while (newCap < *len + extra + 1) {
                newCap *= 2;
            }
            {
                char* grown = (char*)realloc(*buf, newCap);
                if (!grown) return false;
                *buf = grown;
                *cap = newCap;
            }
        }
        if (*len > 0) {
            (*buf)[(*len)++] = ' ';
        }
        memcpy(*buf + *len, text, add);
        *len += add;
        (*buf)[*len] = '\0';
    }
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

    if (args && args[0] && !cmd_append(&buffer, &len, &cap, args)) {
        free(buffer);
        return NULL;
    }

    if (!args_has_flag(args, "-E") && !cmd_append(&buffer, &len, &cap, "-E")) {
        free(buffer);
        return NULL;
    }
    if (!args_has_flag(args, "-P") && !cmd_append(&buffer, &len, &cap, "-P")) {
        free(buffer);
        return NULL;
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

    {
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
    }

    return buffer;
}

bool pp_run_external_preprocessor(const char* cmd,
                                  const char* args,
                                  const char* inputPath,
                                  const char* const* includePaths,
                                  size_t includePathCount,
                                  char** outBuffer,
                                  size_t* outLength) {
    if (!outBuffer || !outLength) return false;
    *outBuffer = NULL;
    *outLength = 0;

    char* command = build_external_preprocess_command(cmd,
                                                      args,
                                                      inputPath,
                                                      includePaths,
                                                      includePathCount);
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
    if (pclose(fp) != 0) {
        free(buffer);
        return false;
    }
    if (!buffer) {
        buffer = strdup("");
        if (!buffer) return false;
    }

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
