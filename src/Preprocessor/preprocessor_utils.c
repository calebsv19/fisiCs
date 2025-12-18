#include "Preprocessor/pp_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Compiler/diagnostics.h"
#include "Lexer/tokens.h"

static bool append_expr_token(Preprocessor* pp, PPTokenBuffer* out, const Token* tok) {
    if (!tok) return true;
    if (tok->type == TOKEN_IDENTIFIER && tok->value) {
        if (strcmp(tok->value, "__LINE__") == 0) {
            char buffer[32];
            const Token remapped = pp_token_clone_remap(pp, tok);
            snprintf(buffer, sizeof(buffer), "%d", remapped.location.start.line);
            return pp_append_number_literal(pp, out, tok, buffer);
        }
        if (strcmp(tok->value, "__COUNTER__") == 0) {
            int value = pp ? pp->counter : 0;
            if (pp) pp->counter++;
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%d", value);
            return pp_append_number_literal(pp, out, tok, buffer);
        }
        if (strcmp(tok->value, "__FILE__") == 0 ||
            strcmp(tok->value, "__BASE_FILE__") == 0 ||
            strcmp(tok->value, "__DATE__") == 0 ||
            strcmp(tok->value, "__TIME__") == 0) {
            return pp_append_number_literal(pp, out, tok, "0");
        }
    }
    return pp_token_buffer_append_clone_remap(out, pp, tok);
}

void pp_token_buffer_init_local(PPTokenBuffer* buffer) {
    if (!buffer) return;
    buffer->tokens = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
}

bool pp_token_buffer_reserve(PPTokenBuffer* buffer, size_t extra) {
    if (!buffer) return false;
    size_t required = buffer->count + extra;
    if (required <= buffer->capacity) {
        return true;
    }
    size_t newCapacity = buffer->capacity ? buffer->capacity * 2 : 16;
    while (newCapacity < required) {
        newCapacity *= 2;
    }
    Token* tokens = (Token*)realloc(buffer->tokens, newCapacity * sizeof(Token));
    if (!tokens) {
        return false;
    }
    buffer->tokens = tokens;
    buffer->capacity = newCapacity;
    return true;
}

char* pp_strdup_local(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = (char*)malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}

void pp_token_free(Token* tok) {
    if (!tok) return;
    free(tok->value);
    tok->value = NULL;
}

Token pp_token_clone(const Token* tok) {
    Token clone = *tok;
    if (tok->value) {
        clone.value = pp_strdup_local(tok->value);
    }
    return clone;
}

Token pp_token_clone_remap(Preprocessor* pp, const Token* tok) {
    Token clone = pp_token_clone(tok);
    if (!pp) return clone;
    if (pp->lineRemapActive) {
        const char* logicalFile = pp->logicalFile ? pp->logicalFile : clone.location.start.file;
        if (logicalFile) {
            clone.location.start.file = logicalFile;
            clone.location.end.file = logicalFile;
        }
        if (pp->lineOffset != 0) {
            clone.location.start.line += pp->lineOffset;
            clone.location.end.line += pp->lineOffset;
            if (clone.location.start.line < 1) clone.location.start.line = 1;
            if (clone.location.end.line < 1) clone.location.end.line = 1;
        }
    }
    return clone;
}

bool pp_token_buffer_append_token(PPTokenBuffer* buffer, Token token) {
    if (!pp_token_buffer_reserve(buffer, 1)) {
        pp_token_free(&token);
        return false;
    }
    buffer->tokens[buffer->count++] = token;
    return true;
}

bool pp_token_buffer_append_clone(PPTokenBuffer* buffer, const Token* tok) {
    Token clone = pp_token_clone(tok);
    if (!pp_token_buffer_append_token(buffer, clone)) {
        pp_token_free(&clone);
        return false;
    }
    return true;
}

bool pp_token_buffer_append_clone_remap(PPTokenBuffer* buffer,
                                        Preprocessor* pp,
                                        const Token* tok) {
    Token clone = pp_token_clone_remap(pp, tok);
    if (!pp_token_buffer_append_token(buffer, clone)) {
        pp_token_free(&clone);
        return false;
    }
    return true;
}

