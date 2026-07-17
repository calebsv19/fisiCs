// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include "lexer.h"
#include "lexer_internal.h"

#include "Compiler/diagnostics.h"

#define _POSIX_C_SOURCE 200809L
#include <unistd.h>

int print_statements = 0;
static struct CompilerContext* g_lexer_diag_ctx = NULL;

static int lexer_debug_flag = -1;
int lexer_debug_enabled(void) {
    if (lexer_debug_flag < 0) {
        const char* env = getenv("FISICS_DEBUG_LEXER");
        lexer_debug_flag = (env && env[0]) ? 1 : 0;
    }
    return lexer_debug_flag;
}

const char* lexer_file_path(const Lexer* lexer) {
    return (lexer && lexer->filePath) ? lexer->filePath : "<unknown>";
}

static const char* lexer_display_file_path_raw(const char* file, char* scratch, size_t scratchSize) {
    if (!file || file[0] != '/' || !scratch || scratchSize == 0) {
        return file;
    }

    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd))) {
        return file;
    }

    size_t cwdLen = strlen(cwd);
    if (strncmp(file, cwd, cwdLen) != 0 || file[cwdLen] != '/') {
        return file;
    }

    snprintf(scratch, scratchSize, "%s", file + cwdLen + 1);
    return scratch;
}

static const char* lexer_display_file_path(const Lexer* lexer, char* scratch, size_t scratchSize) {
    return lexer_display_file_path_raw(lexer_file_path(lexer), scratch, scratchSize);
}

static bool lexer_parse_line_directive(const char* line,
                                       size_t len,
                                       int* outLine,
                                       const char** outFileStart,
                                       size_t* outFileLen) {
    if (outLine) *outLine = 0;
    if (outFileStart) *outFileStart = NULL;
    if (outFileLen) *outFileLen = 0;
    if (!line || len == 0) return false;

    const char* p = line;
    const char* end = line + len;
    while (p < end && (*p == ' ' || *p == '\t')) p++;
    if (p >= end || *p != '#') return false;
    p++;
    while (p < end && (*p == ' ' || *p == '\t')) p++;
    if ((size_t)(end - p) < 4 || strncmp(p, "line", 4) != 0) return false;
    if ((p + 4) < end && (isalnum((unsigned char)p[4]) || p[4] == '_')) return false;
    p += 4;
    while (p < end && (*p == ' ' || *p == '\t')) p++;
    if (p >= end || !isdigit((unsigned char)*p)) return false;

    long requested = 0;
    while (p < end && isdigit((unsigned char)*p)) {
        requested = (requested * 10) + (long)(*p - '0');
        if (requested > 2147483647L) return false;
        p++;
    }
    if (requested <= 0) return false;

    while (p < end && (*p == ' ' || *p == '\t')) p++;
    if (p < end && *p == '"') {
        p++;
        const char* fs = p;
        while (p < end && *p != '"') p++;
        if (p >= end) return false;
        if (outFileStart) *outFileStart = fs;
        if (outFileLen) *outFileLen = (size_t)(p - fs);
        p++;
    }

    if (outLine) *outLine = (int)requested;
    return true;
}

static bool lexer_line_is_spliced_hash_directive_prefix(const char* line, size_t len) {
    if (!line || len == 0) return false;
    const char* p = line;
    const char* end = line + len;
    while (p < end && (*p == ' ' || *p == '\t')) p++;
    if (p >= end || *p != '#') return false;
    p++;
    while (p < end && (*p == ' ' || *p == '\t')) p++;
    if (p >= end || *p != '\\') return false;
    p++;
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r')) p++;
    return p == end;
}

static bool lexer_parse_spliced_line_directive(const char* line,
                                               size_t len,
                                               const char* nextLine,
                                               int* outLine,
                                               const char** outFileStart,
                                               size_t* outFileLen) {
    if (!lexer_line_is_spliced_hash_directive_prefix(line, len) || !nextLine) {
        return false;
    }
    const char* nextEnd = nextLine;
    while (*nextEnd && *nextEnd != '\n') nextEnd++;
    size_t nextLen = (size_t)(nextEnd - nextLine);
    char* joined = (char*)malloc(nextLen + 2);
    if (!joined) return false;
    joined[0] = '#';
    memcpy(joined + 1, nextLine, nextLen);
    bool ok = lexer_parse_line_directive(joined,
                                         nextLen + 1,
                                         outLine,
                                         outFileStart,
                                         outFileLen);
    if (ok && outFileStart && *outFileStart) {
        *outFileStart = nextLine + ((*outFileStart - joined) - 1);
    }
    free(joined);
    return ok;
}

