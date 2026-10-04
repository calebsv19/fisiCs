// SPDX-License-Identifier: Apache-2.0

#include "Compiler/semantic_metadata_json_reader.h"
#include "Compiler/graph_source_snapshot.h"

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Compiler/semantic_project_json.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"

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

static bool object_has_exact_fields(const FisicsJsonValue* object,
                                    const char* const* fields,
                                    size_t count) {
    if (!object || object->type != FISICS_JSON_OBJECT ||
        object->as.object.count != count) return false;
    for (size_t i = 0; i < count; ++i) {
        if (!fisics_json_object_get(object, fields[i])) return false;
    }
    return true;
}

static bool parse_hex_hash(const FisicsJsonValue* value, uint64_t* out) {
    if (!value || value->type != FISICS_JSON_STRING ||
        strlen(value->as.text) != 16u || !out) return false;
    uint64_t result = 0;
    for (size_t i = 0; i < 16u; ++i) {
        char c = value->as.text[i];
        unsigned digit = 0;
        if (c >= '0' && c <= '9') digit = (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') digit = (unsigned)(c - 'a' + 10);
        else return false;
        result = (result << 4u) | digit;
    }
    *out = result;
    return true;
}

static bool parse_decimal_u64_string(const FisicsJsonValue* value, uint64_t* out) {
    if (!value || value->type != FISICS_JSON_STRING || !value->as.text[0] || !out) return false;
    if (value->as.text[0] == '0' && value->as.text[1] != '\0') return false;
    for (const char* p = value->as.text; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
    }
    errno = 0;
    char* end = NULL;
    unsigned long long parsed = strtoull(value->as.text, &end, 10);
    if (errno || !end || *end) return false;
    *out = (uint64_t)parsed;
    return true;
}

static bool semver_compatible(const char* version) {
    if (!version || !version[0]) return false;
    char extra = '\0';
    unsigned major = 0, minor = 0, patch = 0;
    if (sscanf(version, "%u.%u.%u%c", &major, &minor, &patch, &extra) != 3) return false;
    (void)patch;
    return major == 1u && minor >= 9u;
}

static bool valid_range(int start_line,
                        int start_column,
                        int end_line,
                        int end_column) {
    if (start_line <= 0 || start_column <= 0 || end_line <= 0 || end_column <= 0) {
        return false;
    }
    return end_line > start_line ||
           (end_line == start_line && end_column >= start_column);
}

static bool parse_source_range(const FisicsJsonValue* source,
                               const char* expected_path,
                               char** out_path,
                               int* start_line,
                               int* start_column,
                               int* end_line,
                               int* end_column) {
    static const char* const fields[] = {
        "file", "start_line", "start_column", "end_line", "end_column"
    };
    if (!object_has_exact_fields(source, fields, 5u)) return false;
    const FisicsJsonValue* file = fisics_json_object_get(source, "file");
    if (!file || file->type != FISICS_JSON_STRING ||
        strcmp(file->as.text, expected_path) != 0 ||
        !fisics_json_integer_int(fisics_json_object_get(source, "start_line"), start_line) ||
        !fisics_json_integer_int(fisics_json_object_get(source, "start_column"), start_column) ||
        !fisics_json_integer_int(fisics_json_object_get(source, "end_line"), end_line) ||
        !fisics_json_integer_int(fisics_json_object_get(source, "end_column"), end_column) ||
        !valid_range(*start_line, *start_column, *end_line, *end_column)) return false;
    *out_path = duplicate_string(file->as.text);
    return *out_path != NULL;
}

static bool read_file(const char* path,
                      size_t maximum,
                      char** out_bytes,
                      size_t* out_length,
                      FisicsSemanticProjectDiagnostic* diagnostic) {
    struct stat status;
    if (!path || stat(path, &status) != 0 || !S_ISREG(status.st_mode)) {
        set_diagnostic(diagnostic, "semantic metadata artifact is not a regular file: %s",
                       path ? path : "<null>");
        return false;
    }
    if (status.st_size < 0 || (uint64_t)status.st_size > maximum) {
        set_diagnostic(diagnostic, "semantic metadata artifact exceeds %zu bytes: %s",
                       maximum, path);
        return false;
    }
    FILE* file = fopen(path, "rb");
    if (!file) {
        set_diagnostic(diagnostic, "failed to read semantic metadata artifact: %s", path);
        return false;
    }
    size_t length = (size_t)status.st_size;
    char* bytes = (char*)malloc(length + 1u);
    if (!bytes) {
        fclose(file);
        set_diagnostic(diagnostic, "out of memory reading semantic metadata artifact: %s", path);
        return false;
    }
    size_t read_count = fread(bytes, 1u, length, file);
    int extra = fgetc(file);
    bool ok = read_count == length && extra == EOF && !ferror(file);
    if (fclose(file) != 0) ok = false;
    if (!ok) {
        free(bytes);
        set_diagnostic(diagnostic, "failed to read complete semantic metadata artifact: %s", path);
        return false;
    }
    bytes[length] = '\0';
    *out_bytes = bytes;
    *out_length = length;
    return true;
}

static bool verify_source(const char* path,
                          size_t expected_length,
                          uint64_t expected_hash,
                          FisicsSemanticProjectDiagnostic* diagnostic) {
    char snapshot_path[PATH_MAX];
    if (!fisics_graph_snapshot_source(path, snapshot_path)) {
        set_diagnostic(diagnostic, "invalid or missing frozen semantic source: %s", path);
        return false;
    }
    path = snapshot_path;
    char* canonical = realpath(path, NULL);
    if (!canonical || strcmp(canonical, path) != 0) {
        set_diagnostic(diagnostic, "metadata source path is missing or not canonical: %s", path);
        free(canonical);
        return false;
    }
    free(canonical);
    struct stat status;
    if (stat(path, &status) != 0 || !S_ISREG(status.st_mode) ||
        status.st_size < 0 || (uint64_t)status.st_size != expected_length) {
        set_diagnostic(diagnostic, "stale semantic metadata source length: %s", path);
        return false;
    }
    FILE* file = fopen(path, "rb");
    if (!file) {
        set_diagnostic(diagnostic, "failed to read semantic metadata source: %s", path);
        return false;
    }
    uint64_t hash = 1469598103934665603ULL;
    const uint64_t prime = 1099511628211ULL;
    unsigned char buffer[16384];
    size_t total = 0;
    while (!feof(file)) {
        size_t count = fread(buffer, 1u, sizeof(buffer), file);
        total += count;
        for (size_t i = 0; i < count; ++i) {
            hash ^= buffer[i];
            hash *= prime;
        }
        if (ferror(file)) {
            fclose(file);
            set_diagnostic(diagnostic, "failed while hashing semantic metadata source: %s", path);
            return false;
        }
    }
    if (fclose(file) != 0 || total != expected_length || hash != expected_hash) {
        set_diagnostic(diagnostic, "stale semantic metadata source hash: %s", path);
        return false;
    }
    return true;
}

static char* expected_fallback_identity(const FisicsFrontendSemanticMetadataRecord* record) {
    const char* kind = record->symbol_kind == FISICS_SEMANTIC_METADATA_SYMBOL_FUNCTION
                           ? "function"
                           : "file_variable";
    int needed = snprintf(NULL, 0, "symbol:%s:%s:%d:%d:%s", kind,
                          record->source_file_path, record->start_line,
                          record->start_col, record->symbol_name);
    if (needed < 0) return NULL;
    char* value = (char*)malloc((size_t)needed + 1u);
    if (value) snprintf(value, (size_t)needed + 1u, "symbol:%s:%s:%d:%d:%s", kind,
                        record->source_file_path, record->start_line,
                        record->start_col, record->symbol_name);
    return value;
}

static bool parse_record(const FisicsJsonValue* value,
                         const char* source_path,
                         FisicsFrontendSemanticMetadataRecord* out,
                         FisicsSemanticProjectDiagnostic* diagnostic,
                         size_t index) {
    static const char* const fields[] = {
        "source_identity", "authored_id", "symbol_stable_id", "symbol_name",
        "symbol_kind", "source", "domain", "role"
    };
    if (!object_has_exact_fields(value, fields, 8u)) {
        set_diagnostic(diagnostic, "records[%zu] has unknown or missing fields", index);
        return false;
    }
    const FisicsJsonValue* source_identity = fisics_json_object_get(value, "source_identity");
    const FisicsJsonValue* authored_id = fisics_json_object_get(value, "authored_id");
    const FisicsJsonValue* stable_id = fisics_json_object_get(value, "symbol_stable_id");
    const FisicsJsonValue* symbol_name = fisics_json_object_get(value, "symbol_name");
    const FisicsJsonValue* symbol_kind = fisics_json_object_get(value, "symbol_kind");
    const FisicsJsonValue* domain = fisics_json_object_get(value, "domain");
    const FisicsJsonValue* role = fisics_json_object_get(value, "role");
    if (!source_identity || source_identity->type != FISICS_JSON_STRING ||
        !source_identity->as.text[0] || !symbol_name ||
        symbol_name->type != FISICS_JSON_STRING || !symbol_name->as.text[0] ||
        !symbol_kind || symbol_kind->type != FISICS_JSON_STRING) {
        set_diagnostic(diagnostic, "records[%zu] has invalid identity or symbol fields", index);
        return false;
    }
    if (strcmp(symbol_kind->as.text, "function") == 0) {
        out->symbol_kind = FISICS_SEMANTIC_METADATA_SYMBOL_FUNCTION;
    } else if (strcmp(symbol_kind->as.text, "file_variable") == 0) {
        out->symbol_kind = FISICS_SEMANTIC_METADATA_SYMBOL_FILE_VARIABLE;
    } else {
        set_diagnostic(diagnostic, "records[%zu].symbol_kind is invalid", index);
        return false;
    }
    out->source_identity = duplicate_string(source_identity->as.text);
    out->symbol_name = duplicate_string(symbol_name->as.text);
    if (!out->source_identity || !out->symbol_name ||
        !parse_source_range(fisics_json_object_get(value, "source"), source_path,
                            (char**)&out->source_file_path, &out->start_line,
                            &out->start_col, &out->end_line, &out->end_col)) {
        set_diagnostic(diagnostic, "records[%zu] has invalid source range", index);
        return false;
    }
    const char* detail = NULL;
    if (authored_id->type == FISICS_JSON_STRING) {
        if (!fisics_semantic_metadata_is_canonical_name(authored_id->as.text, &detail) ||
            strcmp(authored_id->as.text, source_identity->as.text) != 0) {
            set_diagnostic(diagnostic, "records[%zu].authored_id is invalid", index);
            return false;
        }
        out->authored_id = duplicate_string(authored_id->as.text);
        if (!out->authored_id) return false;
    } else if (authored_id->type == FISICS_JSON_NULL) {
        char* expected = expected_fallback_identity(out);
        bool match = expected && strcmp(expected, out->source_identity) == 0;
        free(expected);
        if (!match) {
            set_diagnostic(diagnostic, "records[%zu].source_identity is not the canonical fallback", index);
            return false;
        }
    } else {
        set_diagnostic(diagnostic, "records[%zu].authored_id must be string or null", index);
        return false;
    }
    if (stable_id->type == FISICS_JSON_STRING) {
        if (!parse_decimal_u64_string(stable_id, &out->symbol_stable_id)) {
            set_diagnostic(diagnostic, "records[%zu].symbol_stable_id is invalid", index);
            return false;
        }
        out->has_symbol_stable_id = true;
    } else if (stable_id->type != FISICS_JSON_NULL) {
        set_diagnostic(diagnostic, "records[%zu].symbol_stable_id must be decimal string or null", index);
        return false;
    }
    if (domain->type == FISICS_JSON_STRING) {
        if (!fisics_semantic_metadata_domain_known(domain->as.text)) {
            set_diagnostic(diagnostic, "records[%zu].domain is not registered", index);
            return false;
        }
        out->domain = duplicate_string(domain->as.text);
        if (!out->domain) return false;
    } else if (domain->type != FISICS_JSON_NULL) {
        set_diagnostic(diagnostic, "records[%zu].domain must be string or null", index);
        return false;
    }
    if (role->type == FISICS_JSON_STRING) {
        if (!fisics_semantic_metadata_role_known(role->as.text)) {
            set_diagnostic(diagnostic, "records[%zu].role is not registered", index);
            return false;
        }
        out->role = duplicate_string(role->as.text);
        if (!out->role) return false;
    } else if (role->type != FISICS_JSON_NULL) {
        set_diagnostic(diagnostic, "records[%zu].role must be string or null", index);
        return false;
    }
    return true;
}

static bool parse_edge(const FisicsJsonValue* value,
                       const char* source_path,
                       FisicsFrontendSemanticMetadataEdge* out,
                       FisicsSemanticProjectDiagnostic* diagnostic,
                       size_t index) {
    static const char* const fields[] = {
        "source_record_index", "kind", "target_id", "resolution",
        "target_record_index", "occurrence"
    };
    if (!object_has_exact_fields(value, fields, 6u)) {
        set_diagnostic(diagnostic, "edges[%zu] has unknown or missing fields", index);
        return false;
    }
    const FisicsJsonValue* kind = fisics_json_object_get(value, "kind");
    const FisicsJsonValue* target = fisics_json_object_get(value, "target_id");
    const FisicsJsonValue* resolution = fisics_json_object_get(value, "resolution");
    const FisicsJsonValue* target_index = fisics_json_object_get(value, "target_record_index");
    if (!fisics_json_integer_size(fisics_json_object_get(value, "source_record_index"),
                                  &out->source_record_index) ||
        !kind || kind->type != FISICS_JSON_STRING ||
        !target || target->type != FISICS_JSON_STRING ||
        !resolution || resolution->type != FISICS_JSON_STRING) {
        set_diagnostic(diagnostic, "edges[%zu] has invalid core fields", index);
        return false;
    }
    if (strcmp(kind->as.text, "produces") == 0) {
        out->kind = FISICS_SEMANTIC_METADATA_EDGE_PRODUCES;
    } else if (strcmp(kind->as.text, "consumes") == 0) {
        out->kind = FISICS_SEMANTIC_METADATA_EDGE_CONSUMES;
    } else {
        set_diagnostic(diagnostic, "edges[%zu].kind is invalid", index);
        return false;
    }
    const char* detail = NULL;
    if (!fisics_semantic_metadata_is_canonical_name(target->as.text, &detail)) {
        set_diagnostic(diagnostic, "edges[%zu].target_id is invalid", index);
        return false;
    }
    out->target_id = duplicate_string(target->as.text);
    if (!out->target_id ||
        !parse_source_range(fisics_json_object_get(value, "occurrence"), source_path,
                            (char**)&out->source_file_path, &out->start_line,
                            &out->start_col, &out->end_line, &out->end_col)) {
        set_diagnostic(diagnostic, "edges[%zu] has invalid occurrence", index);
        return false;
    }
    if (strcmp(resolution->as.text, "resolved") == 0) {
        out->resolution = FISICS_SEMANTIC_METADATA_REFERENCE_RESOLVED;
        if (!fisics_json_integer_size(target_index, &out->target_record_index)) {
            set_diagnostic(diagnostic, "edges[%zu] resolved target index is invalid", index);
            return false;
        }
        out->has_target_record_index = true;
    } else if (strcmp(resolution->as.text, "external") == 0) {
        out->resolution = FISICS_SEMANTIC_METADATA_REFERENCE_EXTERNAL;
        if (target_index->type != FISICS_JSON_NULL) {
            set_diagnostic(diagnostic, "edges[%zu] external target index must be null", index);
            return false;
        }
    } else {
        set_diagnostic(diagnostic, "edges[%zu].resolution is invalid", index);
        return false;
    }
    return true;
}

static int compare_records(const void* left, const void* right) {
    const FisicsFrontendSemanticMetadataRecord* a = left;
    const FisicsFrontendSemanticMetadataRecord* b = right;
    int result = strcmp(a->source_file_path, b->source_file_path);
    if (result) return result;
    if (a->start_line != b->start_line) return a->start_line < b->start_line ? -1 : 1;
    if (a->start_col != b->start_col) return a->start_col < b->start_col ? -1 : 1;
    if (a->symbol_kind != b->symbol_kind) return a->symbol_kind < b->symbol_kind ? -1 : 1;
    result = strcmp(a->symbol_name, b->symbol_name);
    return result ? result : strcmp(a->source_identity, b->source_identity);
}

static int compare_edges(const void* left, const void* right) {
    const FisicsFrontendSemanticMetadataEdge* a = left;
    const FisicsFrontendSemanticMetadataEdge* b = right;
    if (a->source_record_index != b->source_record_index)
        return a->source_record_index < b->source_record_index ? -1 : 1;
    if (a->kind != b->kind) return a->kind < b->kind ? -1 : 1;
    int result = strcmp(a->target_id, b->target_id);
    if (result) return result;
    result = strcmp(a->source_file_path, b->source_file_path);
    if (result) return result;
    if (a->start_line != b->start_line) return a->start_line < b->start_line ? -1 : 1;
    if (a->start_col != b->start_col) return a->start_col < b->start_col ? -1 : 1;
    if (a->end_line != b->end_line) return a->end_line < b->end_line ? -1 : 1;
    if (a->end_col != b->end_col) return a->end_col < b->end_col ? -1 : 1;
    return 0;
}

static bool canonicalize_indices(FisicsSemanticMetadataArtifact* artifact,
                                 FisicsSemanticProjectDiagnostic* diagnostic) {
    size_t count = artifact->record_count;
    const FisicsFrontendSemanticMetadataRecord** old_records =
        (const FisicsFrontendSemanticMetadataRecord**)calloc(count ? count : 1u,
                                                              sizeof(*old_records));
    if (!old_records) return false;
    for (size_t i = 0; i < count; ++i) old_records[i] = &artifact->records[i];
    /* Preserve raw edge references by source identity before record sorting. */
    char** edge_sources = (char**)calloc(artifact->edge_count ? artifact->edge_count : 1u,
                                        sizeof(*edge_sources));
    char** edge_targets = (char**)calloc(artifact->edge_count ? artifact->edge_count : 1u,
                                        sizeof(*edge_targets));
    if (!edge_sources || !edge_targets) {
        free(old_records); free(edge_sources); free(edge_targets);
        return false;
    }
    for (size_t i = 0; i < artifact->edge_count; ++i) {
        FisicsFrontendSemanticMetadataEdge* edge = &artifact->edges[i];
        if (edge->source_record_index >= count ||
            (edge->has_target_record_index && edge->target_record_index >= count)) {
            set_diagnostic(diagnostic, "edge %zu contains an out-of-range local record index", i);
            goto fail;
        }
        edge_sources[i] = duplicate_string(old_records[edge->source_record_index]->source_identity);
        if (edge->has_target_record_index) {
            const FisicsFrontendSemanticMetadataRecord* target =
                old_records[edge->target_record_index];
            if (!target->authored_id || strcmp(target->authored_id, edge->target_id) != 0) {
                set_diagnostic(diagnostic, "edge %zu resolved target does not match target_id %s",
                               i, edge->target_id);
                goto fail;
            }
            edge_targets[i] = duplicate_string(target->source_identity);
        }
        if (!edge_sources[i] || (edge->has_target_record_index && !edge_targets[i])) goto fail;
    }
    qsort(artifact->records, count, sizeof(*artifact->records), compare_records);
    for (size_t i = 1; i < count; ++i) {
        if (strcmp(artifact->records[i - 1u].source_identity,
                   artifact->records[i].source_identity) == 0) {
            set_diagnostic(diagnostic, "duplicate record source_identity in artifact: %s",
                           artifact->records[i].source_identity);
            goto fail;
        }
    }
    for (size_t i = 0; i < artifact->edge_count; ++i) {
        FisicsFrontendSemanticMetadataEdge* edge = &artifact->edges[i];
        bool source_found = false;
        for (size_t j = 0; j < count; ++j) {
            if (strcmp(artifact->records[j].source_identity, edge_sources[i]) == 0) {
                edge->source_record_index = j;
                source_found = true;
            }
            if (edge_targets[i] &&
                strcmp(artifact->records[j].source_identity, edge_targets[i]) == 0) {
                edge->target_record_index = j;
            }
        }
        if (!source_found) goto fail;
        if (!edge->has_target_record_index) {
            for (size_t j = 0; j < count; ++j) {
                if (artifact->records[j].authored_id &&
                    strcmp(artifact->records[j].authored_id, edge->target_id) == 0) {
                    set_diagnostic(diagnostic,
                                   "edge target %s is marked external but resolves locally",
                                   edge->target_id);
                    goto fail;
                }
            }
        }
    }
    qsort(artifact->edges, artifact->edge_count, sizeof(*artifact->edges), compare_edges);
    for (size_t i = 1; i < artifact->edge_count; ++i) {
        const FisicsFrontendSemanticMetadataEdge* previous = &artifact->edges[i - 1u];
        const FisicsFrontendSemanticMetadataEdge* current = &artifact->edges[i];
        if (previous->source_record_index == current->source_record_index &&
            previous->kind == current->kind &&
            strcmp(previous->target_id, current->target_id) == 0) {
            set_diagnostic(diagnostic, "duplicate canonical edge in artifact: %s",
                           artifact->edges[i].target_id);
            goto fail;
        }
    }
    for (size_t i = 0; i < artifact->edge_count; ++i) {
        free(edge_sources[i]); free(edge_targets[i]);
    }
    free(edge_sources); free(edge_targets); free(old_records);
    return true;

fail:
    for (size_t i = 0; i < artifact->edge_count; ++i) {
        free(edge_sources[i]); free(edge_targets[i]);
    }
    free(edge_sources); free(edge_targets); free(old_records);
    if (!diagnostic || !diagnostic->message[0])
        set_diagnostic(diagnostic, "failed to canonicalize artifact indices");
    return false;
}

void fisics_semantic_metadata_artifact_free(FisicsSemanticMetadataArtifact* artifact) {
    if (!artifact) return;
    free(artifact->metadata_path);
    free(artifact->producer_version);
    free(artifact->analysis_contract_version);
    free(artifact->source_path);
    for (size_t i = 0; i < artifact->record_count; ++i) {
        free((char*)artifact->records[i].source_identity);
        free((char*)artifact->records[i].authored_id);
        free((char*)artifact->records[i].symbol_name);
        free((char*)artifact->records[i].source_file_path);
        free((char*)artifact->records[i].domain);
        free((char*)artifact->records[i].role);
    }
    free(artifact->records);
    for (size_t i = 0; i < artifact->edge_count; ++i) {
        free((char*)artifact->edges[i].target_id);
        free((char*)artifact->edges[i].source_file_path);
    }
    free(artifact->edges);
    memset(artifact, 0, sizeof(*artifact));
}

bool fisics_semantic_metadata_artifact_load(
    const char* path,
    FisicsSemanticMetadataArtifact* out,
    size_t* out_artifact_bytes,
    FisicsSemanticProjectDiagnostic* diagnostic) {
    if (!path || !out) return false;
    memset(out, 0, sizeof(*out));
    if (out_artifact_bytes) *out_artifact_bytes = 0;
    char* bytes = NULL;
    size_t length = 0;
    if (!read_file(path, FISICS_SEMANTIC_METADATA_ARTIFACT_MAX_BYTES,
                   &bytes, &length, diagnostic)) return false;
    FisicsJsonValue* root = NULL;
    FisicsJsonDiagnostic json_diagnostic = {0};
    if (!fisics_json_parse(bytes, length, &root, &json_diagnostic)) {
        set_diagnostic(diagnostic, "invalid semantic metadata artifact %s: %s", path,
                       json_diagnostic.message[0] ? json_diagnostic.message : "invalid JSON");
        free(bytes);
        return false;
    }
    free(bytes);
    static const char* const root_fields[] = {
        "schema", "schema_version", "producer", "analysis_contract", "source",
        "complete", "records", "edges"
    };
    if (!object_has_exact_fields(root, root_fields, 8u)) {
        set_diagnostic(diagnostic, "semantic metadata artifact %s has unknown or missing root fields", path);
        goto fail;
    }
    const FisicsJsonValue* schema = fisics_json_object_get(root, "schema");
    uint64_t schema_version = 0;
    if (!schema || schema->type != FISICS_JSON_STRING ||
        strcmp(schema->as.text, "fisiCs.semantic_metadata") != 0 ||
        !fisics_json_integer_u64(fisics_json_object_get(root, "schema_version"),
                                 &schema_version) || schema_version != 1u) {
        set_diagnostic(diagnostic, "semantic metadata artifact %s requires schema v1", path);
        goto fail;
    }
    static const char* const producer_fields[] = {"name", "version"};
    const FisicsJsonValue* producer = fisics_json_object_get(root, "producer");
    const FisicsJsonValue* producer_name = fisics_json_object_get(producer, "name");
    const FisicsJsonValue* producer_version = fisics_json_object_get(producer, "version");
    if (!object_has_exact_fields(producer, producer_fields, 2u) ||
        !producer_name || producer_name->type != FISICS_JSON_STRING ||
        strcmp(producer_name->as.text, "fisiCs") != 0 ||
        !producer_version || producer_version->type != FISICS_JSON_STRING ||
        !producer_version->as.text[0]) {
        set_diagnostic(diagnostic, "semantic metadata artifact %s has invalid producer", path);
        goto fail;
    }
    static const char* const contract_fields[] = {"id", "version"};
    const FisicsJsonValue* contract = fisics_json_object_get(root, "analysis_contract");
    const FisicsJsonValue* contract_id = fisics_json_object_get(contract, "id");
    const FisicsJsonValue* contract_version = fisics_json_object_get(contract, "version");
    if (!object_has_exact_fields(contract, contract_fields, 2u) ||
        !contract_id || contract_id->type != FISICS_JSON_STRING ||
        strcmp(contract_id->as.text, "fisiCs.analysis.contract") != 0 ||
        !contract_version || contract_version->type != FISICS_JSON_STRING ||
        !semver_compatible(contract_version->as.text)) {
        set_diagnostic(diagnostic, "semantic metadata artifact %s has incompatible analysis contract", path);
        goto fail;
    }
    static const char* const source_fields[] = {"path", "hash_fnv1a64", "length_bytes"};
    const FisicsJsonValue* source = fisics_json_object_get(root, "source");
    const FisicsJsonValue* source_path = fisics_json_object_get(source, "path");
    uint64_t source_hash = 0;
    size_t source_length = 0;
    if (!object_has_exact_fields(source, source_fields, 3u) ||
        !source_path || source_path->type != FISICS_JSON_STRING ||
        source_path->as.text[0] != '/' ||
        !parse_hex_hash(fisics_json_object_get(source, "hash_fnv1a64"), &source_hash) ||
        !fisics_json_integer_size(fisics_json_object_get(source, "length_bytes"),
                                  &source_length)) {
        set_diagnostic(diagnostic, "semantic metadata artifact %s has invalid source provenance", path);
        goto fail;
    }
    const FisicsJsonValue* complete = fisics_json_object_get(root, "complete");
    const FisicsJsonValue* records = fisics_json_object_get(root, "records");
    const FisicsJsonValue* edges = fisics_json_object_get(root, "edges");
    if (!complete || complete->type != FISICS_JSON_BOOL || !complete->as.boolean ||
        !records || records->type != FISICS_JSON_ARRAY ||
        records->as.array.count > FISICS_SEMANTIC_PROJECT_MAX_RECORDS ||
        !edges || edges->type != FISICS_JSON_ARRAY ||
        edges->as.array.count > FISICS_SEMANTIC_PROJECT_MAX_EDGES) {
        set_diagnostic(diagnostic, "semantic metadata artifact %s is incomplete or exceeds v1 limits", path);
        goto fail;
    }
    out->metadata_path = duplicate_string(path);
    out->producer_version = duplicate_string(producer_version->as.text);
    out->analysis_contract_version = duplicate_string(contract_version->as.text);
    out->source_path = duplicate_string(source_path->as.text);
    out->source_hash = source_hash;
    out->source_length = source_length;
    out->record_count = records->as.array.count;
    out->edge_count = edges->as.array.count;
    out->records = (FisicsFrontendSemanticMetadataRecord*)calloc(
        out->record_count ? out->record_count : 1u, sizeof(*out->records));
    out->edges = (FisicsFrontendSemanticMetadataEdge*)calloc(
        out->edge_count ? out->edge_count : 1u, sizeof(*out->edges));
    if (!out->metadata_path || !out->producer_version ||
        !out->analysis_contract_version || !out->source_path ||
        !out->records || !out->edges) {
        set_diagnostic(diagnostic, "out of memory loading semantic metadata artifact %s", path);
        goto fail;
    }
    for (size_t i = 0; i < out->record_count; ++i) {
        if (!parse_record(records->as.array.items[i], out->source_path,
                          &out->records[i], diagnostic, i)) goto fail;
    }
    for (size_t i = 0; i < out->edge_count; ++i) {
        if (!parse_edge(edges->as.array.items[i], out->source_path,
                        &out->edges[i], diagnostic, i)) goto fail;
    }
    if (!canonicalize_indices(out, diagnostic) ||
        !verify_source(out->source_path, out->source_length, out->source_hash, diagnostic)) {
        goto fail;
    }
    if (out_artifact_bytes) *out_artifact_bytes = length;
    fisics_json_free(root);
    return true;

fail:
    fisics_json_free(root);
    fisics_semantic_metadata_artifact_free(out);
    return false;
}
