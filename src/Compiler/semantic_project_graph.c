// SPDX-License-Identifier: Apache-2.0

#include "Compiler/semantic_project_graph.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Compiler/semantic_metadata_json_reader.h"

static void set_diagnostic(FisicsSemanticProjectDiagnostic* diagnostic,
                           const char* format,
                           ...) {
    if (!diagnostic || diagnostic->message[0]) return;
    va_list args;
    va_start(args, format);
    vsnprintf(diagnostic->message, sizeof(diagnostic->message), format, args);
    va_end(args);
}

static char* duplicate_string(const char* value) {
    if (!value) return NULL;
    size_t length = strlen(value);
    char* copy = (char*)malloc(length + 1u);
    if (copy) memcpy(copy, value, length + 1u);
    return copy;
}

static bool copy_record(FisicsFrontendSemanticMetadataRecord* out,
                        const FisicsFrontendSemanticMetadataRecord* in) {
    *out = *in;
    out->source_identity = duplicate_string(in->source_identity);
    out->authored_id = duplicate_string(in->authored_id);
    out->symbol_name = duplicate_string(in->symbol_name);
    out->source_file_path = duplicate_string(in->source_file_path);
    out->domain = duplicate_string(in->domain);
    out->role = duplicate_string(in->role);
    return out->source_identity && out->symbol_name && out->source_file_path &&
           (!in->authored_id || out->authored_id) && (!in->domain || out->domain) &&
           (!in->role || out->role);
}

static bool copy_edge(FisicsFrontendSemanticMetadataEdge* out,
                      const FisicsFrontendSemanticMetadataEdge* in) {
    *out = *in;
    out->target_id = duplicate_string(in->target_id);
    out->source_file_path = duplicate_string(in->source_file_path);
    return out->target_id && out->source_file_path;
}

static int compare_artifacts(const void* left, const void* right) {
    const FisicsSemanticMetadataArtifact* a = left;
    const FisicsSemanticMetadataArtifact* b = right;
    int result = strcmp(a->source_path, b->source_path);
    if (result) return result;
    if (a->source_hash != b->source_hash) return a->source_hash < b->source_hash ? -1 : 1;
    return strcmp(a->metadata_path, b->metadata_path);
}

static int compare_project_records(const void* left, const void* right) {
    const FisicsSemanticProjectRecord* a = left;
    const FisicsSemanticProjectRecord* b = right;
    int result = strcmp(a->record.source_file_path, b->record.source_file_path);
    if (result) return result;
    if (a->record.start_line != b->record.start_line)
        return a->record.start_line < b->record.start_line ? -1 : 1;
    if (a->record.start_col != b->record.start_col)
        return a->record.start_col < b->record.start_col ? -1 : 1;
    if (a->record.symbol_kind != b->record.symbol_kind)
        return a->record.symbol_kind < b->record.symbol_kind ? -1 : 1;
    result = strcmp(a->record.symbol_name, b->record.symbol_name);
    return result ? result : strcmp(a->record.source_identity, b->record.source_identity);
}

static int compare_project_edges(const void* left, const void* right) {
    const FisicsSemanticProjectEdge* a = left;
    const FisicsSemanticProjectEdge* b = right;
    if (a->edge.source_record_index != b->edge.source_record_index)
        return a->edge.source_record_index < b->edge.source_record_index ? -1 : 1;
    if (a->edge.kind != b->edge.kind) return a->edge.kind < b->edge.kind ? -1 : 1;
    int result = strcmp(a->edge.target_id, b->edge.target_id);
    if (result) return result;
    result = strcmp(a->edge.source_file_path, b->edge.source_file_path);
    if (result) return result;
    if (a->edge.start_line != b->edge.start_line)
        return a->edge.start_line < b->edge.start_line ? -1 : 1;
    if (a->edge.start_col != b->edge.start_col)
        return a->edge.start_col < b->edge.start_col ? -1 : 1;
    if (a->edge.end_line != b->edge.end_line)
        return a->edge.end_line < b->edge.end_line ? -1 : 1;
    if (a->edge.end_col != b->edge.end_col)
        return a->edge.end_col < b->edge.end_col ? -1 : 1;
    return 0;
}

static FisicsSemanticProjectRecord* find_origin_record(FisicsSemanticProjectGraph* graph,
                                                        size_t unit,
                                                        size_t local) {
    for (size_t i = 0; i < graph->record_count; ++i) {
        if (graph->records[i].translation_unit_index == unit &&
            graph->records[i].local_record_index == local) return &graph->records[i];
    }
    return NULL;
}