static void lexer_map_logical_location(const Lexer* lexer,
                                       int physicalLine,
                                       int* outLine,
                                       const char** outFile,
                                       char* outFileScratch,
                                       size_t outFileScratchSize) {
    if (outLine) *outLine = physicalLine;
    if (outFile) *outFile = lexer_file_path(lexer);
    const char* source = (lexer && lexer->rawSource) ? lexer->rawSource :
                         ((lexer && lexer->source) ? lexer->source : NULL);
    if (!source || physicalLine <= 0) return;

    int lineOffset = 0;
    bool hasLogicalFile = false;
    const char* logicalFileStart = NULL;
    size_t logicalFileLen = 0;

    const char* p = source;
    int currentLine = 1;
    while (*p && currentLine < physicalLine) {
        const char* lineStart = p;
        while (*p && *p != '\n') p++;
        size_t lineLen = (size_t)(p - lineStart);

        int requestedLine = 0;
        const char* fileStart = NULL;
        size_t fileLen = 0;
        if (lexer_parse_line_directive(lineStart, lineLen, &requestedLine, &fileStart, &fileLen)) {
            int nextPhysical = currentLine + 1;
            lineOffset = requestedLine - nextPhysical;
            if (fileStart && fileLen > 0) {
                hasLogicalFile = true;
                logicalFileStart = fileStart;
                logicalFileLen = fileLen;
            }
        } else {
            const char* nextLine = (*p == '\n') ? p + 1 : NULL;
            if (lexer_parse_spliced_line_directive(lineStart, lineLen, nextLine, &requestedLine, &fileStart, &fileLen)) {
                int nextPhysical = currentLine + 2;
                lineOffset = requestedLine - nextPhysical;
                if (fileStart && fileLen > 0) {
                    hasLogicalFile = true;
                    logicalFileStart = fileStart;
                    logicalFileLen = fileLen;
                }
            }
        }

        if (*p == '\n') p++;
        currentLine++;
    }

    if (outLine) {
        int mapped = physicalLine + lineOffset;
        if (mapped < 1) mapped = 1;
        *outLine = mapped;
    }
    if (outFile && hasLogicalFile && outFileScratch && outFileScratchSize > 0) {
        size_t n = logicalFileLen;
        if (n >= outFileScratchSize) n = outFileScratchSize - 1;
        memcpy(outFileScratch, logicalFileStart, n);
        outFileScratch[n] = '\0';
        *outFile = outFileScratch;
    }
}

bool lexer_is_system_header_path(const char* file) {
    if (!file || file[0] != '/') return false;
    static const char* kPrefixes[] = {
        "/usr/include/",
        "/usr/local/include/",
        "/opt/homebrew/include/",
        "/Library/Developer/",
        "/Applications/Xcode.app/",
        "/Applications/Xcode-beta.app/",
        "/Library/Frameworks/",
        "/System/Library/"
    };
    for (size_t i = 0; i < sizeof(kPrefixes) / sizeof(kPrefixes[0]); ++i) {
        size_t n = strlen(kPrefixes[i]);
        if (strncmp(file, kPrefixes[i], n) == 0) {
            return true;
        }
    }
    return false;
}

static inline int lexer_compute_column(int position, int lineStart) {
    int column = (position - lineStart) + 1;
    return (column < 1) ? 1 : column;
}

static void lexer_raw_location_for_position(const Lexer* lexer,
                                            int position,
                                            int fallbackLine,
                                            int fallbackLineStart,
                                            int* outLine,
                                            int* outColumn) {
    int line = fallbackLine;
    int translatedColumn = lexer_compute_column(position, fallbackLineStart);
    int column = translatedColumn;
    if (lexer && position >= 0 && position <= lexer->length &&
        lexer->rawLineMap && lexer->rawColumnMap) {
        int mappedLine = lexer->rawLineMap[position];
        int mappedColumn = lexer->rawColumnMap[position];
        if (mappedLine > 0) {
            line = mappedLine;
            if (mappedLine != fallbackLine && mappedColumn > 0) {
                column = mappedColumn;
            }
        }
    }
    if (outLine) *outLine = line;
    if (outColumn) *outColumn = column;
}

