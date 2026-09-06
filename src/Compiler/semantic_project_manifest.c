// SPDX-License-Identifier: Apache-2.0

#include "Compiler/semantic_project_manifest.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

static char* read_bounded_file(const char* path,
                               size_t maximum,
                               size_t* out_length,
                               FisicsSemanticProjectDiagnostic* diagnostic) {
    struct stat status;
    if (!path || stat(path, &status) != 0 || !S_ISREG(status.st_mode)) {
        set_diagnostic(diagnostic, "semantic project manifest is not a regular file: %s",
                       path ? path : "<null>");
        return NULL;
    }
    if (status.st_size < 0 || (uint64_t)status.st_size > maximum) {
        set_diagnostic(diagnostic, "semantic project manifest exceeds %zu bytes: %s",
                       maximum, path);
        return NULL;
    }
    FILE* file = fopen(path, "rb");
    if (!file) {
        set_diagnostic(diagnostic, "failed to read semantic project manifest: %s", path);
        return NULL;
    }
    size_t length = (size_t)status.st_size;
    char* bytes = (char*)malloc(length + 1u);
    if (!bytes) {
        fclose(file);
        set_diagnostic(diagnostic, "out of memory reading semantic project manifest: %s", path);
        return NULL;
    }
    size_t read_count = fread(bytes, 1u, length, file);
    int extra = fgetc(file);
    bool ok = read_count == length && extra == EOF && !ferror(file);
    if (fclose(file) != 0) ok = false;
    if (!ok) {
        free(bytes);
        set_diagnostic(diagnostic, "failed to read complete semantic project manifest: %s", path);
        return NULL;
    }
    bytes[length] = '\0';
    if (out_length) *out_length = length;
    return bytes;
}

static char* path_directory(const char* path) {
    const char* slash = path ? strrchr(path, '/') : NULL;
    if (!slash) return duplicate_string(".");
    if (slash == path) return duplicate_string("/");
    size_t length = (size_t)(slash - path);
    char* directory = (char*)malloc(length + 1u);
    if (directory) {
        memcpy(directory, path, length);
        directory[length] = '\0';
    }
    return directory;
}

static char* resolve_existing_path(const char* directory,
                                   const char* path,
                                   FisicsSemanticProjectDiagnostic* diagnostic) {
    if (!path || !path[0]) return NULL;
    char* joined = NULL;
    if (path[0] == '/') {
        joined = duplicate_string(path);
    } else {
        size_t directory_length = strlen(directory);
        size_t path_length = strlen(path);
        bool slash = directory_length && directory[directory_length - 1u] != '/';
        joined = (char*)malloc(directory_length + (slash ? 1u : 0u) + path_length + 1u);
        if (joined) {
            memcpy(joined, directory, directory_length);
            size_t position = directory_length;
            if (slash) joined[position++] = '/';
            memcpy(joined + position, path, path_length + 1u);
        }
    }
    if (!joined) {
        set_diagnostic(diagnostic, "out of memory resolving semantic project path");
        return NULL;
    }
    char* resolved = realpath(joined, NULL);
    free(joined);
    if (!resolved) {
        set_diagnostic(diagnostic, "semantic metadata artifact does not exist: %s", path);
    }
    return resolved;
}

static bool object_has_exact_fields(const FisicsJsonValue* object,
                                    const char* const* fields,
                                    size_t field_count) {
    if (!object || object->type != FISICS_JSON_OBJECT ||
        object->as.object.count != field_count) return false;
    for (size_t i = 0; i < field_count; ++i) {
        if (!fisics_json_object_get(object, fields[i])) return false;
    }
    return true;
}

static int compare_units(const void* left, const void* right) {
    const FisicsSemanticProjectManifestUnit* a =
        (const FisicsSemanticProjectManifestUnit*)left;
    const FisicsSemanticProjectManifestUnit* b =
        (const FisicsSemanticProjectManifestUnit*)right;
    return strcmp(a->metadata_path, b->metadata_path);
}

static int compare_strings(const void* left, const void* right) {
    const char* const* a = (const char* const*)left;
    const char* const* b = (const char* const*)right;
    return strcmp(*a, *b);
}

