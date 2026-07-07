// SPDX-License-Identifier: Apache-2.0

#include "Compiler/pipeline_artifacts_units.h"

#include <stdlib.h>
#include <string.h>

#include "AST/ast_node.h"
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

static const char* symbol_file_path(const Symbol* sym) {
    if (!sym || !sym->definition) return NULL;
    return sym->definition->location.start.file;
}

static const FisicsSymbol* find_exported_symbol_match(const Symbol* sym,
                                                      const FisicsSymbol* symbols,
                                                      size_t symbol_count) {
    if (!sym || !sym->name || !symbols || symbol_count == 0) return NULL;
    const FisicsSymbolKind want_kind = map_definition_kind(sym);
    const char* want_path = symbol_file_path(sym);

    for (size_t i = 0; i < symbol_count; ++i) {
        const FisicsSymbol* cand = &symbols[i];
        if (!cand->name || strcmp(cand->name, sym->name) != 0) continue;
        if (cand->kind != want_kind) continue;
        if (want_path && cand->file_path && strcmp(cand->file_path, want_path) != 0) continue;
        return cand;
    }
    return NULL;
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
    if (symbol && symbol->stable_id != 0) {
        dst->symbol_stable_id = symbol->stable_id;
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
                                             SourceRange* range,
                                             const SourceRange* fallback_range) {
    if (!range) return;
    const char* fallback_file = NULL;
    if (fallback_range && fallback_range->start.file && fallback_range->start.file[0]) {
        fallback_file = fallback_range->start.file;
    } else if (buf && buf->fallback_file_path && buf->fallback_file_path[0]) {
        fallback_file = buf->fallback_file_path;
    }
    if (!fallback_file) return;

    if (!range->start.file || !range->start.file[0] ||
        strcmp(range->start.file, fallback_file) != 0) {
        range->start.file = fallback_file;
    }
    if (!range->end.file || !range->end.file[0] ||
        strcmp(range->end.file, fallback_file) != 0) {
        range->end.file = fallback_file;
    }
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
    if (!exported || exported->stable_id == 0) return;

    SourceRange range = sym->definition ? sym->definition->location : (SourceRange){0};
    if (!range.start.file || !range.start.file[0]) range.start.file = buf->fallback_file_path;
    if (!range.end.file || !range.end.file[0]) range.end.file = range.start.file;
    normalize_units_attachment_range(buf, &range, sym->definition ? &sym->definition->location : NULL);
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
        if (!range.start.file || !range.start.file[0]) range.start.file = node->location.start.file;
        if (!range.start.file || !range.start.file[0]) range.start.file = buf->fallback_file_path;
        if (!range.end.file || !range.end.file[0]) range.end.file = range.start.file;
        normalize_units_attachment_range(buf, &range, &node->location);
        const FisicsSymbol* exported = NULL;
        if (buf->symbols && buf->symbol_count > 0) {
            Symbol transient = {0};
            transient.name = (char*)name;
            transient.kind = SYMBOL_VARIABLE;
            transient.definition = node;
            exported = find_exported_symbol_match(&transient, buf->symbols, buf->symbol_count);
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