void lexer_mark_source_location(const Lexer* lexer, LexerMark mark, int* outLine, int* outColumn) {
    lexer_raw_location_for_position(lexer,
                                    mark.position,
                                    mark.line,
                                    mark.lineStart,
                                    outLine,
                                    outColumn);
}

void lexer_set_diag_context(struct CompilerContext* ctx) {
    g_lexer_diag_ctx = ctx;
}

void report_lexer_error(Lexer* lexer, LexerMark start, const char* message, const char* got) {
    if (!lexer || !message) return;
    int rawLine = start.line;
    int rawColumn = lexer_compute_column(start.position, start.lineStart);
    lexer_raw_location_for_position(lexer,
                                    start.position,
                                    start.line,
                                    start.lineStart,
                                    &rawLine,
                                    &rawColumn);
    char logicalFileScratch[4096];
    const char* logicalFile = lexer_file_path(lexer);
    int logicalLine = rawLine;
    lexer_map_logical_location(lexer,
                               rawLine,
                               &logicalLine,
                               &logicalFile,
                               logicalFileScratch,
                               sizeof(logicalFileScratch));
    char fileScratch[4096];
    const char* file = lexer_display_file_path_raw(logicalFile, fileScratch, sizeof(fileScratch));
    int column = rawColumn;
    if (got && got[0] != '\0') {
        fprintf(stderr,
                "Error: %s at %s:%d:%d (got '%s')\n",
                message,
                file,
                logicalLine,
                column,
                got);
    } else {
        fprintf(stderr,
                "Error: %s at %s:%d:%d\n",
                message,
                file,
                logicalLine,
                column);
    }
    if (g_lexer_diag_ctx) {
        SourceRange loc;
        loc.start.file = logicalFile;
        loc.start.line = logicalLine;
        loc.start.column = column;
        loc.end = loc.start;
        if (got && got[0] != '\0') {
            compiler_report_diag(g_lexer_diag_ctx,
                                 loc,
                                 DIAG_ERROR,
                                 CDIAG_GENERIC,
                                 NULL,
                                 "%s (got '%s')",
                                 message,
                                 got);
        } else {
            compiler_report_diag(g_lexer_diag_ctx,
                                 loc,
                                 DIAG_ERROR,
                                 CDIAG_GENERIC,
                                 NULL,
                                 "%s",
                                 message);
        }
    }
    lexer->fatalErrorCount += 1;
}

static void report_unsupported_ucn_identifier(Lexer* lexer, LexerMark start, const char* text) {
    if (!lexer || !text) return;
    char fileScratch[4096];
    const char* file = lexer_display_file_path(lexer, fileScratch, sizeof(fileScratch));
    fprintf(stderr,
            "Error: universal character names in identifiers are not supported yet at %s:%d (got '%s')\n",
            file,
            start.line,
            text);
    if (g_lexer_diag_ctx) {
        int rawLine = start.line;
        int rawColumn = lexer_compute_column(start.position, start.lineStart);
        lexer_raw_location_for_position(lexer,
                                        start.position,
                                        start.line,
                                        start.lineStart,
                                        &rawLine,
                                        &rawColumn);
        char logicalFileScratch[4096];
        const char* logicalFile = lexer_file_path(lexer);
        int logicalLine = rawLine;
        lexer_map_logical_location(lexer,
                                   rawLine,
                                   &logicalLine,
                                   &logicalFile,
                                   logicalFileScratch,
                                   sizeof(logicalFileScratch));
        SourceRange loc;
        loc.start.file = logicalFile;
        loc.start.line = logicalLine;
        loc.start.column = rawColumn;
        loc.end = loc.start;
        compiler_report_diag(g_lexer_diag_ctx,
                             loc,
                             DIAG_ERROR,
                             CDIAG_GENERIC,
                             NULL,
                             "universal character names in identifiers are not supported yet (got '%s')",
                             text);
    }
    lexer->fatalErrorCount += 1;
}

