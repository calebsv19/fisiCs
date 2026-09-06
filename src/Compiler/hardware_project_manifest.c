// SPDX-License-Identifier: Apache-2.0

#include "Compiler/hardware_project_manifest.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Compiler/semantic_project_json.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"

static void diag(FisicsSemanticProjectDiagnostic *d, const char *fmt, ...) {
  if (!d || d->message[0])
    return;
  va_list args;
  va_start(args, fmt);
  vsnprintf(d->message, sizeof(d->message), fmt, args);
  va_end(args);
}

static char *dup_text(const char *text) {
  if (!text)
    return NULL;
  size_t n = strlen(text) + 1u;
  char *out = (char *)malloc(n);
  if (out)
    memcpy(out, text, n);
  return out;
}

static bool exact_fields(const FisicsJsonValue *object,
                         const char *const *fields, size_t count) {
  if (!object || object->type != FISICS_JSON_OBJECT ||
      object->as.object.count != count)
    return false;
  for (size_t i = 0; i < count; ++i) {
    if (!fisics_json_object_get(object, fields[i]))
      return false;
  }
  return true;
}

static char *directory_of(const char *path) {
  const char *slash = path ? strrchr(path, '/') : NULL;
  if (!slash)
    return dup_text(".");
  if (slash == path)
    return dup_text("/");
  size_t n = (size_t)(slash - path);
  char *out = (char *)malloc(n + 1u);
  if (out) {
    memcpy(out, path, n);
    out[n] = '\0';
  }
  return out;
}

static char *resolve_path(const char *directory, const char *path,
                          FisicsSemanticProjectDiagnostic *d) {
  if (!path || !path[0])
    return NULL;
  char *joined = NULL;
  if (path[0] == '/') {
    joined = dup_text(path);
  } else {
    size_t a = strlen(directory), b = strlen(path);
    bool slash = a && directory[a - 1u] != '/';
    joined = (char *)malloc(a + b + (slash ? 2u : 1u));
    if (joined) {
      snprintf(joined, a + b + (slash ? 2u : 1u), "%s%s%s", directory,
               slash ? "/" : "", path);
    }
  }
  if (!joined) {
    diag(d, "out of memory resolving hardware project path");
    return NULL;
  }
  char *resolved = realpath(joined, NULL);
  free(joined);
  if (!resolved)
    diag(d, "hardware project input does not exist: %s", path);
  return resolved;
}

static char *read_file(const char *path, size_t *out_length,
                       FisicsSemanticProjectDiagnostic *d) {
  struct stat st;
  if (!path || stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
    diag(d, "hardware project manifest is not a regular file: %s",
         path ? path : "<null>");
    return NULL;
  }
  if (st.st_size < 0 ||
      (uint64_t)st.st_size > FISICS_HARDWARE_PROJECT_MANIFEST_MAX_BYTES) {
    diag(d, "hardware project manifest exceeds %u bytes: %s",
         FISICS_HARDWARE_PROJECT_MANIFEST_MAX_BYTES, path);
    return NULL;
  }
  FILE *file = fopen(path, "rb");
  if (!file) {
    diag(d, "failed to read hardware project manifest: %s", path);
    return NULL;
  }
  size_t length = (size_t)st.st_size;
  char *bytes = (char *)malloc(length + 1u);
  bool ok = bytes && fread(bytes, 1u, length, file) == length &&
            fgetc(file) == EOF && !ferror(file);
  if (fclose(file) != 0)
    ok = false;
  if (!ok) {
    free(bytes);
    diag(d, "failed to read complete hardware project manifest: %s", path);
    return NULL;
  }
  bytes[length] = '\0';
  *out_length = length;
  return bytes;
}

static int compare_strings(const void *left, const void *right) {
  return strcmp(*(const char *const *)left, *(const char *const *)right);
}

void fisics_hardware_project_manifest_free(FisicsHardwareProjectManifest *m) {
  if (!m)
    return;
  free(m->project_id);
  free(m->manifest_path);
  free(m->physical_graph_path);
  for (size_t i = 0; i < m->catalog_path_count; ++i)
    free(m->catalog_paths[i]);
  free(m->catalog_paths);
  free(m->topology_path);
  memset(m, 0, sizeof(*m));
}