bool pp_token_buffer_append_buffer(PPTokenBuffer* dest, const PPTokenBuffer* src) {
    if (!dest || !src) return true;
    for (size_t i = 0; i < src->count; ++i) {
        if (!pp_token_buffer_append_clone(dest, &src->tokens[i])) {
            return false;
        }
    }
    return true;
}

void pp_token_buffer_reset(PPTokenBuffer* buffer) {
    if (!buffer) return;
    for (size_t i = 0; i < buffer->count; ++i) {
        pp_token_free(&buffer->tokens[i]);
    }
    free(buffer->tokens);
    buffer->tokens = NULL;
    buffer->count = 0;
    buffer->capacity = 0;
}

void skip_to_line_end(const Token* tokens, size_t count, size_t* cursor) {
    size_t i = *cursor;
    int line = tokens[i].line;
    while (i < count && tokens[i].type != TOKEN_EOF && tokens[i].line == line) {
        i++;
    }
    *cursor = (i == 0) ? 0 : i - 1;
}

bool pp_debug_fail_enabled(void) {
    static int cached = -1;
    if (cached < 0) {
        const char* env = getenv("FISICS_DEBUG_PP_FAIL");
        cached = (env && env[0]) ? 1 : 0;
    }
    return cached != 0;
}

void pp_debug_fail(const char* label, const Token* tok) {
    if (!pp_debug_fail_enabled()) return;
    const char* file = tok && tok->location.start.file ? tok->location.start.file : "<unknown>";
    int line = tok ? tok->location.start.line : 0;
    int col = tok ? tok->location.start.column : 0;
    fprintf(stderr, "[PP-DEBUG] failure at %s %s:%d:%d (tok type=%d)\n",
            label ? label : "<unknown>", file, line, col, tok ? tok->type : -1);
}

const char* token_file(const Token* tok) {
    if (!tok || !tok->location.start.file) return NULL;
    return tok->location.start.file;
}

static bool pp_record_logical_file(Preprocessor* pp, const char* path, const char** outPtr) {
    if (!pp || !path) return false;
    if (pp->logicalFileCount == pp->logicalFileCap) {
        size_t newCap = pp->logicalFileCap ? pp->logicalFileCap * 2 : 4;
        char** files = realloc(pp->logicalFilePool, newCap * sizeof(char*));
        if (!files) return false;
        pp->logicalFilePool = files;
        pp->logicalFileCap = newCap;
    }
    char* copy = pp_strdup_local(path);
    if (!copy) return false;
    pp->logicalFilePool[pp->logicalFileCount++] = copy;
    if (outPtr) *outPtr = copy;
    return true;
}

bool pp_set_base_file(Preprocessor* pp, const char* path) {
    if (!pp) return false;
    if (pp->baseFile) return true;
    pp->baseFile = path ? pp_strdup_local(path) : NULL;
    return path ? pp->baseFile != NULL : true;
}

bool pp_set_logical_file(Preprocessor* pp, const char* path) {
    if (!pp || !path) return false;
    const char* stored = NULL;
    if (!pp_record_logical_file(pp, path, &stored)) {
        return false;
    }
    pp->logicalFile = (char*)stored;
    return true;
}

bool pp_append_number_literal(Preprocessor* pp,
                              PPTokenBuffer* buffer,
                              const Token* base,
                              const char* text) {
    Token clone = pp_token_clone_remap(pp, base);
    clone.type = TOKEN_NUMBER;
    if (clone.value) free(clone.value);
    clone.value = text ? pp_strdup_local(text) : NULL;
    bool ok = pp_token_buffer_append_token(buffer, clone);
    if (!ok) {
        pp_token_free(&clone);
    }
    return ok;
}