static void prepend_literal_prefix(Token* tok, const char* prefix) {
    if (!tok || !tok->value || !prefix) return;
    size_t plen = strlen(prefix);
    size_t vlen = strlen(tok->value);
    char* buf = (char*)malloc(plen + vlen + 1);
    if (!buf) return;
    memcpy(buf, prefix, plen);
    memcpy(buf + plen, tok->value, vlen + 1);
    free(tok->value);
    tok->value = buf;
}

static void mark_ambiguous_narrow_string_literal(Token* tok) {
    if (!tok || tok->type != TOKEN_STRING || !tok->value) return;
    if (strncmp(tok->value, "W|", 2) == 0 || strncmp(tok->value, "U8|", 3) == 0) {
        prepend_literal_prefix(tok, "N|");
    }
}

static int lexer_hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool lexer_ucn_is_supported_identifier_character(unsigned codepoint,
                                                         bool initial) {
    (void)initial;
    bool greekLetter =
        (codepoint >= 0x0370u && codepoint <= 0x0373u) ||
        (codepoint >= 0x0376u && codepoint <= 0x0377u) ||
        (codepoint >= 0x037Bu && codepoint <= 0x037Du) ||
        codepoint == 0x037Fu || codepoint == 0x0386u ||
        (codepoint >= 0x0388u && codepoint <= 0x038Au) ||
        codepoint == 0x038Cu ||
        (codepoint >= 0x038Eu && codepoint <= 0x03A1u) ||
        (codepoint >= 0x03A3u && codepoint <= 0x03F5u) ||
        (codepoint >= 0x03F7u && codepoint <= 0x03FFu);
    return greekLetter;
}

int lexer_identifier_ucn_length(const Lexer* lexer, int position, bool initial) {
    if (!lexer || !lexer->source || position < 0 || position + 1 >= lexer->length ||
        lexer->source[position] != '\\' ||
        (lexer->source[position + 1] != 'u' && lexer->source[position + 1] != 'U')) {
        return 0;
    }
    int digits = lexer->source[position + 1] == 'u' ? 4 : 8;
    unsigned codepoint = 0;
    for (int i = 0; i < digits; ++i) {
        int at = position + 2 + i;
        if (at >= lexer->length) return 0;
        int value = lexer_hex_value(lexer->source[at]);
        if (value < 0) return 0;
        codepoint = (codepoint << 4) | (unsigned)value;
    }
    if (codepoint > 0x10FFFFu ||
        (codepoint >= 0xD800u && codepoint <= 0xDFFFu) ||
        !lexer_ucn_is_supported_identifier_character(codepoint, initial)) {
        return 0;
    }
    return digits + 2;
}

static int lexer_decode_utf8(const char* text,
                             size_t available,
                             unsigned* outCodepoint) {
    if (!text || available == 0 || !outCodepoint) return 0;
    const unsigned char* bytes = (const unsigned char*)text;
    unsigned codepoint = 0;
    int length = 0;
    if (bytes[0] >= 0xC2u && bytes[0] <= 0xDFu) {
        length = 2;
        codepoint = bytes[0] & 0x1Fu;
    } else if (bytes[0] >= 0xE0u && bytes[0] <= 0xEFu) {
        length = 3;
        codepoint = bytes[0] & 0x0Fu;
    } else if (bytes[0] >= 0xF0u && bytes[0] <= 0xF4u) {
        length = 4;
        codepoint = bytes[0] & 0x07u;
    } else {
        return 0;
    }
    if ((size_t)length > available) return 0;
    for (int i = 1; i < length; ++i) {
        if ((bytes[i] & 0xC0u) != 0x80u) return 0;
        codepoint = (codepoint << 6) | (bytes[i] & 0x3Fu);
    }
    if ((length == 3 && codepoint < 0x800u) ||
        (length == 4 && codepoint < 0x10000u) ||
        codepoint > 0x10FFFFu ||
        (codepoint >= 0xD800u && codepoint <= 0xDFFFu)) {
        return 0;
    }
    *outCodepoint = codepoint;
    return length;
}

