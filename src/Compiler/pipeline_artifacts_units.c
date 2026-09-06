// SPDX-License-Identifier: Apache-2.0

#include "Compiler/pipeline_artifacts_units.h"

#include <stdlib.h>
#include <string.h>

#include "AST/ast_node.h"
#include "Compiler/pipeline_artifacts_semantic_metadata.h"
#include "Extensions/extension_hooks.h"
#include "Extensions/extension_units_view.h"

typedef struct {
    FisicsUnitsAttachment* items;
    size_t count;
    size_t capacity;
    const FisicsSymbol* symbols;
    size_t symbol_count;
    const char* fallback_file_path;
    bool ok;
} UnitsAttachmentBuffer;

static char* dup_cstr(const char* text) {
    if (!text) return NULL;
    size_t len = strlen(text);
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, text, len + 1);
    return out;
}

static FisicsSymbolKind map_symbol_kind(SymbolKind kind) {
    switch (kind) {
        case SYMBOL_FUNCTION: return FISICS_SYMBOL_FUNCTION;
        case SYMBOL_STRUCT: return FISICS_SYMBOL_STRUCT;
        case SYMBOL_ENUM: return FISICS_SYMBOL_ENUM;
        case SYMBOL_TYPEDEF: return FISICS_SYMBOL_TYPEDEF;
        case SYMBOL_VARIABLE: return FISICS_SYMBOL_VARIABLE;
        default: return FISICS_SYMBOL_UNKNOWN;
    }
}

static FisicsSymbolKind map_definition_kind(const Symbol* sym) {
    if (!sym || !sym->definition) return map_symbol_kind(sym ? sym->kind : SYMBOL_VARIABLE);
    switch (sym->definition->type) {
        case AST_STRUCT_DEFINITION: return FISICS_SYMBOL_STRUCT;
        case AST_UNION_DEFINITION: return FISICS_SYMBOL_UNION;
        case AST_ENUM_DEFINITION: return FISICS_SYMBOL_ENUM;
        default: return map_symbol_kind(sym->kind);
    }
}

static const FisicsSymbol* find_exported_symbol_match(const Symbol* sym,
                                                      const FisicsSymbol* symbols,
                                                      size_t symbol_count) {
    if (!sym || !sym->name || !symbols || symbol_count == 0) return NULL;
    const FisicsSymbolKind want_kind = map_definition_kind(sym);

    for (size_t i = 0; i < symbol_count; ++i) {
        const FisicsSymbol* cand = &symbols[i];
        if (!cand->name || strcmp(cand->name, sym->name) != 0) continue;
        if (cand->kind != want_kind) continue;
        return cand;
    }
    return NULL;
}

static const FisicsSymbol* find_exported_variable_by_range(
    const char* name,
    SourceRange range,
    const FisicsSymbol* symbols,
    size_t symbol_count) {
    if (!name || !symbols) return NULL;
    for (size_t i = 0; i < symbol_count; ++i) {
        const FisicsSymbol* candidate = &symbols[i];
        if (candidate->kind != FISICS_SYMBOL_VARIABLE || !candidate->name ||
            strcmp(candidate->name, name) != 0) continue;
        if (candidate->start_line == range.start.line &&
            candidate->start_col == range.start.column &&
            candidate->end_line == range.end.line &&
            candidate->end_col == range.end.column) return candidate;
    }
    return NULL;
}

static uint64_t exported_stable_id(const FisicsSymbol* symbol) {
    if (!symbol) return 0;
    if (symbol->stable_id) return symbol->stable_id;
    FisicsSymbol normalized = *symbol;
    char* normalized_path =
        pipeline_normalize_semantic_metadata_source_path(symbol->file_path);
    if (normalized_path) normalized.file_path = normalized_path;
    uint64_t stable_id = pipeline_compute_exported_symbol_stable_id(&normalized);
    free(normalized_path);
    return stable_id;
}

static bool append_units_attachment(UnitsAttachmentBuffer* buf,
                                    const FisicsSymbol* symbol,
                                    const char* symbol_name,
                                    SourceRange range,
                                    const FisicsUnitsAnnotation* ann) {
    if (!buf || !symbol_name || !ann) return false;
    if (buf->count == buf->capacity) {
        size_t new_cap = buf->capacity ? buf->capacity * 2 : 4;
        FisicsUnitsAttachment* grown = (FisicsUnitsAttachment*)realloc(
            buf->items,
            new_cap * sizeof(FisicsUnitsAttachment));
        if (!grown) return false;
        buf->items = grown;
        buf->capacity = new_cap;
    }

    FisicsUnitsAttachment* dst = &buf->items[buf->count];
    memset(dst, 0, sizeof(*dst));
    uint64_t stable_id = exported_stable_id(symbol);
    if (stable_id != 0) {
        dst->symbol_stable_id = stable_id;
        dst->has_symbol_stable_id = true;
    }
    dst->symbol_name = dup_cstr(symbol_name);
    if (!dst->symbol_name) return false;
    dst->source_file_path = dup_cstr(range.start.file);
    dst->start_line = range.start.line;
    dst->start_col = range.start.column;
    dst->end_line = range.end.line;
    dst->end_col = range.end.column;
    if (range.start.file && !dst->source_file_path) {
        free((void*)dst->symbol_name);
        dst->symbol_name = NULL;
        return false;
    }

    const char* dim_text = ann->canonicalText ? ann->canonicalText : ann->dimExprText;
    dst->dim_text = dup_cstr(dim_text);
    if (!dst->dim_text) {
        free((void*)dst->symbol_name);
        free((void*)dst->source_file_path);
        dst->symbol_name = NULL;
        dst->source_file_path = NULL;
        return false;
    }

    memcpy(dst->dim, ann->dim.e, sizeof(dst->dim));
    dst->resolved = ann->resolved;
    if (ann->unitDef && ann->unitResolved) {
        dst->unit_source_text = dup_cstr(ann->unitExprText ? ann->unitExprText : ann->unitDef->name);
        dst->unit_name = dup_cstr(ann->unitDef->name);
        dst->unit_symbol = dup_cstr(ann->unitDef->symbol);
        dst->unit_family = dup_cstr(fisics_dim_family_name(ann->unitDef->family));
        if (!dst->unit_source_text || !dst->unit_name || !dst->unit_symbol || !dst->unit_family) {
            free((void*)dst->symbol_name);
            free((void*)dst->source_file_path);
            free((void*)dst->dim_text);
            free((void*)dst->unit_source_text);
            free((void*)dst->unit_name);
            free((void*)dst->unit_symbol);
            free((void*)dst->unit_family);
            memset(dst, 0, sizeof(*dst));
            return false;
        }
        dst->unit_resolved = true;
    }
    buf->count++;
    return true;
}

