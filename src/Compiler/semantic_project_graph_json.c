// SPDX-License-Identifier: Apache-2.0

#include "Compiler/semantic_project_graph_json.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef FISICS_VERSION
#define FISICS_VERSION "unknown"
#endif

static bool json_string(FILE* out, const char* value) {
    if (fputc('"', out) == EOF) return false;
    const unsigned char* cursor = (const unsigned char*)(value ? value : "");
    for (; *cursor; ++cursor) {
        switch (*cursor) {
            case '"': if (fputs("\\\"", out) == EOF) return false; break;
            case '\\': if (fputs("\\\\", out) == EOF) return false; break;
            case '\b': if (fputs("\\b", out) == EOF) return false; break;
            case '\f': if (fputs("\\f", out) == EOF) return false; break;
            case '\n': if (fputs("\\n", out) == EOF) return false; break;
            case '\r': if (fputs("\\r", out) == EOF) return false; break;
            case '\t': if (fputs("\\t", out) == EOF) return false; break;
            default:
                if (*cursor < 0x20u) {
                    if (fprintf(out, "\\u%04x", (unsigned)*cursor) < 0) return false;
                } else if (fputc(*cursor, out) == EOF) return false;
        }
    }
    return fputc('"', out) != EOF;
}

static bool nullable_string(FILE* out, const char* value) {
    return value ? json_string(out, value) : fputs("null", out) != EOF;
}

static const char* symbol_kind_name(FisicsFrontendSemanticMetadataSymbolKind kind) {
    return kind == FISICS_SEMANTIC_METADATA_SYMBOL_FUNCTION ? "function" : "file_variable";
}

static const char* edge_kind_name(FisicsFrontendSemanticMetadataEdgeKind kind) {
    return kind == FISICS_SEMANTIC_METADATA_EDGE_PRODUCES ? "produces" : "consumes";
}

static const char* resolution_name(FisicsFrontendSemanticMetadataReferenceState state) {
    return state == FISICS_SEMANTIC_METADATA_REFERENCE_RESOLVED ? "resolved" : "external";
}

static bool write_translation_unit(FILE* out,
                                   const FisicsSemanticProjectTranslationUnit* unit,
                                   bool final) {
    if (fprintf(out, "    {\n      \"translation_unit_index\": %zu,\n      \"source\": {\n        \"path\": ",
                unit->translation_unit_index) < 0 ||
        !json_string(out, unit->source_path) ||
        fprintf(out,
                ",\n        \"hash_fnv1a64\": \"%016" PRIx64 "\",\n"
                "        \"length_bytes\": %zu\n      },\n"
                "      \"metadata_artifact\": {\n        \"path\": ",
                unit->source_hash, unit->source_length) < 0 ||
        !json_string(out, unit->metadata_path) ||
        fputs("\n      },\n      \"producer\": {\n        \"name\": \"fisiCs\",\n        \"version\": ", out) == EOF ||
        !json_string(out, unit->producer_version) ||
        fputs("\n      },\n      \"analysis_contract\": {\n        \"id\": \"fisiCs.analysis.contract\",\n        \"version\": ", out) == EOF ||
        !json_string(out, unit->analysis_contract_version) ||
        fprintf(out,
                "\n      },\n      \"record_count\": %zu,\n      \"edge_count\": %zu\n    }%s\n",
                unit->record_count, unit->edge_count, final ? "" : ",") < 0) return false;
    return true;
}