bool pp_prepare_expr_tokens(Preprocessor* pp,
                            const Token* tokens,
                            size_t count,
                            PPTokenBuffer* out) {
    pp_token_buffer_init_local(out);
    if (!tokens || count == 0) return true;

    size_t i = 0;
    while (i < count) {
        const Token* tok = &tokens[i];
        // Preserve defined operand without expanding it.
        if (tok->type == TOKEN_IDENTIFIER && tok->value &&
            strcmp(tok->value, "defined") == 0) {
            if (!append_expr_token(pp, out, tok)) return false;
            i++;
            if (i < count && tokens[i].type == TOKEN_LPAREN) {
                if (!append_expr_token(pp, out, &tokens[i])) return false;
                i++;
            }
            if (i < count) {
                if (!append_expr_token(pp, out, &tokens[i])) return false;
                i++;
            }
            if (i < count && tokens[i].type == TOKEN_RPAREN) {
                if (!append_expr_token(pp, out, &tokens[i])) return false;
                i++;
            }
            continue;
        }

        // Collect a span until the next defined and expand macros inside it.
        size_t start = i;
        while (i < count) {
            const Token* t = &tokens[i];
            if (t->type == TOKEN_IDENTIFIER && t->value &&
                strcmp(t->value, "defined") == 0) {
                break;
            }
            if (t->type == TOKEN_EOF) {
                break;
            }
            i++;
        }
        size_t span = i - start;
        if (span > 0) {
            PPTokenBuffer expanded = {0};
            if (!macro_expander_expand(&pp->expander, tokens + start, span, &expanded)) {
                pp_token_buffer_destroy(&expanded);
                return false;
            }
            for (size_t j = 0; j < expanded.count; ++j) {
                if (!append_expr_token(pp, out, &expanded.tokens[j])) {
                    pp_token_buffer_destroy(&expanded);
                    return false;
                }
            }
            pp_token_buffer_destroy(&expanded);
        }
    }
    const char* debugExpr = getenv("DEBUG_PP_EXPR");
    if (debugExpr && debugExpr[0] != '\0') {
        fprintf(stderr, "PP expr tokens (%zu):\n", out->count);
        for (size_t k = 0; k < out->count; ++k) {
            const Token* t = &out->tokens[k];
            fprintf(stderr, "  [%zu] type=%d val=%s\n",
                    k,
                    t->type,
                    t->value ? t->value : "<null>");
        }
    }
    return true;
}

bool pp_include_stack_push(Preprocessor* pp,
                           const char* path,
                           IncludeSearchOrigin origin,
                           size_t originIndex) {
    if (!pp || !path) return false;
    if (pp->includeStack.depth == pp->includeStack.capacity) {
        size_t newCap = pp->includeStack.capacity ? pp->includeStack.capacity * 2 : 4;
        PPIncludeFrame* frames = realloc(pp->includeStack.frames, newCap * sizeof(PPIncludeFrame));
        if (!frames) return false;
        pp->includeStack.frames = frames;
        pp->includeStack.capacity = newCap;
    }
    pp->includeStack.frames[pp->includeStack.depth].path = path;
    pp->includeStack.frames[pp->includeStack.depth].origin = origin;
    pp->includeStack.frames[pp->includeStack.depth].originIndex = originIndex;
    pp->includeStack.depth++;
    return true;
}

void pp_include_stack_pop(Preprocessor* pp) {
    if (!pp || pp->includeStack.depth == 0) return;
    pp->includeStack.depth--;
}

const PPIncludeFrame* pp_include_stack_top(const Preprocessor* pp) {
    if (!pp || pp->includeStack.depth == 0) return NULL;
    return &pp->includeStack.frames[pp->includeStack.depth - 1];
}

bool pp_include_stack_contains(const Preprocessor* pp, const char* path) {
    if (!pp || !path) return false;
    for (size_t i = 0; i < pp->includeStack.depth; ++i) {
        if (pp->includeStack.frames[i].path &&
            strcmp(pp->includeStack.frames[i].path, path) == 0) {
            return true;
        }
    }
    return false;
}

void pp_report_diag(Preprocessor* pp,
                    const Token* tok,
                    DiagKind kind,
                    int code,
                    const char* fmt,
                    ...) {
    if (!pp || !pp->ctx || !fmt) return;
    SourceRange loc = tok ? tok->location : (SourceRange){0};
    va_list args;
    va_start(args, fmt);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    compiler_report_diag(pp->ctx,
                         loc,
                         kind,
                         code,
                         NULL,
                         "%s",
                         buffer);
}
