#include "Compiler/diagnostics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Compiler/compiler_context.h"
#include "Utils/logging.h"

static CompilerDiagnostics* ctx_buffer(struct CompilerContext* ctx) {
    return ctx ? &ctx->diags : NULL;
}

static const CompilerDiagnostics* ctx_buffer_const(const struct CompilerContext* ctx) {
    return ctx ? &ctx->diags : NULL;
}

static int range_length(SourceRange range) {
    if (range.start.line == range.end.line &&
        range.start.line > 0 &&
        range.start.column > 0 &&
        range.end.column > 0 &&
        range.end.column >= range.start.column) {
        int len = range.end.column - range.start.column;
        return len > 0 ? len : 1;
    }
    return 1;
}

static bool diag_buffer_reserve(CompilerDiagnostics* buf, size_t extra) {
    if (!buf) return false;
    size_t need = buf->count + extra;
    if (need <= buf->capacity) return true;
    size_t newCap = buf->capacity ? buf->capacity * 2 : 8;
    while (newCap < need) {
        newCap *= 2;
    }
    FisicsDiagnostic* grown = (FisicsDiagnostic*)realloc(buf->items, newCap * sizeof(FisicsDiagnostic));
    if (!grown) return false;
    buf->items = grown;
    buf->capacity = newCap;
    return true;
}

static char* fmt_message(const char* fmt, va_list args) {
    va_list copy;
    va_copy(copy, args);
    int needed = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (needed < 0) return NULL;
    size_t bytes = (size_t)needed + 1;
    char* out = (char*)malloc(bytes);
    if (!out) return NULL;
    vsnprintf(out, bytes, fmt, args);
    return out;
}

bool compiler_report_diag(struct CompilerContext* ctx,
                          SourceRange loc,
                          DiagKind kind,
                          int code,
                          const char* hint,
                          const char* fmt,
                          ...) {
    const char* debugEnv = getenv("FISICS_DEBUG_LAYOUT");
    bool debugLayout = debugEnv && debugEnv[0] && debugEnv[0] != '0';
    const char* dl_before = ctx ? ctx->dataLayout : NULL;
    uint64_t canary_front = ctx ? ctx->dl_canary_front : 0;
    uint64_t canary_back  = ctx ? ctx->dl_canary_back  : 0;
    CompilerDiagnostics* buf = ctx_buffer(ctx);
    if (!buf || !fmt) return false;
    if (!diag_buffer_reserve(buf, 1)) {
        return false;
    }

    va_list args;
    va_start(args, fmt);
    char* message = fmt_message(fmt, args);
    va_end(args);
    if (!message) return false;
    char* hintCopy = NULL;
    if (hint && hint[0]) {
        hintCopy = strdup(hint);
        if (!hintCopy) {
            free(message);
            return false;
        }
    }

    FisicsDiagnostic* d = &buf->items[buf->count++];
    if (loc.start.file) {
        d->file_path = strdup(loc.start.file);
        if (!d->file_path) {
            free(message);
            free(hintCopy);
            return false;
        }
    } else {
        d->file_path = NULL;
    }
    d->line = loc.start.line;
    d->column = loc.start.column;
    d->length = range_length(loc);
    d->kind = kind;
    d->code = code;
    d->message = message;
    d->hint = hintCopy;
    const char* dl_after = ctx ? ctx->dataLayout : NULL;
    if (debugLayout &&
        ctx &&
        (dl_before != dl_after ||
         canary_front != ctx->dl_canary_front ||
         canary_back  != ctx->dl_canary_back)) {
        LOG_WARN("codegen",
                 "[compiler_report_diag] dataLayout changed %p->%p code=%d kind=%d line=%d canaries %llx/%llx -> %llx/%llx",
                 (void*)dl_before,
                 (void*)dl_after,
                 code,
                 kind,
                 loc.start.line,
                 (unsigned long long)canary_front,
                 (unsigned long long)canary_back,
                 (unsigned long long)ctx->dl_canary_front,
                 (unsigned long long)ctx->dl_canary_back);
    }
    return true;
}

const FisicsDiagnostic* compiler_diagnostics_data(const struct CompilerContext* ctx, size_t* countOut) {
    const CompilerDiagnostics* buf = ctx_buffer_const(ctx);
    if (countOut) {
        *countOut = buf ? buf->count : 0;
    }
    return buf ? buf->items : NULL;
}

bool compiler_diagnostics_copy(const struct CompilerContext* ctx,
                               FisicsDiagnostic** outItems,
                               size_t* outCount) {
    if (outItems) *outItems = NULL;
    if (outCount) *outCount = 0;
    const CompilerDiagnostics* buf = ctx_buffer_const(ctx);
    if (!buf || buf->count == 0) {
        return true;
    }
    FisicsDiagnostic* copy = (FisicsDiagnostic*)calloc(buf->count, sizeof(FisicsDiagnostic));
    if (!copy) return false;
    for (size_t i = 0; i < buf->count; ++i) {
        copy[i] = buf->items[i];
        if (buf->items[i].message) {
            copy[i].message = strdup(buf->items[i].message);
            if (!copy[i].message) {
                for (size_t j = 0; j < i; ++j) {
                    free(copy[j].message);
                }
                free(copy);
                return false;
            }
        }
        if (buf->items[i].hint) {
            copy[i].hint = strdup(buf->items[i].hint);
            if (!copy[i].hint) {
                for (size_t j = 0; j <= i; ++j) {
                    free(copy[j].message);
                    free(copy[j].hint);
                }
                free(copy);
                return false;
            }
        }
    }
    if (outItems) *outItems = copy;
    if (outCount) *outCount = buf->count;
    return true;
}

void compiler_diagnostics_clear(struct CompilerContext* ctx) {
    CompilerDiagnostics* buf = ctx_buffer(ctx);
    if (!buf) return;
    for (size_t i = 0; i < buf->count; ++i) {
        free((void*)buf->items[i].file_path);
        free(buf->items[i].message);
        buf->items[i].message = NULL;
        free(buf->items[i].hint);
        buf->items[i].hint = NULL;
        buf->items[i].file_path = NULL;
    }
    free(buf->items);
    buf->items = NULL;
    buf->count = 0;
    buf->capacity = 0;
}

static size_t count_errors_in_range(const CompilerDiagnostics* buf, int startCode, int endCode) {
    if (!buf) return 0;
    size_t errors = 0;
    for (size_t i = 0; i < buf->count; ++i) {
        const FisicsDiagnostic* d = &buf->items[i];
        if (d->kind != DIAG_ERROR) continue;
        if (startCode >= 0 && d->code < startCode) continue;
        if (endCode   >= 0 && d->code >= endCode) continue;
        errors++;
    }
    return errors;
}

size_t compiler_diagnostics_error_count(const struct CompilerContext* ctx) {
    return count_errors_in_range(ctx_buffer_const(ctx), -1, -1);
}

size_t compiler_diagnostics_parser_error_count(const struct CompilerContext* ctx) {
    return count_errors_in_range(ctx_buffer_const(ctx),
                                 CDIAG_PARSER_GENERIC,
                                 CDIAG_SEMANTIC_GENERIC);
}
