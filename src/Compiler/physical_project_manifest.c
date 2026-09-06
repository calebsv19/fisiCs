// SPDX-License-Identifier: Apache-2.0

#include "Compiler/physical_project_manifest.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Compiler/semantic_project_json.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"

static void diag(FisicsSemanticProjectDiagnostic* d, const char* fmt, ...) {
    if (!d || d->message[0]) return;
    va_list args; va_start(args, fmt);
    vsnprintf(d->message, sizeof(d->message), fmt, args);
    va_end(args);
}

static char* dup_text(const char* text) {
    if (!text) return NULL;
    size_t n = strlen(text) + 1u;
    char* out = (char*)malloc(n);
    if (out) memcpy(out, text, n);
    return out;
}

static bool exact_fields(const FisicsJsonValue* object,
                         const char* const* fields, size_t count) {
    if (!object || object->type != FISICS_JSON_OBJECT || object->as.object.count != count) return false;
    for (size_t i = 0; i < count; ++i) if (!fisics_json_object_get(object, fields[i])) return false;
    return true;
}

static char* read_file(const char* path, size_t maximum, size_t* out_length,
                       FisicsSemanticProjectDiagnostic* d) {
    struct stat st;
    if (!path || stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
        diag(d, "physical project manifest is not a regular file: %s", path ? path : "<null>");
        return NULL;
    }
    if (st.st_size < 0 || (uint64_t)st.st_size > maximum) {
        diag(d, "physical project manifest exceeds %zu bytes: %s", maximum, path);
        return NULL;
    }
    FILE* f = fopen(path, "rb");
    if (!f) { diag(d, "failed to read physical project manifest: %s", path); return NULL; }
    size_t length = (size_t)st.st_size;
    char* bytes = (char*)malloc(length + 1u);
    bool ok = bytes && fread(bytes, 1u, length, f) == length && fgetc(f) == EOF && !ferror(f);
    if (fclose(f) != 0) ok = false;
    if (!ok) { free(bytes); diag(d, "failed to read complete physical project manifest: %s", path); return NULL; }
    bytes[length] = '\0';
    if (out_length) *out_length = length;
    return bytes;
}

static char* directory_of(const char* path) {
    const char* slash = path ? strrchr(path, '/') : NULL;
    if (!slash) return dup_text(".");
    if (slash == path) return dup_text("/");
    size_t n = (size_t)(slash - path);
    char* out = (char*)malloc(n + 1u);
    if (out) { memcpy(out, path, n); out[n] = '\0'; }
    return out;
}

static char* resolve_path(const char* directory, const char* path,
                          FisicsSemanticProjectDiagnostic* d) {
    if (!path || !path[0]) return NULL;
    char* joined = NULL;
    if (path[0] == '/') joined = dup_text(path);
    else {
        size_t a = strlen(directory), b = strlen(path);
        bool slash = a && directory[a - 1u] != '/';
        joined = (char*)malloc(a + b + (slash ? 2u : 1u));
        if (joined) snprintf(joined, a + b + (slash ? 2u : 1u), "%s%s%s", directory, slash ? "/" : "", path);
    }
    if (!joined) { diag(d, "out of memory resolving physical project path"); return NULL; }
    char* resolved = realpath(joined, NULL);
    free(joined);
    if (!resolved) diag(d, "physical project input does not exist: %s", path);
    return resolved;
}

static int compare_strings(const void* l, const void* r) {
    return strcmp(*(const char* const*)l, *(const char* const*)r);
}

void fisics_physical_project_manifest_free(FisicsPhysicalProjectManifest* m) {
    if (!m) return;
    free(m->project_id); free(m->manifest_path); free(m->semantic_graph_path);
    for (size_t i = 0; i < m->units_metadata_path_count; ++i) free(m->units_metadata_paths[i]);
    for (size_t i = 0; i < m->catalog_path_count; ++i) free(m->catalog_paths[i]);
    free(m->units_metadata_paths); free(m->catalog_paths);
    memset(m, 0, sizeof(*m));
}

static bool parse_path_array(const FisicsJsonValue* value, const char* directory,
                             size_t maximum, const char* label, char*** out_paths,
                             size_t* out_count, FisicsSemanticProjectDiagnostic* d) {
    if (!value || value->type != FISICS_JSON_ARRAY || value->as.array.count == 0u ||
        value->as.array.count > maximum) {
        diag(d, "%s must contain 1 through %zu paths", label, maximum); return false;
    }
    char** paths = (char**)calloc(value->as.array.count, sizeof(*paths));
    if (!paths) { diag(d, "out of memory loading %s", label); return false; }
    for (size_t i = 0; i < value->as.array.count; ++i) {
        const FisicsJsonValue* item = value->as.array.items[i];
        if (!item || item->type != FISICS_JSON_STRING || !item->as.text[0]) {
            diag(d, "%s[%zu] must be a non-empty path string", label, i); goto fail;
        }
        paths[i] = resolve_path(directory, item->as.text, d);
        if (!paths[i]) goto fail;
    }
    qsort(paths, value->as.array.count, sizeof(*paths), compare_strings);
    for (size_t i = 1; i < value->as.array.count; ++i) {
        if (strcmp(paths[i - 1u], paths[i]) == 0) {
            diag(d, "duplicate %s path: %s", label, paths[i]); goto fail;
        }
    }
    *out_paths = paths; *out_count = value->as.array.count; return true;
fail:
    for (size_t i = 0; i < value->as.array.count; ++i) free(paths[i]);
    free(paths); return false;
}