void fisics_semantic_project_manifest_free(FisicsSemanticProjectManifest* manifest) {
    if (!manifest) return;
    free(manifest->project_id);
    free(manifest->manifest_path);
    for (size_t i = 0; i < manifest->translation_unit_count; ++i) {
        free(manifest->translation_units[i].metadata_path);
    }
    free(manifest->translation_units);
    for (size_t i = 0; i < manifest->allowed_external_target_count; ++i) {
        free(manifest->allowed_external_targets[i]);
    }
    free(manifest->allowed_external_targets);
    memset(manifest, 0, sizeof(*manifest));
}

bool fisics_semantic_project_manifest_load(
    const char* path,
    FisicsSemanticProjectManifest* out,
    FisicsSemanticProjectDiagnostic* diagnostic) {
    if (diagnostic) diagnostic->message[0] = '\0';
    if (!path || !path[0] || !out) {
        set_diagnostic(diagnostic, "invalid semantic project manifest arguments");
        return false;
    }
    memset(out, 0, sizeof(*out));
    char* manifest_path = realpath(path, NULL);
    if (!manifest_path) {
        set_diagnostic(diagnostic, "semantic project manifest does not exist: %s", path);
        return false;
    }
    size_t length = 0;
    char* bytes = read_bounded_file(manifest_path,
                                    FISICS_SEMANTIC_PROJECT_MANIFEST_MAX_BYTES,
                                    &length,
                                    diagnostic);
    if (!bytes) {
        free(manifest_path);
        return false;
    }
    FisicsJsonValue* root = NULL;
    FisicsJsonDiagnostic json_diagnostic = {0};
    if (!fisics_json_parse(bytes, length, &root, &json_diagnostic)) {
        set_diagnostic(diagnostic, "invalid semantic project manifest %s: %s",
                       manifest_path,
                       json_diagnostic.message[0] ? json_diagnostic.message : "invalid JSON");
        free(bytes);
        free(manifest_path);
        return false;
    }
    free(bytes);
    static const char* const root_fields[] = {
        "schema", "schema_version", "project_id", "translation_units",
        "allowed_external_targets"
    };
    if (!object_has_exact_fields(root, root_fields,
                                 sizeof(root_fields) / sizeof(root_fields[0]))) {
        set_diagnostic(diagnostic,
                       "semantic project manifest must contain exactly schema, schema_version, project_id, translation_units, and allowed_external_targets");
        goto fail;
    }
    const FisicsJsonValue* schema = fisics_json_object_get(root, "schema");
    const FisicsJsonValue* schema_version = fisics_json_object_get(root, "schema_version");
    const FisicsJsonValue* project_id = fisics_json_object_get(root, "project_id");
    const FisicsJsonValue* units = fisics_json_object_get(root, "translation_units");
    const FisicsJsonValue* external = fisics_json_object_get(root, "allowed_external_targets");
    uint64_t version = 0;
    if (!schema || schema->type != FISICS_JSON_STRING ||
        strcmp(schema->as.text, "fisiCs.semantic_project_manifest") != 0) {
        set_diagnostic(diagnostic,
                       "semantic project manifest schema must be fisiCs.semantic_project_manifest");
        goto fail;
    }
    if (!fisics_json_integer_u64(schema_version, &version) || version != 1u) {
        set_diagnostic(diagnostic, "semantic project manifest schema_version must be 1");
        goto fail;
    }
    const char* name_detail = NULL;
    if (!project_id || project_id->type != FISICS_JSON_STRING ||
        !fisics_semantic_metadata_is_canonical_name(project_id->as.text, &name_detail)) {
        set_diagnostic(diagnostic, "invalid semantic project_id: %s",
                       name_detail ? name_detail : "expected canonical string");
        goto fail;
    }
    if (!units || units->type != FISICS_JSON_ARRAY || units->as.array.count == 0u ||
        units->as.array.count > FISICS_SEMANTIC_PROJECT_MAX_TRANSLATION_UNITS) {
        set_diagnostic(diagnostic, "translation_units must contain 1 through %u items",
                       FISICS_SEMANTIC_PROJECT_MAX_TRANSLATION_UNITS);
        goto fail;
    }
    if (!external || external->type != FISICS_JSON_ARRAY ||
        external->as.array.count > FISICS_SEMANTIC_PROJECT_MAX_EXTERNAL_TARGETS) {
        set_diagnostic(diagnostic, "allowed_external_targets must be an array with at most %u items",
                       FISICS_SEMANTIC_PROJECT_MAX_EXTERNAL_TARGETS);
        goto fail;
    }
    out->project_id = duplicate_string(project_id->as.text);
    out->manifest_path = manifest_path;
    manifest_path = NULL;
    out->translation_units = (FisicsSemanticProjectManifestUnit*)calloc(
        units->as.array.count, sizeof(*out->translation_units));
    out->translation_unit_count = units->as.array.count;
    out->allowed_external_targets = (char**)calloc(
        external->as.array.count ? external->as.array.count : 1u,
        sizeof(*out->allowed_external_targets));
    out->allowed_external_target_count = external->as.array.count;
    if (!out->project_id || !out->translation_units || !out->allowed_external_targets) {
        set_diagnostic(diagnostic, "out of memory loading semantic project manifest");
        goto fail;
    }
    char* directory = path_directory(out->manifest_path);
    if (!directory) {
        set_diagnostic(diagnostic, "out of memory resolving semantic project manifest directory");
        goto fail;
    }
    static const char* const unit_fields[] = {"metadata_path"};
    for (size_t i = 0; i < units->as.array.count; ++i) {
        const FisicsJsonValue* unit = units->as.array.items[i];
        if (!object_has_exact_fields(unit, unit_fields, 1u)) {
            set_diagnostic(diagnostic,
                           "translation_units[%zu] must contain exactly metadata_path", i);
            free(directory);
            goto fail;
        }
        const FisicsJsonValue* metadata_path =
            fisics_json_object_get(unit, "metadata_path");
        if (!metadata_path || metadata_path->type != FISICS_JSON_STRING ||
            !metadata_path->as.text[0]) {
            set_diagnostic(diagnostic,
                           "translation_units[%zu].metadata_path must be a non-empty string", i);
            free(directory);
            goto fail;
        }
        out->translation_units[i].metadata_path =
            resolve_existing_path(directory, metadata_path->as.text, diagnostic);
        if (!out->translation_units[i].metadata_path) {
            free(directory);
            goto fail;
        }
    }
    free(directory);
    for (size_t i = 0; i < external->as.array.count; ++i) {
        const FisicsJsonValue* target = external->as.array.items[i];
        name_detail = NULL;
        if (!target || target->type != FISICS_JSON_STRING ||
            !fisics_semantic_metadata_is_canonical_name(target->as.text, &name_detail)) {
            set_diagnostic(diagnostic, "invalid allowed_external_targets[%zu]: %s", i,
                           name_detail ? name_detail : "expected canonical string");
            goto fail;
        }
        out->allowed_external_targets[i] = duplicate_string(target->as.text);
        if (!out->allowed_external_targets[i]) {
            set_diagnostic(diagnostic, "out of memory loading allowed external targets");
            goto fail;
        }
    }
    qsort(out->translation_units, out->translation_unit_count,
          sizeof(*out->translation_units), compare_units);
    qsort(out->allowed_external_targets, out->allowed_external_target_count,
          sizeof(*out->allowed_external_targets), compare_strings);
    for (size_t i = 1; i < out->translation_unit_count; ++i) {
        if (strcmp(out->translation_units[i - 1u].metadata_path,
                   out->translation_units[i].metadata_path) == 0) {
            set_diagnostic(diagnostic, "duplicate semantic metadata artifact path: %s",
                           out->translation_units[i].metadata_path);
            goto fail;
        }
    }
    for (size_t i = 1; i < out->allowed_external_target_count; ++i) {
        if (strcmp(out->allowed_external_targets[i - 1u],
                   out->allowed_external_targets[i]) == 0) {
            set_diagnostic(diagnostic, "duplicate allowed external target: %s",
                           out->allowed_external_targets[i]);
            goto fail;
        }
    }
    fisics_json_free(root);
    return true;

fail:
    free(manifest_path);
    fisics_json_free(root);
    fisics_semantic_project_manifest_free(out);
    return false;
}