static void normalize_units_attachment_range(UnitsAttachmentBuffer* buf,
                                             SourceRange* range) {
    if (!range) return;
    const char* fallback_file =
        buf && buf->fallback_file_path && buf->fallback_file_path[0]
            ? buf->fallback_file_path : NULL;
    if (!fallback_file) return;

    range->start.file = fallback_file;
    range->end.file = fallback_file;
}

static void free_units_attachment_buffer(UnitsAttachmentBuffer* buf) {
    if (!buf || !buf->items) return;
    for (size_t i = 0; i < buf->count; ++i) {
        free((void*)buf->items[i].symbol_name);
        free((void*)buf->items[i].source_file_path);
        free((void*)buf->items[i].dim_text);
        free((void*)buf->items[i].unit_source_text);
        free((void*)buf->items[i].unit_name);
        free((void*)buf->items[i].unit_symbol);
        free((void*)buf->items[i].unit_family);
    }
    free(buf->items);
    buf->items = NULL;
    buf->count = 0;
    buf->capacity = 0;
}

static void collect_units_symbol_cb(const Symbol* sym, void* user_data) {
    UnitsAttachmentBuffer* buf = (UnitsAttachmentBuffer*)user_data;
    if (!buf || !buf->ok || !sym) return;

    const FisicsUnitsAnnotation* ann = symbolGetUnitsAnnotation(sym);
    if (!ann || !ann->resolved) return;

    const FisicsSymbol* exported = find_exported_symbol_match(sym, buf->symbols, buf->symbol_count);
    if (!exported) return;

    SourceRange range = sym->definition ? sym->definition->location : (SourceRange){0};
    normalize_units_attachment_range(buf, &range);
    if (!append_units_attachment(buf, exported, exported->name, range, ann)) {
        buf->ok = false;
    }
}

static void collect_units_annotation_cb(const FisicsUnitsAnnotation* ann, void* user_data) {
    UnitsAttachmentBuffer* buf = (UnitsAttachmentBuffer*)user_data;
    if (!buf || !buf->ok || !ann || !ann->node || !ann->resolved) return;
    ASTNode* node = ann->node;
    if (node->type != AST_VARIABLE_DECLARATION) return;

    for (size_t i = 0; i < node->varDecl.varCount; ++i) {
        ASTNode* ident = node->varDecl.varNames ? node->varDecl.varNames[i] : NULL;
        const char* name = (ident && ident->type == AST_IDENTIFIER) ? ident->valueNode.value : NULL;
        if (!name || !name[0]) continue;

        SourceRange range = ident ? ident->location : node->location;
        normalize_units_attachment_range(buf, &range);
        const FisicsSymbol* exported = NULL;
        if (buf->symbols && buf->symbol_count > 0) {
            exported = find_exported_variable_by_range(
                name, range, buf->symbols, buf->symbol_count);
        }
        if (!append_units_attachment(buf, exported, name, range, ann)) {
            buf->ok = false;
            return;
        }
    }
}

bool pipeline_collect_units_attachments(const SemanticModel* model,
                                        const FisicsSymbol* symbols,
                                        size_t symbol_count,
                                        const char* fallback_file_path,
                                        FisicsUnitsAttachment** out_attachments,
                                        size_t* out_count) {
    if (out_attachments) *out_attachments = NULL;
    if (out_count) *out_count = 0;
    if (!model || !out_attachments || !out_count) {
        return true;
    }

    UnitsAttachmentBuffer buf = {
        .symbols = symbols,
        .symbol_count = symbol_count,
        .fallback_file_path = fallback_file_path,
        .ok = true
    };
    CompilerContext* ctx = semanticModelGetContext(model);
    if (!buf.fallback_file_path) {
        buf.fallback_file_path = cc_get_input_path(ctx);
    }
    if (ctx && ctx->extensionState) {
        fisics_extension_for_each_units_annotation(ctx, collect_units_annotation_cb, &buf);
    } else if (symbols && symbol_count > 0) {
        semanticModelForEachGlobal(model, collect_units_symbol_cb, &buf);
    }
    if (!buf.ok) {
        free_units_attachment_buffer(&buf);
        return false;
    }

    *out_attachments = buf.items;
    *out_count = buf.count;
    return true;
}