static bool write_record(FILE* out,
                         const FisicsSemanticProjectRecord* project_record,
                         bool final) {
    const FisicsFrontendSemanticMetadataRecord* record = &project_record->record;
    if (fprintf(out,
                "    {\n      \"record_index\": %zu,\n"
                "      \"origin\": {\n"
                "        \"translation_unit_index\": %zu,\n"
                "        \"local_record_index\": %zu\n"
                "      },\n      \"source_identity\": ",
                project_record->record_index,
                project_record->translation_unit_index,
                project_record->local_record_index) < 0 ||
        !json_string(out, record->source_identity) ||
        fputs(",\n      \"authored_id\": ", out) == EOF ||
        !nullable_string(out, record->authored_id) ||
        fputs(",\n      \"symbol_stable_id\": ", out) == EOF) return false;
    if (record->has_symbol_stable_id) {
        if (fprintf(out, "\"%" PRIu64 "\"", record->symbol_stable_id) < 0) return false;
    } else if (fputs("null", out) == EOF) return false;
    if (fputs(",\n      \"symbol_name\": ", out) == EOF ||
        !json_string(out, record->symbol_name) ||
        fputs(",\n      \"symbol_kind\": ", out) == EOF ||
        !json_string(out, symbol_kind_name(record->symbol_kind)) ||
        fputs(",\n      \"source\": {\n        \"file\": ", out) == EOF ||
        !json_string(out, record->source_file_path) ||
        fprintf(out,
                ",\n        \"start_line\": %d,\n"
                "        \"start_column\": %d,\n"
                "        \"end_line\": %d,\n"
                "        \"end_column\": %d\n      },\n"
                "      \"domain\": ",
                record->start_line, record->start_col,
                record->end_line, record->end_col) < 0 ||
        !nullable_string(out, record->domain) ||
        fputs(",\n      \"role\": ", out) == EOF ||
        !nullable_string(out, record->role) ||
        fprintf(out, "\n    }%s\n", final ? "" : ",") < 0) return false;
    return true;
}

static bool write_edge(FILE* out,
                       const FisicsSemanticProjectEdge* project_edge,
                       bool final) {
    const FisicsFrontendSemanticMetadataEdge* edge = &project_edge->edge;
    if (fprintf(out, "    {\n      \"source_record_index\": %zu,\n      \"kind\": ",
                edge->source_record_index) < 0 ||
        !json_string(out, edge_kind_name(edge->kind)) ||
        fputs(",\n      \"target_id\": ", out) == EOF ||
        !json_string(out, edge->target_id) ||
        fputs(",\n      \"resolution\": ", out) == EOF ||
        !json_string(out, resolution_name(edge->resolution)) ||
        fputs(",\n      \"target_record_index\": ", out) == EOF) return false;
    if (edge->has_target_record_index) {
        if (fprintf(out, "%zu", edge->target_record_index) < 0) return false;
    } else if (fputs("null", out) == EOF) return false;
    if (fprintf(out,
                ",\n      \"origin\": {\n"
                "        \"translation_unit_index\": %zu,\n"
                "        \"local_edge_index\": %zu\n"
                "      },\n      \"occurrence\": {\n        \"file\": ",
                project_edge->translation_unit_index,
                project_edge->local_edge_index) < 0 ||
        !json_string(out, edge->source_file_path) ||
        fprintf(out,
                ",\n        \"start_line\": %d,\n"
                "        \"start_column\": %d,\n"
                "        \"end_line\": %d,\n"
                "        \"end_column\": %d\n      }\n"
                "    }%s\n",
                edge->start_line, edge->start_col,
                edge->end_line, edge->end_col,
                final ? "" : ",") < 0) return false;
    return true;
}

static bool write_document(FILE* out, const FisicsSemanticProjectGraph* graph) {
    if (fputs("{\n  \"schema\": \"fisiCs.semantic_project_graph\",\n"
              "  \"schema_version\": 1,\n"
              "  \"producer\": {\n"
              "    \"name\": \"fisiCs\",\n"
              "    \"version\": \"" FISICS_VERSION "\"\n"
              "  },\n"
              "  \"project\": {\n"
              "    \"id\": ", out) == EOF ||
        !json_string(out, graph->project_id) ||
        fputs(",\n    \"manifest_path\": ", out) == EOF ||
        !json_string(out, graph->manifest_path) ||
        fputs("\n  },\n  \"complete\": true,\n  \"translation_units\": [\n", out) == EOF) return false;
    for (size_t i = 0; i < graph->translation_unit_count; ++i) {
        if (!write_translation_unit(out, &graph->translation_units[i],
                                    i + 1u == graph->translation_unit_count)) return false;
    }
    if (fputs("  ],\n  \"allowed_external_targets\": [", out) == EOF) return false;
    if (graph->allowed_external_target_count && fputc('\n', out) == EOF) return false;
    for (size_t i = 0; i < graph->allowed_external_target_count; ++i) {
        if (fputs("    ", out) == EOF ||
            !json_string(out, graph->allowed_external_targets[i]) ||
            fprintf(out, "%s\n", i + 1u == graph->allowed_external_target_count ? "" : ",") < 0)
            return false;
    }
    if (graph->allowed_external_target_count) {
        if (fputs("  ],\n", out) == EOF) return false;
    } else if (fputs("],\n", out) == EOF) return false;
    if (fputs("  \"records\": [\n", out) == EOF) return false;
    for (size_t i = 0; i < graph->record_count; ++i) {
        if (!write_record(out, &graph->records[i], i + 1u == graph->record_count)) return false;
    }
    if (fputs("  ],\n  \"edges\": [\n", out) == EOF) return false;
    for (size_t i = 0; i < graph->edge_count; ++i) {
        if (!write_edge(out, &graph->edges[i], i + 1u == graph->edge_count)) return false;
    }
    return fputs("  ]\n}\n", out) != EOF;
}

