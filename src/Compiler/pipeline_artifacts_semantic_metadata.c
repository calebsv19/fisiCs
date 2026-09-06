// SPDX-License-Identifier: Apache-2.0

#include "Compiler/pipeline_artifacts_semantic_metadata.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Extensions/extension_hooks.h"
#include "Syntax/semantic_model.h"

typedef struct {
    FisicsFrontendSemanticMetadataRecord record;
    const Symbol* internal_symbol;
} ProjectedRecord;

static char* dup_cstr(const char* text) {
    if (!text) return NULL;
    size_t len = strlen(text);
    char* copy = (char*)malloc(len + 1u);
    if (!copy) return NULL;
    memcpy(copy, text, len + 1u);
    return copy;
}

static char* normalized_path(const char* path, const char* fallback) {
    /* SM-5 records are translation-unit local. The caller's input identity is
     * authoritative; lexer/preprocessor source labels are only a fallback for
     * callers that do not supply it. */
    const char* selected = (fallback && fallback[0]) ? fallback : path;
    if (!selected || !selected[0]) return dup_cstr("<unknown>");

    char* resolved = realpath(selected, NULL);
    if (resolved) return resolved;
    if (selected[0] == '/') return dup_cstr(selected);

    char* cwd = getcwd(NULL, 0);
    if (!cwd) return dup_cstr(selected);
    size_t cwd_len = strlen(cwd);
    size_t selected_len = strlen(selected);
    char* joined = (char*)malloc(cwd_len + 1u + selected_len + 1u);
    if (!joined) {
        free(cwd);
        return NULL;
    }
    memcpy(joined, cwd, cwd_len);
    joined[cwd_len] = '/';
    memcpy(joined + cwd_len + 1u, selected, selected_len + 1u);
    free(cwd);
    return joined;
}

static uint64_t stable_id_mix_u64(uint64_t hash, uint64_t value) {
    const uint64_t prime = 1099511628211ULL;
    for (int i = 0; i < 8; ++i) {
        hash ^= (uint8_t)((value >> (i * 8)) & 0xffu);
        hash *= prime;
    }
    return hash;
}

static uint64_t stable_id_mix_string(uint64_t hash, const char* value) {
    const uint64_t prime = 1099511628211ULL;
    const char* text = value ? value : "";
    size_t length = strlen(text);
    hash = stable_id_mix_u64(hash, (uint64_t)length);
    for (size_t i = 0; i < length; ++i) {
        hash ^= (uint8_t)text[i];
        hash *= prime;
    }
    return hash;
}

uint64_t pipeline_compute_exported_symbol_stable_id(const FisicsSymbol* symbol) {
    if (!symbol) return 0;
    uint64_t hash = 1469598103934665603ULL;
    hash = stable_id_mix_string(hash, symbol->name);
    hash = stable_id_mix_string(hash, symbol->file_path);
    hash = stable_id_mix_u64(hash, (uint64_t)symbol->kind);
    hash = stable_id_mix_string(hash, symbol->parent_name);
    hash = stable_id_mix_u64(hash, (uint64_t)symbol->parent_kind);
    hash = stable_id_mix_u64(hash, (uint64_t)symbol->start_line);
    hash = stable_id_mix_u64(hash, (uint64_t)symbol->start_col);
    hash = stable_id_mix_u64(hash, (uint64_t)symbol->end_line);
    hash = stable_id_mix_u64(hash, (uint64_t)symbol->end_col);
    hash = stable_id_mix_u64(hash, symbol->is_definition ? 1u : 0u);
    hash = stable_id_mix_u64(hash, symbol->is_variadic ? 1u : 0u);
    hash = stable_id_mix_string(hash, symbol->return_type);
    hash = stable_id_mix_u64(hash, (uint64_t)symbol->param_count);
    for (size_t i = 0; i < symbol->param_count; ++i) {
        hash = stable_id_mix_string(
            hash, symbol->param_types ? symbol->param_types[i] : NULL);
        hash = stable_id_mix_string(
            hash, symbol->param_names ? symbol->param_names[i] : NULL);
    }
    return hash;
}

char* pipeline_normalize_semantic_metadata_source_path(const char* path) {
    return normalized_path(path, NULL);
}

static FisicsFrontendSemanticMetadataSymbolKind public_symbol_kind(
    const FisicsSemanticMetadataRecord* record) {
    return record && record->isFunction
               ? FISICS_SEMANTIC_METADATA_SYMBOL_FUNCTION
               : FISICS_SEMANTIC_METADATA_SYMBOL_FILE_VARIABLE;
}