int lexer_identifier_utf8_length(const Lexer* lexer, int position, bool initial) {
    if (!lexer || !lexer->source || position < 0 || position >= lexer->length) return 0;
    unsigned codepoint = 0;
    int length = lexer_decode_utf8(lexer->source + position,
                                   (size_t)(lexer->length - position),
                                   &codepoint);
    if (length == 0 || !lexer_ucn_is_supported_identifier_character(codepoint, initial)) {
        return 0;
    }
    return length;
}

static int lexer_encode_utf8(unsigned codepoint, char out[4]) {
    if (codepoint <= 0x7Fu) {
        out[0] = (char)codepoint;
        return 1;
    }
    if (codepoint <= 0x7FFu) {
        out[0] = (char)(0xC0u | (codepoint >> 6));
        out[1] = (char)(0x80u | (codepoint & 0x3Fu));
        return 2;
    }
    if (codepoint <= 0xFFFFu) {
        out[0] = (char)(0xE0u | (codepoint >> 12));
        out[1] = (char)(0x80u | ((codepoint >> 6) & 0x3Fu));
        out[2] = (char)(0x80u | (codepoint & 0x3Fu));
        return 3;
    }
    out[0] = (char)(0xF0u | (codepoint >> 18));
    out[1] = (char)(0x80u | ((codepoint >> 12) & 0x3Fu));
    out[2] = (char)(0x80u | ((codepoint >> 6) & 0x3Fu));
    out[3] = (char)(0x80u | (codepoint & 0x3Fu));
    return 4;
}

char* lexer_normalize_identifier_spelling(const char* text, size_t length) {
    if (!text) return NULL;
    char* normalized = (char*)malloc(length + 1);
    if (!normalized) return NULL;
    size_t read = 0;
    size_t written = 0;
    while (read < length) {
        if (text[read] == '\\' && read + 1 < length &&
            (text[read + 1] == 'u' || text[read + 1] == 'U')) {
            int digits = text[read + 1] == 'u' ? 4 : 8;
            unsigned codepoint = 0;
            bool valid = read + 2u + (size_t)digits <= length;
            for (int i = 0; valid && i < digits; ++i) {
                int value = lexer_hex_value(text[read + 2u + (size_t)i]);
                if (value < 0) {
                    valid = false;
                } else {
                    codepoint = (codepoint << 4) | (unsigned)value;
                }
            }
            if (valid) {
                char encoded[4];
                int encodedLength = lexer_encode_utf8(codepoint, encoded);
                memcpy(normalized + written, encoded, (size_t)encodedLength);
                written += (size_t)encodedLength;
                read += 2u + (size_t)digits;
                continue;
            }
        }
        normalized[written++] = text[read++];
    }
    normalized[written] = '\0';
    return normalized;
}

LexerMark lexer_mark(const Lexer* lexer) {
    LexerMark mark = {0};
    if (lexer) {
        mark.position = lexer->position;
        mark.line = lexer->line;
        mark.lineStart = lexer->lineStart;
    }
    return mark;
}

static inline SourceLocation lexer_build_location(const Lexer* lexer,
                                                  int position,
                                                  int line,
                                                  int lineStart) {
    SourceLocation loc;
    loc.file = lexer_file_path(lexer);
    loc.line = line;
    loc.column = lexer_compute_column(position, lineStart);
    return loc;
}

static inline SourceRange lexer_build_range(const Lexer* lexer, LexerMark start) {
    SourceRange range;
    range.start = lexer_build_location(lexer, start.position, start.line, start.lineStart);
    range.end   = lexer_build_location(lexer, lexer ? lexer->position : 0,
                                       lexer ? lexer->line : 0,
                                       lexer ? lexer->lineStart : 0);
    return range;
}

static inline SourceRange empty_source_range(void) {
    SourceRange range;
    range.start.file = NULL;
    range.start.line = 0;
    range.start.column = 0;
    range.end = range.start;
    return range;
}

Token make_token(Lexer* lexer, TokenType type, char* value, LexerMark start) {
    Token token;
    token.type = type;
    token.value = value;
    token.line = start.line;
    token.spellingLine = start.line;
    token.spellingColumn = lexer_compute_column(start.position, start.lineStart);
    lexer_raw_location_for_position(lexer,
                                    start.position,
                                    start.line,
                                    start.lineStart,
                                    &token.spellingLine,
                                    &token.spellingColumn);
    token.location = lexer_build_range(lexer, start);
    token.macroCallSite = empty_source_range();
    token.macroDefinition = empty_source_range();
    return token;
}