bool fisics_hardware_project_manifest_load(const char *path,
                                           FisicsHardwareProjectManifest *out,
                                           FisicsSemanticProjectDiagnostic *d) {
  if (d)
    d->message[0] = '\0';
  if (!path || !path[0] || !out) {
    diag(d, "invalid hardware project manifest arguments");
    return false;
  }
  memset(out, 0, sizeof(*out));
  char *canonical = realpath(path, NULL);
  if (!canonical) {
    diag(d, "hardware project manifest does not exist: %s", path);
    return false;
  }
  size_t length = 0;
  char *bytes = read_file(canonical, &length, d);
  if (!bytes) {
    free(canonical);
    return false;
  }
  FisicsJsonValue *root = NULL;
  FisicsJsonDiagnostic jd = {0};
  if (!fisics_json_parse(bytes, length, &root, &jd)) {
    diag(d, "invalid hardware project manifest %s: %s", canonical, jd.message);
    free(bytes);
    free(canonical);
    return false;
  }
  free(bytes);
  static const char *const fields[] = {"schema",
                                       "schema_version",
                                       "project_id",
                                       "physical_project_graph_path",
                                       "hardware_catalog_paths",
                                       "hardware_topology_path",
                                       "physical_evidence_policy",
                                       "required_port_policy"};
  if (!exact_fields(root, fields, 8u)) {
    diag(d,
         "hardware project manifest must contain exactly the eight v1 fields");
    goto fail;
  }
  uint64_t version = 0;
  const FisicsJsonValue *schema = fisics_json_object_get(root, "schema");
  const FisicsJsonValue *project = fisics_json_object_get(root, "project_id");
  const FisicsJsonValue *graph =
      fisics_json_object_get(root, "physical_project_graph_path");
  const FisicsJsonValue *catalogs =
      fisics_json_object_get(root, "hardware_catalog_paths");
  const FisicsJsonValue *topology =
      fisics_json_object_get(root, "hardware_topology_path");
  const FisicsJsonValue *evidence =
      fisics_json_object_get(root, "physical_evidence_policy");
  const FisicsJsonValue *ports =
      fisics_json_object_get(root, "required_port_policy");
  const char *detail = NULL;
  if (!schema || schema->type != FISICS_JSON_STRING ||
      strcmp(schema->as.text, "fisiCs.hardware_project_manifest") != 0 ||
      !fisics_json_integer_u64(fisics_json_object_get(root, "schema_version"),
                               &version) ||
      version != 1u) {
    diag(d, "hardware project manifest requires "
            "fisiCs.hardware_project_manifest schema v1");
    goto fail;
  }
  if (!project || project->type != FISICS_JSON_STRING ||
      !fisics_semantic_metadata_is_canonical_name(project->as.text, &detail)) {
    diag(d, "invalid hardware project_id: %s",
         detail ? detail : "expected canonical string");
    goto fail;
  }
  if (!graph || graph->type != FISICS_JSON_STRING || !graph->as.text[0] ||
      !topology || topology->type != FISICS_JSON_STRING ||
      !topology->as.text[0] || !evidence ||
      evidence->type != FISICS_JSON_STRING || !ports ||
      ports->type != FISICS_JSON_STRING || !catalogs ||
      catalogs->type != FISICS_JSON_ARRAY || catalogs->as.array.count == 0u ||
      catalogs->as.array.count > FISICS_HARDWARE_PROJECT_MAX_CATALOGS) {
    diag(d, "hardware project manifest paths, policies, or catalog count are "
            "invalid");
    goto fail;
  }
  if (strcmp(evidence->as.text, "require_source_compatible") == 0) {
    out->evidence_policy = FISICS_HARDWARE_EVIDENCE_REQUIRE_SOURCE;
  } else if (strcmp(evidence->as.text, "allow_declared") == 0) {
    out->evidence_policy = FISICS_HARDWARE_EVIDENCE_ALLOW_DECLARED;
  } else {
    diag(d, "unknown physical_evidence_policy: %s", evidence->as.text);
    goto fail;
  }
  if (strcmp(ports->as.text, "require_all") == 0) {
    out->port_policy = FISICS_HARDWARE_PORTS_REQUIRE_ALL;
  } else if (strcmp(ports->as.text, "report_unbound") == 0) {
    out->port_policy = FISICS_HARDWARE_PORTS_REPORT_UNBOUND;
  } else {
    diag(d, "unknown required_port_policy: %s", ports->as.text);
    goto fail;
  }
  out->project_id = dup_text(project->as.text);
  out->manifest_path = canonical;
  canonical = NULL;
  char *directory = directory_of(out->manifest_path);
  if (!out->project_id || !directory) {
    free(directory);
    diag(d, "out of memory loading hardware project manifest");
    goto fail;
  }
  out->physical_graph_path = resolve_path(directory, graph->as.text, d);
  out->topology_path = resolve_path(directory, topology->as.text, d);
  out->catalog_paths =
      (char **)calloc(catalogs->as.array.count, sizeof(char *));
  if (!out->physical_graph_path || !out->topology_path || !out->catalog_paths) {
    free(directory);
    if (!d || !d->message[0])
      diag(d, "out of memory loading hardware project paths");
    goto fail;
  }
  out->catalog_path_count = catalogs->as.array.count;
  for (size_t i = 0; i < out->catalog_path_count; ++i) {
    const FisicsJsonValue *item = catalogs->as.array.items[i];
    if (!item || item->type != FISICS_JSON_STRING || !item->as.text[0]) {
      free(directory);
      diag(d, "hardware_catalog_paths[%zu] must be a non-empty string", i);
      goto fail;
    }
    out->catalog_paths[i] = resolve_path(directory, item->as.text, d);
    if (!out->catalog_paths[i]) {
      free(directory);
      goto fail;
    }
  }
  free(directory);
  qsort(out->catalog_paths, out->catalog_path_count, sizeof(char *),
        compare_strings);
  for (size_t i = 1; i < out->catalog_path_count; ++i) {
    if (strcmp(out->catalog_paths[i - 1u], out->catalog_paths[i]) == 0) {
      diag(d, "duplicate hardware catalog path: %s", out->catalog_paths[i]);
      goto fail;
    }
  }
  if (strcmp(out->physical_graph_path, out->topology_path) == 0) {
    diag(d, "hardware topology path aliases physical graph input: %s",
         out->topology_path);
    goto fail;
  }
  if (strcmp(out->physical_graph_path, out->manifest_path) == 0 ||
      strcmp(out->topology_path, out->manifest_path) == 0) {
    diag(d, "hardware input path aliases manifest input");
    goto fail;
  }
  for (size_t i = 0; i < out->catalog_path_count; ++i) {
    if (strcmp(out->catalog_paths[i], out->physical_graph_path) == 0 ||
        strcmp(out->catalog_paths[i], out->topology_path) == 0 ||
        strcmp(out->catalog_paths[i], out->manifest_path) == 0) {
      diag(d, "hardware catalog path aliases another input: %s",
           out->catalog_paths[i]);
      goto fail;
    }
  }
  fisics_json_free(root);
  return true;

fail:
  free(canonical);
  fisics_json_free(root);
  fisics_hardware_project_manifest_free(out);
  return false;
}