static FisicsSymbolKind exported_symbol_kind(
    FisicsFrontendSemanticMetadataSymbolKind kind) {
    return kind == FISICS_SEMANTIC_METADATA_SYMBOL_FUNCTION
               ? FISICS_SYMBOL_FUNCTION
               : FISICS_SYMBOL_VARIABLE;
}

static const FisicsSymbol* find_exported_symbol(
    const FisicsSemanticMetadataRecord* record,
    const char* normalized_source_path,
    const FisicsSymbol* symbols,
    size_t symbol_count) {
    if (!record || !record->symbolName || !symbols) return NULL;
    FisicsSymbolKind wanted_kind = exported_symbol_kind(public_symbol_kind(record));
    const FisicsSymbol* name_kind_match = NULL;
    for (size_t i = 0; i < symbol_count; ++i) {
        const FisicsSymbol* candidate = &symbols[i];
        if (!candidate->name || strcmp(candidate->name, record->symbolName) != 0 ||
            candidate->kind != wanted_kind) {
            continue;
        }
        if (!name_kind_match) name_kind_match = candidate;
        if (candidate->file_path && normalized_source_path &&
            strcmp(candidate->file_path, normalized_source_path) == 0) {
            return candidate;
        }
    }
    return name_kind_match;
}

static char* make_source_identity(
    const FisicsSemanticMetadataRecord* source,
    const FisicsFrontendSemanticMetadataRecord* projected) {
    if (source->authoredId && !source->idConflicted) {
        return dup_cstr(source->authoredId);
    }
    const char* kind = source->isFunction ? "function" : "file_variable";
    const char* path = projected->source_file_path
                           ? projected->source_file_path
                           : "<unknown>";
    const char* name = projected->symbol_name ? projected->symbol_name : "<unnamed>";
    int needed = snprintf(NULL, 0, "symbol:%s:%s:%d:%d:%s",
                          kind, path, projected->start_line,
                          projected->start_col, name);
    if (needed < 0) return NULL;
    char* identity = (char*)malloc((size_t)needed + 1u);
    if (!identity) return NULL;
    snprintf(identity, (size_t)needed + 1u, "symbol:%s:%s:%d:%d:%s",
             kind, path, projected->start_line, projected->start_col, name);
    return identity;
}

static void free_record(FisicsFrontendSemanticMetadataRecord* record) {
    if (!record) return;
    free((void*)record->source_identity);
    free((void*)record->authored_id);
    free((void*)record->symbol_name);
    free((void*)record->source_file_path);
    free((void*)record->domain);
    free((void*)record->role);
    memset(record, 0, sizeof(*record));
}

static void free_edge(FisicsFrontendSemanticMetadataEdge* edge) {
    if (!edge) return;
    free((void*)edge->target_id);
    free((void*)edge->source_file_path);
    memset(edge, 0, sizeof(*edge));
}

void pipeline_free_semantic_metadata_projection(
    FisicsFrontendSemanticMetadataRecord* records,
    size_t record_count,
    FisicsFrontendSemanticMetadataEdge* edges,
    size_t edge_count) {
    for (size_t i = 0; i < record_count; ++i) free_record(&records[i]);
    for (size_t i = 0; i < edge_count; ++i) free_edge(&edges[i]);
    free(records);
    free(edges);
}

