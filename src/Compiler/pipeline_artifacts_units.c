// SPDX-License-Identifier: Apache-2.0

#include "Compiler/pipeline_artifacts_units.h"

#include <stdlib.h>
#include <string.h>

#include "AST/ast_node.h"
#include "Extensions/extension_hooks.h"

typedef struct {
    FisicsUnitsAttachment* items;
    size_t count;
    size_t capacity;
    const FisicsSymbol* symbols;
    size_t symbol_count;
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
                                    const FisicsUnitsAnnotation* ann) {
    if (!buf || !symbol || !ann) return false;
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
    dst->symbol_stable_id = symbol->stable_id;
    dst->symbol_name = dup_cstr(symbol->name);
    if (!dst->symbol_name) return false;

    const char* dim_text = ann->canonicalText ? ann->canonicalText : ann->exprText;
    dst->dim_text = dup_cstr(dim_text);
    if (!dst->dim_text) {
        free((void*)dst->symbol_name);
        dst->symbol_name = NULL;
        return false;
    }

    memcpy(dst->dim, ann->dim.e, sizeof(dst->dim));
    dst->resolved = ann->resolved;
    buf->count++;
    return true;
}

static void free_units_attachment_buffer(UnitsAttachmentBuffer* buf) {
    if (!buf || !buf->items) return;
    for (size_t i = 0; i < buf->count; ++i) {
        free((void*)buf->items[i].symbol_name);
        free((void*)buf->items[i].dim_text);
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

    if (!append_units_attachment(buf, exported, ann)) {
        buf->ok = false;
    }
}

bool pipeline_collect_units_attachments(const SemanticModel* model,
                                        const FisicsSymbol* symbols,
                                        size_t symbol_count,
                                        FisicsUnitsAttachment** out_attachments,
                                        size_t* out_count) {
    if (out_attachments) *out_attachments = NULL;
    if (out_count) *out_count = 0;
    if (!model || !out_attachments || !out_count || !symbols || symbol_count == 0) {
        return true;
    }

    UnitsAttachmentBuffer buf = {
        .symbols = symbols,
        .symbol_count = symbol_count,
        .ok = true
    };
    semanticModelForEachGlobal(model, collect_units_symbol_cb, &buf);
    if (!buf.ok) {
        free_units_attachment_buffer(&buf);
        return false;
    }

    *out_attachments = buf.items;
    *out_count = buf.count;
    return true;
}