static char* duplicate_range(const char* start, size_t length) {
    char* value = (char*)malloc(length + 1u);
    if (value) {
        memcpy(value, start, length);
        value[length] = '\0';
    }
    return value;
}

static char* canonical_output_path(const char* path) {
    char* existing = realpath(path, NULL);
    if (existing) return existing;
    const char* slash = strrchr(path, '/');
    char* directory = NULL;
    const char* basename = path;
    if (!slash) {
        directory = duplicate_range(".", 1u);
    } else {
        basename = slash + 1u;
        directory = duplicate_range(path, slash == path ? 1u : (size_t)(slash - path));
    }
    if (!directory || !basename[0]) {
        free(directory);
        return NULL;
    }
    char* resolved_directory = realpath(directory, NULL);
    free(directory);
    if (!resolved_directory) return NULL;
    size_t directory_length = strlen(resolved_directory);
    size_t basename_length = strlen(basename);
    bool slash_needed = resolved_directory[directory_length - 1u] != '/';
    char* result = (char*)malloc(directory_length + (slash_needed ? 1u : 0u) +
                                 basename_length + 1u);
    if (result) {
        memcpy(result, resolved_directory, directory_length);
        size_t position = directory_length;
        if (slash_needed) result[position++] = '/';
        memcpy(result + position, basename, basename_length + 1u);
    }
    free(resolved_directory);
    return result;
}

static bool output_path_safe(const char* output_path,
                             const FisicsSemanticProjectGraph* graph,
                             FisicsSemanticProjectDiagnostic* diagnostic) {
    char* canonical = canonical_output_path(output_path);
    if (!canonical) {
        if (diagnostic) snprintf(diagnostic->message, sizeof(diagnostic->message),
                                 "cannot resolve semantic project graph output path: %s",
                                 output_path);
        return false;
    }
    bool conflict = strcmp(canonical, graph->manifest_path) == 0;
    for (size_t i = 0; !conflict && i < graph->translation_unit_count; ++i) {
        conflict = strcmp(canonical, graph->translation_units[i].metadata_path) == 0 ||
                   strcmp(canonical, graph->translation_units[i].source_path) == 0;
    }
    if (conflict && diagnostic) {
        snprintf(diagnostic->message, sizeof(diagnostic->message),
                 "semantic project graph output conflicts with an input or source path: %s",
                 canonical);
    }
    free(canonical);
    return !conflict;
}

bool fisics_semantic_project_graph_write_json_atomic(
    const char* output_path,
    const FisicsSemanticProjectGraph* graph,
    FisicsSemanticProjectDiagnostic* diagnostic) {
    if (!output_path || !output_path[0] || !graph) return false;
    if (!output_path_safe(output_path, graph, diagnostic)) return false;
    const char* fail_write = getenv("FISICS_TEST_FAIL_SEMANTIC_PROJECT_GRAPH_WRITE");
    if (fail_write && fail_write[0] && fail_write[0] != '0') return false;
    size_t path_length = strlen(output_path);
    char* temporary_path = (char*)malloc(path_length + sizeof(".tmp.XXXXXX"));
    if (!temporary_path) return false;
    snprintf(temporary_path, path_length + sizeof(".tmp.XXXXXX"),
             "%s.tmp.XXXXXX", output_path);
    int descriptor = mkstemp(temporary_path);
    if (descriptor < 0) {
        free(temporary_path);
        return false;
    }
    FILE* out = fdopen(descriptor, "w");
    bool ok = out && write_document(out, graph);
    if (out) {
        if (ok && fflush(out) != 0) ok = false;
        if (ok && fsync(descriptor) != 0) ok = false;
        if (fclose(out) != 0) ok = false;
    } else {
        close(descriptor);
    }
    if (ok && rename(temporary_path, output_path) != 0) ok = false;
    if (!ok) unlink(temporary_path);
    free(temporary_path);
    return ok;
}