static char translate_trigraph_char(char c) {
    switch (c) {
        case '=': return '#';
        case '/': return '\\';
        case '\'': return '^';
        case '<': return '{';
        case '>': return '}';
        case '!': return '|';
        case '(': return '[';
        case ')': return ']';
        case '-': return '~';
        default: return 0;
    }
}

static void advance_raw_position(const char* source, size_t* index, size_t count, int* line, int* column) {
    if (!source || !index || !line || !column) return;
    for (size_t n = 0; n < count && source[*index]; ++n) {
        char c = source[*index];
        *index += 1;
        if (c == '\n') {
            *line += 1;
            *column = 1;
        } else {
            *column += 1;
        }
    }
}

static void record_translated_position(int* lineMap,
                                       int* columnMap,
                                       size_t position,
                                       int rawLine,
                                       int rawColumn) {
    if (lineMap) lineMap[position] = rawLine;
    if (columnMap) columnMap[position] = rawColumn;
}

static char* translate_source(const char* source,
                              bool enableTrigraphs,
                              bool* outSawTrigraph,
                              int** outLineMap,
                              int** outColumnMap) {
    if (!source) return NULL;
    size_t len = strlen(source);
    char* out = (char*)malloc(len + 1);
    int* lineMap = (int*)calloc(len + 1, sizeof(int));
    int* columnMap = (int*)calloc(len + 1, sizeof(int));
    if (!out || !lineMap || !columnMap) {
        free(out);
        free(lineMap);
        free(columnMap);
        return NULL;
    }
    size_t w = 0;
    bool sawTrigraph = false;
    int rawLine = 1;
    int rawColumn = 1;
    for (size_t i = 0; i < len; ) {
        if (enableTrigraphs && i + 2 < len && source[i] == '?' && source[i + 1] == '?') {
            char mapped = translate_trigraph_char(source[i + 2]);
            if (mapped) {
                size_t next = i + 3;
                if (mapped == '\\' && next < len) {
                    if (source[next] == '\n') {
                        advance_raw_position(source, &i, 4, &rawLine, &rawColumn);
                        sawTrigraph = true;
                        continue;
                    }
                    if (source[next] == '\r' && next + 1 < len && source[next + 1] == '\n') {
                        advance_raw_position(source, &i, 5, &rawLine, &rawColumn);
                        sawTrigraph = true;
                        continue;
                    }
                }
                record_translated_position(lineMap, columnMap, w, rawLine, rawColumn);
                out[w++] = mapped;
                advance_raw_position(source, &i, 3, &rawLine, &rawColumn);
                sawTrigraph = true;
                continue;
            }
        }
        if (source[i] == '\\' && i + 1 < len) {
            if (source[i + 1] == '\n') {
                advance_raw_position(source, &i, 2, &rawLine, &rawColumn);
                continue;
            }
            if (source[i + 1] == '\r' && i + 2 < len && source[i + 2] == '\n') {
                advance_raw_position(source, &i, 3, &rawLine, &rawColumn);
                continue;
            }
        }
        if (i + 3 < len && source[i] == '%' && source[i + 1] == ':' &&
            source[i + 2] == '%' && source[i + 3] == ':') {
            record_translated_position(lineMap, columnMap, w, rawLine, rawColumn);
            out[w++] = '#';
            record_translated_position(lineMap, columnMap, w, rawLine, rawColumn + 2);
            out[w++] = '#';
            advance_raw_position(source, &i, 4, &rawLine, &rawColumn);
            continue;
        }
        if (i + 1 < len && source[i] == '%' && source[i + 1] == ':') {
            record_translated_position(lineMap, columnMap, w, rawLine, rawColumn);
            out[w++] = '#';
            advance_raw_position(source, &i, 2, &rawLine, &rawColumn);
            continue;
        }
        if (i + 1 < len && source[i] == '<' && source[i + 1] == ':') {
            record_translated_position(lineMap, columnMap, w, rawLine, rawColumn);
            out[w++] = '[';
            advance_raw_position(source, &i, 2, &rawLine, &rawColumn);
            continue;
        }
        if (i + 1 < len && source[i] == ':' && source[i + 1] == '>') {
            record_translated_position(lineMap, columnMap, w, rawLine, rawColumn);
            out[w++] = ']';
            advance_raw_position(source, &i, 2, &rawLine, &rawColumn);
            continue;
        }
        if (i + 1 < len && source[i] == '<' && source[i + 1] == '%') {
            record_translated_position(lineMap, columnMap, w, rawLine, rawColumn);
            out[w++] = '{';
            advance_raw_position(source, &i, 2, &rawLine, &rawColumn);
            continue;
        }
        if (i + 1 < len && source[i] == '%' && source[i + 1] == '>') {
            record_translated_position(lineMap, columnMap, w, rawLine, rawColumn);
            out[w++] = '}';
            advance_raw_position(source, &i, 2, &rawLine, &rawColumn);
            continue;
        }
        record_translated_position(lineMap, columnMap, w, rawLine, rawColumn);
        out[w++] = source[i];
        advance_raw_position(source, &i, 1, &rawLine, &rawColumn);
    }
    out[w] = '\0';
    record_translated_position(lineMap, columnMap, w, rawLine, rawColumn);
    if (outSawTrigraph) *outSawTrigraph = sawTrigraph;
    if (outLineMap) {
        int* shrunkLineMap = realloc(lineMap, (w + 1) * sizeof(int));
        *outLineMap = shrunkLineMap ? shrunkLineMap : lineMap;
    } else {
        free(lineMap);
    }
    if (outColumnMap) {
        int* shrunkColumnMap = realloc(columnMap, (w + 1) * sizeof(int));
        *outColumnMap = shrunkColumnMap ? shrunkColumnMap : columnMap;
    } else {
        free(columnMap);
    }
    char* shrunk = realloc(out, w + 1);
    return shrunk ? shrunk : out;
}

