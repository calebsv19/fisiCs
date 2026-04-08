// SPDX-License-Identifier: Apache-2.0

#include "Compiler/diagnostics.h"

#include "core_io.h"
#include "core_pack.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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

static int map_diag_severity_id(DiagKind kind) {
    switch (kind) {
        case DIAG_WARNING: return FISICS_DIAG_SEVERITY_WARNING;
        case DIAG_NOTE: return FISICS_DIAG_SEVERITY_INFO;
        case DIAG_ERROR:
        default: return FISICS_DIAG_SEVERITY_ERROR;
    }
}

static int map_diag_category_id(int code) {
    if (code >= FISICS_DIAG_CODE_PREPROCESSOR_GENERIC) {
        return FISICS_DIAG_CATEGORY_PREPROCESSOR;
    }
    if (code >= FISICS_DIAG_CODE_SEMANTIC_GENERIC) {
        return FISICS_DIAG_CATEGORY_SEMANTIC;
    }
    if (code >= FISICS_DIAG_CODE_PARSER_GENERIC) {
        return FISICS_DIAG_CATEGORY_PARSER;
    }
    if (code >= FISICS_DIAG_CODE_GENERIC) {
        return FISICS_DIAG_CATEGORY_ANALYSIS;
    }
    return FISICS_DIAG_CATEGORY_UNKNOWN;
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

static bool str_eq(const char* a, const char* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    return strcmp(a, b) == 0;
}

static bool diag_matches(const FisicsDiagnostic* d,
                         SourceRange loc,
                         DiagKind kind,
                         int code,
                         const char* hint,
                         const char* message) {
    if (!d || d->kind != kind || d->code != code) return false;
    if (d->line != loc.start.line || d->column != loc.start.column) return false;
    if (!str_eq(d->file_path, loc.start.file)) return false;
    if (!str_eq(d->message, message)) return false;
    if (!str_eq(d->hint, hint)) return false;
    return true;
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

    for (size_t i = 0; i < buf->count; ++i) {
        if (diag_matches(&buf->items[i], loc, kind, code, hintCopy, message)) {
            free(message);
            free(hintCopy);
            return true;
        }
    }

    if (!diag_buffer_reserve(buf, 1)) {
        free(message);
        free(hintCopy);
        return false;
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
    d->severity_id = map_diag_severity_id(kind);
    d->category_id = map_diag_category_id(code);
    d->code_id = code;
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

CoreResult compiler_diagnostics_build_core_dataset(const struct CompilerContext* ctx,
                                                   CoreDataset* out_dataset) {
    const CompilerDiagnostics* buf = ctx_buffer_const(ctx);
    uint32_t n = 0u;
    uint32_t* line_col = NULL;
    uint32_t* column_col = NULL;
    uint32_t* length_col = NULL;
    uint32_t* kind_col = NULL;
    int64_t* code_col = NULL;
    bool* has_file_col = NULL;
    bool* has_message_col = NULL;
    bool* has_hint_col = NULL;
    uint32_t error_count = 0u;
    uint32_t warning_count = 0u;
    uint32_t note_count = 0u;
    CoreResult r;
    const char* col_names[] = {
        "line",
        "column",
        "length",
        "kind",
        "code",
        "has_file",
        "has_message",
        "has_hint"
    };
    const CoreTableColumnType col_types[] = {
        CORE_TABLE_COL_U32,
        CORE_TABLE_COL_U32,
        CORE_TABLE_COL_U32,
        CORE_TABLE_COL_U32,
        CORE_TABLE_COL_I64,
        CORE_TABLE_COL_BOOL,
        CORE_TABLE_COL_BOOL,
        CORE_TABLE_COL_BOOL
    };
    const void* col_data[] = {
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
    };

    if (!out_dataset) {
        CoreResult invalid = { CORE_ERR_INVALID_ARG, "invalid argument" };
        return invalid;
    }

    core_dataset_init(out_dataset);
    if (buf) {
        n = (uint32_t)buf->count;
    }

    if (n > 0u) {
        line_col = (uint32_t*)malloc(sizeof(uint32_t) * n);
        column_col = (uint32_t*)malloc(sizeof(uint32_t) * n);
        length_col = (uint32_t*)malloc(sizeof(uint32_t) * n);
        kind_col = (uint32_t*)malloc(sizeof(uint32_t) * n);
        code_col = (int64_t*)malloc(sizeof(int64_t) * n);
        has_file_col = (bool*)malloc(sizeof(bool) * n);
        has_message_col = (bool*)malloc(sizeof(bool) * n);
        has_hint_col = (bool*)malloc(sizeof(bool) * n);
        if (!line_col || !column_col || !length_col || !kind_col ||
            !code_col || !has_file_col || !has_message_col || !has_hint_col) {
            r.code = CORE_ERR_OUT_OF_MEMORY;
            r.message = "out of memory";
            goto fail;
        }
    }

    if (buf) {
        for (uint32_t i = 0u; i < n; ++i) {
            const FisicsDiagnostic* d = &buf->items[i];
            if (d->kind == DIAG_ERROR) error_count += 1u;
            else if (d->kind == DIAG_WARNING) warning_count += 1u;
            else if (d->kind == DIAG_NOTE) note_count += 1u;

            line_col[i] = (d->line > 0) ? (uint32_t)d->line : 0u;
            column_col[i] = (d->column > 0) ? (uint32_t)d->column : 0u;
            length_col[i] = (d->length > 0) ? (uint32_t)d->length : 0u;
            kind_col[i] = (uint32_t)d->kind;
            code_col[i] = (int64_t)d->code;
            has_file_col[i] = (d->file_path && d->file_path[0] != '\0');
            has_message_col[i] = (d->message && d->message[0] != '\0');
            has_hint_col[i] = (d->hint && d->hint[0] != '\0');
        }
    }

    r = core_dataset_add_metadata_string(out_dataset, "profile", "fisics_diagnostics_v1");
    if (r.code != CORE_OK) goto fail;
    r = core_dataset_add_metadata_i64(out_dataset, "schema_version", 1);
    if (r.code != CORE_OK) goto fail;
    r = core_dataset_add_metadata_i64(out_dataset, "diag_count", (int64_t)n);
    if (r.code != CORE_OK) goto fail;
    r = core_dataset_add_metadata_i64(out_dataset, "error_count", (int64_t)error_count);
    if (r.code != CORE_OK) goto fail;
    r = core_dataset_add_metadata_i64(out_dataset, "warning_count", (int64_t)warning_count);
    if (r.code != CORE_OK) goto fail;
    r = core_dataset_add_metadata_i64(out_dataset, "note_count", (int64_t)note_count);
    if (r.code != CORE_OK) goto fail;

    if (n > 0u) {
        col_data[0] = line_col;
        col_data[1] = column_col;
        col_data[2] = length_col;
        col_data[3] = kind_col;
        col_data[4] = code_col;
        col_data[5] = has_file_col;
        col_data[6] = has_message_col;
        col_data[7] = has_hint_col;
        r = core_dataset_add_table_typed(out_dataset,
                                         "diagnostics_v1",
                                         col_names,
                                         col_types,
                                         (uint32_t)(sizeof(col_names) / sizeof(col_names[0])),
                                         n,
                                         col_data);
        if (r.code != CORE_OK) goto fail;
    }

    free(line_col);
    free(column_col);
    free(length_col);
    free(kind_col);
    free(code_col);
    free(has_file_col);
    free(has_message_col);
    free(has_hint_col);
    return core_result_ok();

fail:
    free(line_col);
    free(column_col);
    free(length_col);
    free(kind_col);
    free(code_col);
    free(has_file_col);
    free(has_message_col);
    free(has_hint_col);
    core_dataset_free(out_dataset);
    return r;
}

typedef struct JsonBuilder {
    char* data;
    size_t len;
    size_t cap;
} JsonBuilder;

static bool json_builder_reserve(JsonBuilder* b, size_t extra) {
    size_t needed;
    size_t cap;
    char* grown;
    if (!b) return false;
    if (extra > SIZE_MAX - b->len - 1u) return false;
    needed = b->len + extra + 1u;
    if (needed <= b->cap) return true;
    cap = b->cap ? b->cap : 256u;
    while (cap < needed) {
        if (cap > SIZE_MAX / 2u) {
            cap = needed;
            break;
        }
        cap *= 2u;
    }
    grown = (char*)realloc(b->data, cap);
    if (!grown) return false;
    b->data = grown;
    b->cap = cap;
    return true;
}

static bool json_builder_append(JsonBuilder* b, const char* text) {
    size_t n;
    if (!b || !text) return false;
    n = strlen(text);
    if (!json_builder_reserve(b, n)) return false;
    memcpy(b->data + b->len, text, n);
    b->len += n;
    b->data[b->len] = '\0';
    return true;
}

static bool json_builder_appendf(JsonBuilder* b, const char* fmt, ...) {
    va_list args;
    va_list copy;
    int needed;
    if (!b || !fmt) return false;
    va_start(args, fmt);
    va_copy(copy, args);
    needed = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (needed < 0) {
        va_end(args);
        return false;
    }
    if (!json_builder_reserve(b, (size_t)needed)) {
        va_end(args);
        return false;
    }
    vsnprintf(b->data + b->len, (size_t)needed + 1u, fmt, args);
    b->len += (size_t)needed;
    va_end(args);
    return true;
}

static const CoreTableColumnTyped* find_typed_column(const CoreDataItem* item, const char* name) {
    uint32_t i;
    if (!item || !name || item->kind != CORE_DATA_TABLE_TYPED) return NULL;
    for (i = 0u; i < item->as.table_typed.column_count; ++i) {
        const CoreTableColumnTyped* col = &item->as.table_typed.columns[i];
        if (col->name && strcmp(col->name, name) == 0) return col;
    }
    return NULL;
}

static int64_t metadata_i64_or_default(const CoreDataset* dataset, const char* key, int64_t fallback) {
    const CoreMetadataItem* item = core_dataset_find_metadata(dataset, key);
    if (!item || item->type != CORE_META_I64) return fallback;
    return item->as.i64_value;
}

CoreResult compiler_diagnostics_write_core_dataset_json(const struct CompilerContext* ctx,
                                                        const char* out_path) {
    CoreDataset dataset;
    CoreResult r;
    const CoreDataItem* table_item = NULL;
    const CoreTableColumnTyped* line_col = NULL;
    const CoreTableColumnTyped* column_col = NULL;
    const CoreTableColumnTyped* length_col = NULL;
    const CoreTableColumnTyped* kind_col = NULL;
    const CoreTableColumnTyped* code_col = NULL;
    const CoreTableColumnTyped* has_file_col = NULL;
    const CoreTableColumnTyped* has_message_col = NULL;
    const CoreTableColumnTyped* has_hint_col = NULL;
    JsonBuilder jb = {0};
    uint32_t rows = 0u;

    if (!ctx || !out_path || out_path[0] == '\0') {
        CoreResult invalid = { CORE_ERR_INVALID_ARG, "invalid argument" };
        return invalid;
    }

    r = compiler_diagnostics_build_core_dataset(ctx, &dataset);
    if (r.code != CORE_OK) return r;

    table_item = core_dataset_find(&dataset, "diagnostics_v1");
    if (!table_item || table_item->kind != CORE_DATA_TABLE_TYPED) {
        rows = 0u;
    } else {
        rows = table_item->as.table_typed.row_count;
        line_col = find_typed_column(table_item, "line");
        column_col = find_typed_column(table_item, "column");
        length_col = find_typed_column(table_item, "length");
        kind_col = find_typed_column(table_item, "kind");
        code_col = find_typed_column(table_item, "code");
        has_file_col = find_typed_column(table_item, "has_file");
        has_message_col = find_typed_column(table_item, "has_message");
        has_hint_col = find_typed_column(table_item, "has_hint");
    }
    if (rows > 0u &&
        (!line_col || !column_col || !length_col || !kind_col || !code_col ||
         !has_file_col || !has_message_col || !has_hint_col)) {
        core_dataset_free(&dataset);
        r.code = CORE_ERR_FORMAT;
        r.message = "diagnostics table schema mismatch";
        return r;
    }

    if (!json_builder_append(&jb, "{")) goto oom;
    if (!json_builder_appendf(&jb, "\"profile\":\"fisics_diagnostics_v1\",\"schema_version\":%lld,",
                              (long long)metadata_i64_or_default(&dataset, "schema_version", 1))) goto oom;
    if (!json_builder_appendf(&jb, "\"diag_count\":%lld,", (long long)metadata_i64_or_default(&dataset, "diag_count", (int64_t)rows))) goto oom;
    if (!json_builder_appendf(&jb, "\"error_count\":%lld,", (long long)metadata_i64_or_default(&dataset, "error_count", 0))) goto oom;
    if (!json_builder_appendf(&jb, "\"warning_count\":%lld,", (long long)metadata_i64_or_default(&dataset, "warning_count", 0))) goto oom;
    if (!json_builder_appendf(&jb, "\"note_count\":%lld,", (long long)metadata_i64_or_default(&dataset, "note_count", 0))) goto oom;
    if (!json_builder_append(&jb, "\"diagnostics\":[")) goto oom;

    for (uint32_t i = 0u; i < rows; ++i) {
        if (i > 0u && !json_builder_append(&jb, ",")) goto oom;
        if (!json_builder_appendf(&jb,
                                  "{\"line\":%u,\"column\":%u,\"length\":%u,\"kind\":%u,\"code\":%lld,"
                                  "\"has_file\":%s,\"has_message\":%s,\"has_hint\":%s}",
                                  line_col->as.u32_values[i],
                                  column_col->as.u32_values[i],
                                  length_col->as.u32_values[i],
                                  kind_col->as.u32_values[i],
                                  (long long)code_col->as.i64_values[i],
                                  has_file_col->as.bool_values[i] ? "true" : "false",
                                  has_message_col->as.bool_values[i] ? "true" : "false",
                                  has_hint_col->as.bool_values[i] ? "true" : "false")) goto oom;
    }

    if (!json_builder_append(&jb, "]}\n")) goto oom;
    r = core_io_write_all(out_path, jb.data, jb.len);
    free(jb.data);
    core_dataset_free(&dataset);
    return r;

oom:
    free(jb.data);
    core_dataset_free(&dataset);
    r.code = CORE_ERR_OUT_OF_MEMORY;
    r.message = "out of memory";
    return r;
}

typedef struct FisicsDiagPackHeaderV1 {
    uint32_t schema_version;
    uint32_t diag_count;
    uint32_t error_count;
    uint32_t warning_count;
    uint32_t note_count;
} FisicsDiagPackHeaderV1;

typedef struct FisicsDiagPackRowV1 {
    uint32_t line;
    uint32_t column;
    uint32_t length;
    uint32_t kind;
    int64_t code;
    uint32_t flags;
} FisicsDiagPackRowV1;

CoreResult compiler_diagnostics_write_core_dataset_pack(const struct CompilerContext* ctx,
                                                        const char* out_path) {
    CoreDataset dataset;
    CoreResult r;
    const CoreDataItem* table_item = NULL;
    const CoreTableColumnTyped* line_col = NULL;
    const CoreTableColumnTyped* column_col = NULL;
    const CoreTableColumnTyped* length_col = NULL;
    const CoreTableColumnTyped* kind_col = NULL;
    const CoreTableColumnTyped* code_col = NULL;
    const CoreTableColumnTyped* has_file_col = NULL;
    const CoreTableColumnTyped* has_message_col = NULL;
    const CoreTableColumnTyped* has_hint_col = NULL;
    FisicsDiagPackHeaderV1 header = {0};
    FisicsDiagPackRowV1* rows = NULL;
    CorePackWriter writer = {0};
    JsonBuilder meta = {0};
    uint32_t row_count = 0u;

    if (!ctx || !out_path || out_path[0] == '\0') {
        CoreResult invalid = { CORE_ERR_INVALID_ARG, "invalid argument" };
        return invalid;
    }

    r = compiler_diagnostics_build_core_dataset(ctx, &dataset);
    if (r.code != CORE_OK) return r;

    table_item = core_dataset_find(&dataset, "diagnostics_v1");
    if (!table_item || table_item->kind != CORE_DATA_TABLE_TYPED) {
        row_count = 0u;
    } else {
        row_count = table_item->as.table_typed.row_count;
        line_col = find_typed_column(table_item, "line");
        column_col = find_typed_column(table_item, "column");
        length_col = find_typed_column(table_item, "length");
        kind_col = find_typed_column(table_item, "kind");
        code_col = find_typed_column(table_item, "code");
        has_file_col = find_typed_column(table_item, "has_file");
        has_message_col = find_typed_column(table_item, "has_message");
        has_hint_col = find_typed_column(table_item, "has_hint");
    }
    if (row_count > 0u &&
        (!line_col || !column_col || !length_col || !kind_col || !code_col ||
         !has_file_col || !has_message_col || !has_hint_col)) {
        r.code = CORE_ERR_FORMAT;
        r.message = "diagnostics table schema mismatch";
        goto fail;
    }

    if (row_count > 0u) {
        rows = (FisicsDiagPackRowV1*)malloc(sizeof(FisicsDiagPackRowV1) * row_count);
        if (!rows) {
            r.code = CORE_ERR_OUT_OF_MEMORY;
            r.message = "out of memory";
            goto fail;
        }
        for (uint32_t i = 0u; i < row_count; ++i) {
            uint32_t flags = 0u;
            if (has_file_col->as.bool_values[i]) flags |= 1u << 0;
            if (has_message_col->as.bool_values[i]) flags |= 1u << 1;
            if (has_hint_col->as.bool_values[i]) flags |= 1u << 2;
            rows[i].line = line_col->as.u32_values[i];
            rows[i].column = column_col->as.u32_values[i];
            rows[i].length = length_col->as.u32_values[i];
            rows[i].kind = kind_col->as.u32_values[i];
            rows[i].code = code_col->as.i64_values[i];
            rows[i].flags = flags;
        }
    }

    header.schema_version = (uint32_t)metadata_i64_or_default(&dataset, "schema_version", 1);
    header.diag_count = (uint32_t)metadata_i64_or_default(&dataset, "diag_count", (int64_t)row_count);
    header.error_count = (uint32_t)metadata_i64_or_default(&dataset, "error_count", 0);
    header.warning_count = (uint32_t)metadata_i64_or_default(&dataset, "warning_count", 0);
    header.note_count = (uint32_t)metadata_i64_or_default(&dataset, "note_count", 0);

    if (!json_builder_appendf(&meta,
                              "{\"profile\":\"fisics_diagnostics_v1\",\"schema_version\":%u,"
                              "\"diag_count\":%u,\"error_count\":%u,\"warning_count\":%u,\"note_count\":%u}\n",
                              header.schema_version,
                              header.diag_count,
                              header.error_count,
                              header.warning_count,
                              header.note_count)) {
        r.code = CORE_ERR_OUT_OF_MEMORY;
        r.message = "out of memory";
        goto fail;
    }

    r = core_pack_writer_open(out_path, &writer);
    if (r.code != CORE_OK) goto fail;

    r = core_pack_writer_add_chunk(&writer, "FDHD", &header, (uint64_t)sizeof(header));
    if (r.code != CORE_OK) goto fail_close;

    r = core_pack_writer_add_chunk(&writer, "FDMJ", meta.data, (uint64_t)meta.len);
    if (r.code != CORE_OK) goto fail_close;

    r = core_pack_writer_add_chunk(&writer,
                                   "FDRW",
                                   rows,
                                   (uint64_t)sizeof(FisicsDiagPackRowV1) * (uint64_t)row_count);
    if (r.code != CORE_OK) goto fail_close;

    r = core_pack_writer_close(&writer);
    if (r.code != CORE_OK) goto fail;

    free(meta.data);
    free(rows);
    core_dataset_free(&dataset);
    return core_result_ok();

fail_close:
    (void)core_pack_writer_close(&writer);
fail:
    free(meta.data);
    free(rows);
    core_dataset_free(&dataset);
    return r;
}