static int compare_source_identity_pointers(const void* left, const void* right) {
    const FisicsSemanticProjectRecord* const* a = left;
    const FisicsSemanticProjectRecord* const* b = right;
    return strcmp((*a)->record.source_identity, (*b)->record.source_identity);
}

static int compare_authored_identity_pointers(const void* left, const void* right) {
    const FisicsSemanticProjectRecord* const* a = left;
    const FisicsSemanticProjectRecord* const* b = right;
    return strcmp((*a)->record.authored_id, (*b)->record.authored_id);
}

static FisicsSemanticProjectRecord* find_authored_record(
    FisicsSemanticProjectRecord* const* index,
    size_t count,
    const char* authored_id) {
    size_t low = 0, high = count;
    while (low < high) {
        size_t middle = low + (high - low) / 2u;
        int comparison = strcmp(index[middle]->record.authored_id, authored_id);
        if (comparison == 0) return index[middle];
        if (comparison < 0) low = middle + 1u;
        else high = middle;
    }
    return NULL;
}

static bool external_allowed(const FisicsSemanticProjectGraph* graph, const char* target) {
    size_t low = 0, high = graph->allowed_external_target_count;
    while (low < high) {
        size_t middle = low + (high - low) / 2u;
        int comparison = strcmp(graph->allowed_external_targets[middle], target);
        if (comparison == 0) return true;
        if (comparison < 0) low = middle + 1u;
        else high = middle;
    }
    return false;
}

void fisics_semantic_project_graph_free(FisicsSemanticProjectGraph* graph) {
    if (!graph) return;
    free(graph->project_id);
    free(graph->manifest_path);
    for (size_t i = 0; i < graph->translation_unit_count; ++i) {
        free(graph->translation_units[i].source_path);
        free(graph->translation_units[i].metadata_path);
        free(graph->translation_units[i].producer_version);
        free(graph->translation_units[i].analysis_contract_version);
    }
    free(graph->translation_units);
    for (size_t i = 0; i < graph->allowed_external_target_count; ++i)
        free(graph->allowed_external_targets[i]);
    free(graph->allowed_external_targets);
    for (size_t i = 0; i < graph->record_count; ++i) {
        FisicsFrontendSemanticMetadataRecord* record = &graph->records[i].record;
        free((char*)record->source_identity); free((char*)record->authored_id);
        free((char*)record->symbol_name); free((char*)record->source_file_path);
        free((char*)record->domain); free((char*)record->role);
    }
    free(graph->records);
    for (size_t i = 0; i < graph->edge_count; ++i) {
        free((char*)graph->edges[i].edge.target_id);
        free((char*)graph->edges[i].edge.source_file_path);
    }
    free(graph->edges);
    memset(graph, 0, sizeof(*graph));
}