void initLexer(Lexer* lexer, const char* source, const char* filePath, bool enableTrigraphs){
	lexer->ownedSource = NULL;
	lexer->enableTrigraphs = enableTrigraphs;
	lexer->filePath = filePath;
	lexer->position = 0;
    lexer->length = 0;
	lexer->line = 1;
	lexer->lineStart = 0;
    lexer->fatalErrorCount = 0;
    lexer->rawSource = source ? source : "";
    lexer->rawLineMap = NULL;
    lexer->rawColumnMap = NULL;
    bool sawTrigraph = false;
    const char* rawSource = source ? source : "";
    char* translated = translate_source(rawSource,
                                        enableTrigraphs,
                                        &sawTrigraph,
                                        &lexer->rawLineMap,
                                        &lexer->rawColumnMap);
    if (translated) {
        lexer->ownedSource = translated;
        lexer->source = translated;
    } else {
        lexer->source = rawSource;
    }
    lexer->length = (int)strlen(lexer->source);
    if (sawTrigraph && !enableTrigraphs) {
        fprintf(stderr, "warning: trigraphs present; enable trigraph translation to honor them\n");
    }
}

void destroyLexer(Lexer* lexer) {
    if (!lexer) return;
    free(lexer->ownedSource);
    free(lexer->rawLineMap);
    free(lexer->rawColumnMap);
    lexer->ownedSource = NULL;
    lexer->rawLineMap = NULL;
    lexer->rawColumnMap = NULL;
    lexer->rawSource = NULL;
    lexer->source = NULL;
    lexer->length = 0;
}