static bool project_record(ProjectedRecord* destination,
                           const FisicsSemanticMetadataRecord* source,
                           const FisicsSymbol* symbols,
                           size_t symbol_count,
                           const char* fallback_file_path) {
    memset(destination, 0, sizeof(*destination));
    FisicsFrontendSemanticMetadataRecord* record = &destination->record;
    record->symbol_kind = public_symbol_kind(source);
    record->symbol_name = dup_cstr(source->symbolName ? source->symbolName : "");
    record->source_file_path = normalized_path(source->location.start.file,
                                               fallback_file_path);
    record->start_line = source->location.start.line;
    record->start_col = source->location.start.column;
    record->end_line = source->location.end.line > 0
                           ? source->location.end.line
                           : record->start_line;
    record->end_col = source->location.end.column > 0
                          ? source->location.end.column
                          : record->start_col;
    if (source->authoredId && !source->idConflicted) {
        record->authored_id = dup_cstr(source->authoredId);
    }
    if (source->domain && !source->domainConflicted) {
        record->domain = dup_cstr(source->domain);
    }
    if (source->role && !source->roleConflicted) {
        record->role = dup_cstr(source->role);
    }
    record->source_identity = make_source_identity(source, record);

    if (!record->symbol_name || !record->symbol_name[0] ||
        !record->source_file_path || !record->source_identity ||
        (source->authoredId && !source->idConflicted && !record->authored_id) ||
        (source->domain && !source->domainConflicted && !record->domain) ||
        (source->role && !source->roleConflicted && !record->role)) {
        free_record(record);
        return false;
    }

    const FisicsSymbol* exported = find_exported_symbol(
        source, record->source_file_path, symbols, symbol_count);
    if (exported) {
        FisicsSymbol normalized_symbol = *exported;
        char* normalized_symbol_path = normalized_path(exported->file_path, NULL);
        if (normalized_symbol_path) normalized_symbol.file_path = normalized_symbol_path;
        record->symbol_stable_id = exported->stable_id
                                       ? exported->stable_id
                                       : pipeline_compute_exported_symbol_stable_id(&normalized_symbol);
        free(normalized_symbol_path);
        record->has_symbol_stable_id = true;
    }
    destination->internal_symbol = source->symbol;
    return true;
}

static int compare_projected_records(const void* lhs, const void* rhs) {
    const ProjectedRecord* a = (const ProjectedRecord*)lhs;
    const ProjectedRecord* b = (const ProjectedRecord*)rhs;
    int cmp = strcmp(a->record.source_file_path, b->record.source_file_path);
    if (cmp != 0) return cmp;
    if (a->record.start_line != b->record.start_line) {
        return a->record.start_line < b->record.start_line ? -1 : 1;
    }
    if (a->record.start_col != b->record.start_col) {
        return a->record.start_col < b->record.start_col ? -1 : 1;
    }
    if (a->record.symbol_kind != b->record.symbol_kind) {
        return a->record.symbol_kind < b->record.symbol_kind ? -1 : 1;
    }
    cmp = strcmp(a->record.symbol_name, b->record.symbol_name);
    if (cmp != 0) return cmp;
    return strcmp(a->record.source_identity, b->record.source_identity);
}

static bool find_record_index(const ProjectedRecord* records,
                              size_t record_count,
                              const Symbol* symbol,
                              size_t* out_index) {
    for (size_t i = 0; i < record_count; ++i) {
        if (records[i].internal_symbol == symbol) {
            if (out_index) *out_index = i;
            return true;
        }
    }
    return false;
}

static bool find_authored_id_index(const ProjectedRecord* records,
                                   size_t record_count,
                                   const char* authored_id,
                                   size_t* out_index) {
    if (!authored_id) return false;
    for (size_t i = 0; i < record_count; ++i) {
        const char* candidate = records[i].record.authored_id;
        if (candidate && strcmp(candidate, authored_id) == 0) {
            if (out_index) *out_index = i;
            return true;
        }
    }
    return false;
}

static bool project_edge(FisicsFrontendSemanticMetadataEdge* destination,
                         const FisicsSemanticMetadataEdge* source,
                         const ProjectedRecord* records,
                         size_t record_count,
                         const char* fallback_file_path) {
    memset(destination, 0, sizeof(*destination));
    if (!find_record_index(records, record_count, source->sourceSymbol,
                           &destination->source_record_index)) {
        return false;
    }
    destination->kind = source->kind == FISICS_METADATA_EDGE_CONSUMES
                            ? FISICS_SEMANTIC_METADATA_EDGE_CONSUMES
                            : FISICS_SEMANTIC_METADATA_EDGE_PRODUCES;
    destination->target_id = dup_cstr(source->targetId);
    destination->resolution = source->resolution == FISICS_METADATA_REFERENCE_RESOLVED
                                  ? FISICS_SEMANTIC_METADATA_REFERENCE_RESOLVED
                                  : FISICS_SEMANTIC_METADATA_REFERENCE_EXTERNAL;
    destination->source_file_path = normalized_path(source->location.start.file,
                                                    fallback_file_path);
    destination->start_line = source->location.start.line;
    destination->start_col = source->location.start.column;
    destination->end_line = source->location.end.line > 0
                                ? source->location.end.line
                                : destination->start_line;
    destination->end_col = source->location.end.column > 0
                               ? source->location.end.column
                               : destination->start_col;
    if (!destination->target_id || !destination->source_file_path) {
        free_edge(destination);
        return false;
    }
    if (destination->resolution == FISICS_SEMANTIC_METADATA_REFERENCE_RESOLVED) {
        if (!find_authored_id_index(records, record_count, destination->target_id,
                                    &destination->target_record_index)) {
            free_edge(destination);
            return false;
        }
        destination->has_target_record_index = true;
    }
    return true;
}