bool fisics_semantic_project_graph_build(
    const FisicsSemanticProjectManifest* manifest,
    FisicsSemanticProjectGraph* out,
    FisicsSemanticProjectDiagnostic* diagnostic) {
    if (diagnostic) diagnostic->message[0] = '\0';
    if (!manifest || !out) return false;
    memset(out, 0, sizeof(*out));
    const char* fail_allocation = getenv("FISICS_TEST_FAIL_SEMANTIC_PROJECT_GRAPH_ALLOC");
    if (fail_allocation && fail_allocation[0] && fail_allocation[0] != '0') {
        set_diagnostic(diagnostic, "injected semantic project graph allocation failure");
        return false;
    }
    FisicsSemanticMetadataArtifact* artifacts =
        (FisicsSemanticMetadataArtifact*)calloc(manifest->translation_unit_count,
                                                 sizeof(*artifacts));
    if (!artifacts) {
        set_diagnostic(diagnostic, "out of memory allocating semantic project inputs");
        return false;
    }
    FisicsSemanticProjectRecord** source_identity_index = NULL;
    FisicsSemanticProjectRecord** authored_identity_index = NULL;
    size_t authored_identity_count = 0;
    size_t loaded = 0, total_bytes = 0, total_records = 0, total_edges = 0;
    for (; loaded < manifest->translation_unit_count; ++loaded) {
        size_t artifact_bytes = 0;
        if (!fisics_semantic_metadata_artifact_load(
                manifest->translation_units[loaded].metadata_path,
                &artifacts[loaded], &artifact_bytes, diagnostic)) goto fail;
        if (artifact_bytes > FISICS_SEMANTIC_PROJECT_MAX_TOTAL_ARTIFACT_BYTES - total_bytes) {
            set_diagnostic(diagnostic, "semantic project artifact bytes exceed %u",
                           FISICS_SEMANTIC_PROJECT_MAX_TOTAL_ARTIFACT_BYTES);
            loaded++;
            goto fail;
        }
        total_bytes += artifact_bytes;
        if (artifacts[loaded].record_count > FISICS_SEMANTIC_PROJECT_MAX_RECORDS - total_records ||
            artifacts[loaded].edge_count > FISICS_SEMANTIC_PROJECT_MAX_EDGES - total_edges) {
            set_diagnostic(diagnostic, "semantic project record or edge limit exceeded");
            loaded++;
            goto fail;
        }
        total_records += artifacts[loaded].record_count;
        total_edges += artifacts[loaded].edge_count;
    }
    qsort(artifacts, loaded, sizeof(*artifacts), compare_artifacts);
    for (size_t i = 1; i < loaded; ++i) {
        if (strcmp(artifacts[i - 1u].source_path, artifacts[i].source_path) == 0) {
            set_diagnostic(diagnostic, "duplicate semantic project source path: %s",
                           artifacts[i].source_path);
            goto fail;
        }
    }
    out->project_id = duplicate_string(manifest->project_id);
    out->manifest_path = duplicate_string(manifest->manifest_path);
    out->translation_unit_count = loaded;
    out->translation_units = (FisicsSemanticProjectTranslationUnit*)calloc(
        loaded, sizeof(*out->translation_units));
    out->allowed_external_target_count = manifest->allowed_external_target_count;
    out->allowed_external_targets = (char**)calloc(
        out->allowed_external_target_count ? out->allowed_external_target_count : 1u,
        sizeof(*out->allowed_external_targets));
    out->record_count = total_records;
    out->records = (FisicsSemanticProjectRecord*)calloc(
        total_records ? total_records : 1u, sizeof(*out->records));
    out->edge_count = total_edges;
    out->edges = (FisicsSemanticProjectEdge*)calloc(
        total_edges ? total_edges : 1u, sizeof(*out->edges));
    if (!out->project_id || !out->manifest_path || !out->translation_units ||
        !out->allowed_external_targets || !out->records || !out->edges) {
        set_diagnostic(diagnostic, "out of memory allocating semantic project graph");
        goto fail;
    }
    for (size_t i = 0; i < out->allowed_external_target_count; ++i) {
        out->allowed_external_targets[i] =
            duplicate_string(manifest->allowed_external_targets[i]);
        if (!out->allowed_external_targets[i]) goto fail;
    }
    size_t record_position = 0, edge_position = 0;
    for (size_t unit = 0; unit < loaded; ++unit) {
        const FisicsSemanticMetadataArtifact* artifact = &artifacts[unit];
        FisicsSemanticProjectTranslationUnit* target_unit = &out->translation_units[unit];
        target_unit->translation_unit_index = unit;
        target_unit->source_path = duplicate_string(artifact->source_path);
        target_unit->source_hash = artifact->source_hash;
        target_unit->source_length = artifact->source_length;
        target_unit->metadata_path = duplicate_string(artifact->metadata_path);
        target_unit->producer_version = duplicate_string(artifact->producer_version);
        target_unit->analysis_contract_version =
            duplicate_string(artifact->analysis_contract_version);
        target_unit->record_count = artifact->record_count;
        target_unit->edge_count = artifact->edge_count;
        if (!target_unit->source_path || !target_unit->metadata_path ||
            !target_unit->producer_version || !target_unit->analysis_contract_version) goto fail;
        for (size_t local = 0; local < artifact->record_count; ++local) {
            FisicsSemanticProjectRecord* target = &out->records[record_position++];
            target->translation_unit_index = unit;
            target->local_record_index = local;
            if (!copy_record(&target->record, &artifact->records[local])) goto fail;
        }
        for (size_t local = 0; local < artifact->edge_count; ++local) {
            FisicsSemanticProjectEdge* target = &out->edges[edge_position++];
            target->translation_unit_index = unit;
            target->local_edge_index = local;
            if (!copy_edge(&target->edge, &artifact->edges[local])) goto fail;
        }
    }
    qsort(out->records, out->record_count, sizeof(*out->records), compare_project_records);
    for (size_t i = 0; i < out->record_count; ++i) out->records[i].record_index = i;
    source_identity_index = (FisicsSemanticProjectRecord**)calloc(
        out->record_count ? out->record_count : 1u, sizeof(*source_identity_index));
    authored_identity_index = (FisicsSemanticProjectRecord**)calloc(
        out->record_count ? out->record_count : 1u, sizeof(*authored_identity_index));
    if (!source_identity_index || !authored_identity_index) {
        set_diagnostic(diagnostic, "out of memory indexing semantic project identities");
        goto fail;
    }
    for (size_t i = 0; i < out->record_count; ++i) {
        source_identity_index[i] = &out->records[i];
        if (out->records[i].record.authored_id) {
            authored_identity_index[authored_identity_count++] = &out->records[i];
        }
    }
    qsort(source_identity_index, out->record_count, sizeof(*source_identity_index),
          compare_source_identity_pointers);
    qsort(authored_identity_index, authored_identity_count,
          sizeof(*authored_identity_index), compare_authored_identity_pointers);
    for (size_t i = 1; i < authored_identity_count; ++i) {
        FisicsSemanticProjectRecord* left = authored_identity_index[i - 1u];
        FisicsSemanticProjectRecord* right = authored_identity_index[i];
        if (strcmp(left->record.authored_id, right->record.authored_id) == 0) {
            set_diagnostic(diagnostic,
                           "duplicate project authored_id %s at %s:%d and %s:%d",
                           left->record.authored_id,
                           left->record.source_file_path, left->record.start_line,
                           right->record.source_file_path, right->record.start_line);
            goto fail;
        }
    }
    for (size_t i = 1; i < out->record_count; ++i) {
        FisicsSemanticProjectRecord* left = source_identity_index[i - 1u];
        FisicsSemanticProjectRecord* right = source_identity_index[i];
        if (strcmp(left->record.source_identity, right->record.source_identity) == 0) {
            set_diagnostic(diagnostic,
                           "duplicate project source_identity %s at %s:%d and %s:%d",
                           left->record.source_identity,
                           left->record.source_file_path, left->record.start_line,
                           right->record.source_file_path, right->record.start_line);
            goto fail;
        }
    }
    for (size_t i = 0; i < out->edge_count; ++i) {
        FisicsSemanticProjectEdge* project_edge = &out->edges[i];
        FisicsSemanticMetadataArtifact* artifact = &artifacts[project_edge->translation_unit_index];
        FisicsSemanticProjectRecord* source = find_origin_record(
            out, project_edge->translation_unit_index,
            artifact->edges[project_edge->local_edge_index].source_record_index);
        if (!source) {
            set_diagnostic(diagnostic, "cannot map project edge source origin");
            goto fail;
        }
        project_edge->edge.source_record_index = source->record_index;
        FisicsSemanticProjectRecord* target = find_authored_record(
            authored_identity_index, authored_identity_count,
            project_edge->edge.target_id);
        if (target) {
            project_edge->edge.resolution = FISICS_SEMANTIC_METADATA_REFERENCE_RESOLVED;
            project_edge->edge.has_target_record_index = true;
            project_edge->edge.target_record_index = target->record_index;
        } else if (external_allowed(out, project_edge->edge.target_id)) {
            project_edge->edge.resolution = FISICS_SEMANTIC_METADATA_REFERENCE_EXTERNAL;
            project_edge->edge.has_target_record_index = false;
            project_edge->edge.target_record_index = 0u;
        } else {
            set_diagnostic(diagnostic,
                           "missing required semantic target %s referenced at %s:%d",
                           project_edge->edge.target_id,
                           project_edge->edge.source_file_path,
                           project_edge->edge.start_line);
            goto fail;
        }
    }
    qsort(out->edges, out->edge_count, sizeof(*out->edges), compare_project_edges);
    free(source_identity_index);
    free(authored_identity_index);
    for (size_t i = 0; i < loaded; ++i) fisics_semantic_metadata_artifact_free(&artifacts[i]);
    free(artifacts);
    return true;

fail:
    free(source_identity_index);
    free(authored_identity_index);
    for (size_t i = 0; i < loaded; ++i) fisics_semantic_metadata_artifact_free(&artifacts[i]);
    free(artifacts);
    fisics_semantic_project_graph_free(out);
    if (diagnostic && !diagnostic->message[0])
        set_diagnostic(diagnostic, "failed to assemble semantic project graph");
    return false;
}