Token getNextToken(Lexer* lexer) {
    skipWhitespace(lexer);

    if (lexer_debug_enabled()) {
    	LEXER_DEBUG_PRINTF("DEBUG: Current char in getNextToken(): '%c' (ASCII: %d) at line %d\n",
    	 	      (lexer->position < lexer->length ? lexer->source[lexer->position] : '\0'),
                  (lexer->position < lexer->length ? lexer->source[lexer->position] : 0),
                  lexer->line);
    }

    if (isEOF(lexer)) {
        LexerMark start = lexer_mark(lexer);
        lexer->position++; // move past terminator to avoid repeated EOF
        return make_token(lexer, TOKEN_EOF, (char*)"EOF", start);
    }

    int pos = lexer->position;
    char c = lexer->source[pos];
    char n = (pos + 1 < lexer->length) ? lexer->source[pos + 1] : '\0';
    char n2 = (pos + 2 < lexer->length) ? lexer->source[pos + 2] : '\0';
    // Wide/UTF-prefixed string/char literals
    if (c == 'u' && n == '8' && n2 == '"') {
        lexer->position += 2; // consume u8
        Token t = handleStringLiteral(lexer);
        prepend_literal_prefix(&t, "U8|");
        return t;
    }
    if ((c == 'L' || c == 'u' || c == 'U') && n == '"') {
        lexer->position += 1; // consume prefix
        Token t = handleStringLiteral(lexer);
        prepend_literal_prefix(&t, "W|");
        return t;
    }
    if ((c == 'L' || c == 'u' || c == 'U') && n == '\'') {
        lexer->position += 1; // consume prefix
        Token t = handleCharLiteral(lexer);
        prepend_literal_prefix(&t, "W|");
        return t;
    }
    if (c == '\\' && (n == 'u' || n == 'U')) {
        if (lexer_identifier_ucn_length(lexer, lexer->position, true) > 0) {
            return handleIdentifierOrKeyword(lexer);
        }
        LexerMark start = lexer_mark(lexer);
        int startPos = lexer->position;
        int expectedDigits = (n == 'u') ? 4 : 8;
        lexer->position += 2;
        for (int i = 0; i < expectedDigits; ++i) {
            if (!isxdigit((unsigned char)lexer->source[lexer->position])) {
                break;
            }
            lexer->position++;
        }
        char* text = strndup(lexer->source + startPos, lexer->position - startPos);
        report_unsupported_ucn_identifier(lexer, start, text);
        return make_token(lexer, TOKEN_UNKNOWN, text, start);
    }

    if (isalpha((unsigned char)lexer->source[lexer->position]) ||
        lexer->source[lexer->position] == '_' ||
        lexer_identifier_utf8_length(lexer, lexer->position, true) > 0) {
        return handleIdentifierOrKeyword(lexer);
    }

    if (isdigit(lexer->source[lexer->position])) {
        return handleNumber(lexer);
    }

    // C permits floating literals that begin with a dot (e.g. `.8`).
    if (lexer->source[lexer->position] == '.' &&
        lexer->position + 1 < lexer->length &&
        isdigit((unsigned char)lexer->source[lexer->position + 1])) {
        return handleNumber(lexer);
    }

    if (lexer->source[lexer->position] == '"') {
        Token t = handleStringLiteral(lexer);
        mark_ambiguous_narrow_string_literal(&t);
        return t;
    }


    if (lexer->source[lexer->position] == '\'') {
        LEXER_DEBUG_PRINTF("it worked it worked\n");
        return handleCharLiteral(lexer);
    }

    if (lexer->source[lexer->position] == '#') {
        return handlePreprocessorDirective(lexer);
    }

    if (lexer->source[lexer->position] == '/') {
        return handleComment(lexer);
    }

    if (strchr("=+-*/%<>!&|^~", lexer->source[lexer->position])) {
        return handleOperator(lexer);
    }

    if (strchr("?:;,(){}[].", lexer->source[lexer->position])) {
        return handlePunctuation(lexer);
    }

    return handleUnknownToken(lexer);
}


// Skips whitespace and keeps track of line numbers
void skipWhitespace(Lexer* lexer) {
    while (lexer->position < lexer->length) {
        char c = lexer->source[lexer->position];
        char next = (lexer->position + 1 < lexer->length) ? lexer->source[lexer->position + 1] : '\0';
        char next2 = (lexer->position + 2 < lexer->length) ? lexer->source[lexer->position + 2] : '\0';

        // Line splices: remove backslash + newline (handles \n and \r\n).
        if (c == '\\' && next == '\n') {
            lexer->position += 2;
            continue;
        }
        if (c == '\\' && next == '\r' && next2 == '\n') {
            lexer->position += 3;
            continue;
        }

        if (!isspace((unsigned char)c)) {
            break;
        }
        if (c == '\n') {
            lexer->position++;
            lexer->line++;
            lexer->lineStart = lexer->position;
            continue;
        }
        lexer->position++;
    }
}

// Checks for end of file
int isEOF(Lexer* lexer) {
    return !lexer || lexer->position >= lexer->length || lexer->source[lexer->position] == '\0';
}