bool fisics_physical_project_manifest_load(const char* path,
                                           FisicsPhysicalProjectManifest* out,
                                           FisicsSemanticProjectDiagnostic* d) {
    if (d) d->message[0] = '\0';
    if (!path || !path[0] || !out) { diag(d, "invalid physical project manifest arguments"); return false; }
    memset(out, 0, sizeof(*out));
    char* canonical = realpath(path, NULL);
    if (!canonical) { diag(d, "physical project manifest does not exist: %s", path); return false; }
    size_t length = 0;
    char* bytes = read_file(canonical, FISICS_PHYSICAL_PROJECT_MANIFEST_MAX_BYTES, &length, d);
    if (!bytes) { free(canonical); return false; }
    FisicsJsonValue* root = NULL; FisicsJsonDiagnostic jd = {0};
    if (!fisics_json_parse(bytes, length, &root, &jd)) {
        diag(d, "invalid physical project manifest %s: %s", canonical, jd.message);
        free(bytes); free(canonical); return false;
    }
    free(bytes);
    static const char* const fields[] = {"schema","schema_version","project_id",
        "semantic_project_graph_path","units_metadata_paths","physical_catalog_paths",
        "coverage_policy","source_evidence_policy"};
    if (!exact_fields(root, fields, 8u)) {
        diag(d, "physical project manifest must contain exactly the eight v1 fields"); goto fail;
    }
    uint64_t version = 0;
    const FisicsJsonValue* schema = fisics_json_object_get(root, "schema");
    const FisicsJsonValue* project = fisics_json_object_get(root, "project_id");
    const FisicsJsonValue* graph = fisics_json_object_get(root, "semantic_project_graph_path");
    const FisicsJsonValue* coverage = fisics_json_object_get(root, "coverage_policy");
    const FisicsJsonValue* evidence = fisics_json_object_get(root, "source_evidence_policy");
    const char* detail = NULL;
    if (!schema || schema->type != FISICS_JSON_STRING || strcmp(schema->as.text, "fisiCs.physical_project_manifest") != 0 ||
        !fisics_json_integer_u64(fisics_json_object_get(root, "schema_version"), &version) || version != 1u) {
        diag(d, "physical project manifest requires fisiCs.physical_project_manifest schema v1"); goto fail;
    }
    if (!project || project->type != FISICS_JSON_STRING ||
        !fisics_semantic_metadata_is_canonical_name(project->as.text, &detail)) {
        diag(d, "invalid physical project_id: %s", detail ? detail : "expected canonical string"); goto fail;
    }
    if (!graph || graph->type != FISICS_JSON_STRING || !graph->as.text[0] ||
        !coverage || coverage->type != FISICS_JSON_STRING ||
        !evidence || evidence->type != FISICS_JSON_STRING) {
        diag(d, "physical project manifest path and policy fields must be strings"); goto fail;
    }
    if (strcmp(coverage->as.text, "allow_partial") == 0) out->coverage_policy = FISICS_PHYSICAL_COVERAGE_ALLOW_PARTIAL;
    else if (strcmp(coverage->as.text, "require_relationship_targets") == 0) out->coverage_policy = FISICS_PHYSICAL_COVERAGE_RELATIONSHIP_TARGETS;
    else if (strcmp(coverage->as.text, "require_all_authored_records") == 0) out->coverage_policy = FISICS_PHYSICAL_COVERAGE_ALL_AUTHORED;
    else { diag(d, "unknown physical coverage_policy: %s", coverage->as.text); goto fail; }
    if (strcmp(evidence->as.text, "report_unverified") == 0) out->evidence_policy = FISICS_PHYSICAL_EVIDENCE_REPORT_UNVERIFIED;
    else if (strcmp(evidence->as.text, "require_for_source_contracts") == 0) out->evidence_policy = FISICS_PHYSICAL_EVIDENCE_REQUIRE_SOURCE;
    else { diag(d, "unknown source_evidence_policy: %s", evidence->as.text); goto fail; }
    out->project_id = dup_text(project->as.text);
    out->manifest_path = canonical; canonical = NULL;
    char* directory = directory_of(out->manifest_path);
    if (!out->project_id || !directory) { free(directory); diag(d, "out of memory loading physical manifest"); goto fail; }
    out->semantic_graph_path = resolve_path(directory, graph->as.text, d);
    bool ok = out->semantic_graph_path &&
        parse_path_array(fisics_json_object_get(root, "units_metadata_paths"), directory,
                         FISICS_PHYSICAL_PROJECT_MAX_UNITS_RECEIPTS, "units_metadata_paths",
                         &out->units_metadata_paths, &out->units_metadata_path_count, d) &&
        parse_path_array(fisics_json_object_get(root, "physical_catalog_paths"), directory,
                         FISICS_PHYSICAL_PROJECT_MAX_CATALOGS, "physical_catalog_paths",
                         &out->catalog_paths, &out->catalog_path_count, d);
    free(directory);
    if (!ok) goto fail;
    fisics_json_free(root); return true;
fail:
    free(canonical); fisics_json_free(root); fisics_physical_project_manifest_free(out); return false;
}
