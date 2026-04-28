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

static void lexer_map_logical_location(const Lexer* lexer,
                                       int physicalLine,
                                       int* outLine,
                                       const char** outFile,
                                       char* outFileScratch,
                                       size_t outFileScratchSize) {
    if (outLine) *outLine = physicalLine;
    if (outFile) *outFile = lexer_file_path(lexer);
    if (!lexer || !lexer->source || physicalLine <= 0) return;

    int lineOffset = 0;
    bool hasLogicalFile = false;
    const char* logicalFileStart = NULL;
    size_t logicalFileLen = 0;

    const char* p = lexer->source;
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

void lexer_set_diag_context(struct CompilerContext* ctx) {
    g_lexer_diag_ctx = ctx;
}

void report_lexer_error(Lexer* lexer, LexerMark start, const char* message, const char* got) {
    if (!lexer || !message) return;
    char logicalFileScratch[4096];
    const char* logicalFile = lexer_file_path(lexer);
    int logicalLine = start.line;
    lexer_map_logical_location(lexer,
                               start.line,
                               &logicalLine,
                               &logicalFile,
                               logicalFileScratch,
                               sizeof(logicalFileScratch));
    char fileScratch[4096];
    const char* file = lexer_display_file_path_raw(logicalFile, fileScratch, sizeof(fileScratch));
    int column = lexer_compute_column(start.position, start.lineStart);
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

static char* translate_source(const char* source, bool enableTrigraphs, bool* outSawTrigraph) {
    if (!source) return NULL;
    size_t len = strlen(source);
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    size_t w = 0;
    bool sawTrigraph = false;
    for (size_t i = 0; i < len; ) {
        if (enableTrigraphs && i + 2 < len && source[i] == '?' && source[i + 1] == '?') {
            char mapped = translate_trigraph_char(source[i + 2]);
            if (mapped) {
                size_t next = i + 3;
                if (mapped == '\\' && next < len) {
                    if (source[next] == '\n') {
                        i = next + 1;
                        sawTrigraph = true;
                        continue;
                    }
                    if (source[next] == '\r' && next + 1 < len && source[next + 1] == '\n') {
                        i = next + 2;
                        sawTrigraph = true;
                        continue;
                    }
                }
                out[w++] = mapped;
                i += 3;
                sawTrigraph = true;
                continue;
            }
        }
        if (source[i] == '\\' && i + 1 < len) {
            if (source[i + 1] == '\n') {
                i += 2;
                continue;
            }
            if (source[i + 1] == '\r' && i + 2 < len && source[i + 2] == '\n') {
                i += 3;
                continue;
            }
        }
        if (i + 3 < len && source[i] == '%' && source[i + 1] == ':' &&
            source[i + 2] == '%' && source[i + 3] == ':') {
            out[w++] = '#';
            out[w++] = '#';
            i += 4;
            continue;
        }
        if (i + 1 < len && source[i] == '%' && source[i + 1] == ':') {
            out[w++] = '#';
            i += 2;
            continue;
        }
        if (i + 1 < len && source[i] == '<' && source[i + 1] == ':') {
            out[w++] = '[';
            i += 2;
            continue;
        }
        if (i + 1 < len && source[i] == ':' && source[i + 1] == '>') {
            out[w++] = ']';
            i += 2;
            continue;
        }
        if (i + 1 < len && source[i] == '<' && source[i + 1] == '%') {
            out[w++] = '{';
            i += 2;
            continue;
        }
        if (i + 1 < len && source[i] == '%' && source[i + 1] == '>') {
            out[w++] = '}';
            i += 2;
            continue;
        }
        out[w++] = source[i++];
    }
    out[w] = '\0';
    if (outSawTrigraph) *outSawTrigraph = sawTrigraph;
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
    bool sawTrigraph = false;
    const char* rawSource = source ? source : "";
    char* translated = translate_source(rawSource, enableTrigraphs, &sawTrigraph);
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
    lexer->ownedSource = NULL;
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

    if (isalpha(lexer->source[lexer->position]) || lexer->source[lexer->position] == '_') {
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
