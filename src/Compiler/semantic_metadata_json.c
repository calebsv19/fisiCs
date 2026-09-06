// SPDX-License-Identifier: Apache-2.0

#include "Compiler/semantic_metadata_json.h"
#include "Compiler/analysis_contract_version.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
                } else if (fputc(*cursor, out) == EOF) {
                    return false;
                }
        }
    }
    return fputc('"', out) != EOF;
}

static bool nullable_string(FILE* out, const char* value) {
    return value ? json_string(out, value) : fputs("null", out) != EOF;
}

static const char* symbol_kind_name(FisicsFrontendSemanticMetadataSymbolKind kind) {
    return kind == FISICS_SEMANTIC_METADATA_SYMBOL_FUNCTION
               ? "function"
               : "file_variable";
}

static const char* edge_kind_name(FisicsFrontendSemanticMetadataEdgeKind kind) {
    return kind == FISICS_SEMANTIC_METADATA_EDGE_PRODUCES
               ? "produces"
               : "consumes";
}

static const char* resolution_name(FisicsFrontendSemanticMetadataReferenceState state) {
    return state == FISICS_SEMANTIC_METADATA_REFERENCE_RESOLVED
               ? "resolved"
               : "external";
}

static bool write_record(FILE* out,
                         const FisicsFrontendSemanticMetadataRecord* record,
                         bool final) {
    if (fputs("    {\n      \"source_identity\": ", out) == EOF ||
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
                       const FisicsFrontendSemanticMetadataEdge* edge,
                       bool final) {
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
    if (fputs(",\n      \"occurrence\": {\n        \"file\": ", out) == EOF ||
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

static bool write_document(FILE* out,
                           const char* source_path,
                           uint64_t source_hash,
                           size_t source_length,
                           const FisicsFrontendSemanticMetadataRecord* records,
                           size_t record_count,
                           const FisicsFrontendSemanticMetadataEdge* edges,
                           size_t edge_count) {
    if (fputs("{\n  \"schema\": \"fisiCs.semantic_metadata\",\n"
              "  \"schema_version\": 1,\n"
              "  \"producer\": {\n"
              "    \"name\": \"fisiCs\",\n"
              "    \"version\": \"" FISICS_VERSION "\"\n"
              "  },\n"
              "  \"analysis_contract\": {\n"
              "    \"id\": \"" FISICS_ANALYSIS_CONTRACT_ID "\",\n"
              "    \"version\": \"" FISICS_ANALYSIS_CONTRACT_VERSION_STRING "\"\n"
              "  },\n"
              "  \"source\": {\n"
              "    \"path\": ", out) == EOF ||
        !json_string(out, source_path) ||
        fprintf(out,
                ",\n    \"hash_fnv1a64\": \"%016" PRIx64 "\",\n"
                "    \"length_bytes\": %zu\n"
                "  },\n"
                "  \"complete\": true,\n"
                "  \"records\": [\n",
                source_hash, source_length) < 0) return false;
    for (size_t i = 0; i < record_count; ++i) {
        if (!write_record(out, &records[i], i + 1u == record_count)) return false;
    }
    if (fputs("  ],\n  \"edges\": [\n", out) == EOF) return false;
    for (size_t i = 0; i < edge_count; ++i) {
        if (!write_edge(out, &edges[i], i + 1u == edge_count)) return false;
    }
    return fputs("  ]\n}\n", out) != EOF;
}

bool fisics_semantic_metadata_write_json_atomic(
    const char* output_path,
    const char* source_path,
    uint64_t source_hash,
    size_t source_length,
    const FisicsFrontendSemanticMetadataRecord* records,
    size_t record_count,
    const FisicsFrontendSemanticMetadataEdge* edges,
    size_t edge_count) {
    if (!output_path || !output_path[0] || !source_path || !source_path[0] ||
        (record_count && !records) || (edge_count && !edges)) return false;

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
    bool ok = out && write_document(out, source_path, source_hash, source_length,
                                    records, record_count, edges, edge_count);
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