static int compare_edges(const void* lhs, const void* rhs) {
    const FisicsFrontendSemanticMetadataEdge* a =
        (const FisicsFrontendSemanticMetadataEdge*)lhs;
    const FisicsFrontendSemanticMetadataEdge* b =
        (const FisicsFrontendSemanticMetadataEdge*)rhs;
    if (a->source_record_index != b->source_record_index) {
        return a->source_record_index < b->source_record_index ? -1 : 1;
    }
    if (a->kind != b->kind) return a->kind < b->kind ? -1 : 1;
    int cmp = strcmp(a->target_id, b->target_id);
    if (cmp != 0) return cmp;
    cmp = strcmp(a->source_file_path, b->source_file_path);
    if (cmp != 0) return cmp;
    if (a->start_line != b->start_line) return a->start_line < b->start_line ? -1 : 1;
    if (a->start_col != b->start_col) return a->start_col < b->start_col ? -1 : 1;
    if (a->end_line != b->end_line) return a->end_line < b->end_line ? -1 : 1;
    if (a->end_col != b->end_col) return a->end_col < b->end_col ? -1 : 1;
    return 0;
}

bool pipeline_collect_semantic_metadata_projection(
    const SemanticModel* model,
    const FisicsSymbol* symbols,
    size_t symbol_count,
    const char* fallback_file_path,
    FisicsFrontendSemanticMetadataRecord** out_records,
    size_t* out_record_count,
    FisicsFrontendSemanticMetadataEdge** out_edges,
    size_t* out_edge_count) {
    if (!out_records || !out_record_count || !out_edges || !out_edge_count) return false;
    *out_records = NULL;
    *out_record_count = 0;
    *out_edges = NULL;
    *out_edge_count = 0;
    const char* fail_projection =
        getenv("FISICS_TEST_FAIL_SEMANTIC_METADATA_PROJECTION_ALLOC");
    if (fail_projection && fail_projection[0] && fail_projection[0] != '0') {
        return false;
    }
    if (!model) return true;

    CompilerContext* ctx = semanticModelGetContext(model);
    FisicsExtensionState* state = ctx ? ctx->extensionState : NULL;
    if (!state || state->semanticMetadataRecordCount == 0) return true;

    size_t record_count = state->semanticMetadataRecordCount;
    ProjectedRecord* projected =
        (ProjectedRecord*)calloc(record_count, sizeof(*projected));
    if (!projected) return false;
    for (size_t i = 0; i < record_count; ++i) {
        if (!project_record(&projected[i], &state->semanticMetadataRecords[i],
                            symbols, symbol_count, fallback_file_path)) {
            for (size_t j = 0; j < record_count; ++j) free_record(&projected[j].record);
            free(projected);
            return false;
        }
    }
    qsort(projected, record_count, sizeof(*projected), compare_projected_records);

    FisicsFrontendSemanticMetadataRecord* records =
        (FisicsFrontendSemanticMetadataRecord*)calloc(record_count, sizeof(*records));
    if (!records) {
        for (size_t i = 0; i < record_count; ++i) free_record(&projected[i].record);
        free(projected);
        return false;
    }
    for (size_t i = 0; i < record_count; ++i) records[i] = projected[i].record;

    size_t edge_count = state->semanticMetadataEdgeCount;
    FisicsFrontendSemanticMetadataEdge* edges = NULL;
    if (edge_count > 0) {
        edges = (FisicsFrontendSemanticMetadataEdge*)calloc(edge_count, sizeof(*edges));
        if (!edges) {
            free(projected);
            pipeline_free_semantic_metadata_projection(records, record_count, NULL, 0);
            return false;
        }
        for (size_t i = 0; i < edge_count; ++i) {
            if (!project_edge(&edges[i], &state->semanticMetadataEdges[i],
                              projected, record_count, fallback_file_path)) {
                free(projected);
                pipeline_free_semantic_metadata_projection(records, record_count,
                                                            edges, edge_count);
                return false;
            }
        }
        qsort(edges, edge_count, sizeof(*edges), compare_edges);
    }

    free(projected);
    *out_records = records;
    *out_record_count = record_count;
    *out_edges = edges;
    *out_edge_count = edge_count;
    return true;
}
