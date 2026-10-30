// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_policy_internal.h"
#include "Extensions/PhysicalSemantics/physical_representations.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include "Extensions/PhysicalSemantics/physical_frames.h"
#include "Extensions/PhysicalSemantics/physical_energy.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static char *resolve(const char *parent, const char *input,
                     FisicsPhysicalLoadContext *l,
                     FisicsSemanticProjectDiagnostic *d) {
  if (!input || !input[0]) {
    physical_load_diag(d, "empty physical policy input path");
    return NULL;
  }
  size_t n = strlen(input) + (parent ? strlen(parent) : 0) + 2;
  char *joined = physical_alloc(l, n);
  if (!joined)
    return NULL;
  if (input[0] == '/' || !parent)
    snprintf(joined, n, "%s", input);
  else
    snprintf(joined, n, "%s/%s", parent, input);
  char canonical[PATH_MAX];
  bool ok = realpath(joined, canonical) != NULL;
  int resolve_errno = errno;
  free(joined);
  if (!ok) {
    l->failure =
        resolve_errno == ENOMEM ? PHYSICAL_LOAD_ALLOCATION : PHYSICAL_LOAD_IO;
    physical_load_diag(
        d, "physical policy input does not exist or cannot resolve: %s", input);
    return NULL;
  }
  return physical_dup_text(l, canonical);
}
static int compare_inputs(const void *a, const void *b) {
  return strcmp(((const FisicsPhysicalCatalogInput *)a)->path,
                ((const FisicsPhysicalCatalogInput *)b)->path);
}
bool fisics_physical_policy_load(const char *path, FisicsPhysicalPolicy *out,
                                 FisicsPhysicalPolicyDiagnostic *d) {
  return fisics_physical_policy_load_with_options(
      path, out, fisics_physical_load_defaults(), d);
}
bool fisics_physical_policy_load_with_options(
    const char *path, FisicsPhysicalPolicy *out,
    FisicsPhysicalLoadOptions options,
    FisicsPhysicalPolicyDiagnostic *diagnostic) {
  if (diagnostic)
    memset(diagnostic, 0, sizeof(*diagnostic));
  FisicsSemanticProjectDiagnostic d = {0};
  FisicsPhysicalPolicy p = {0};
  FisicsJsonValue *root = NULL;
  FisicsPhysicalLoadContext load = {.options = options};
  FisicsDiagCode code = FISICS_DIAG_CODE_PHYSICAL_POLICY_INVALID;
  if (!out || !fisics_physical_load_options_valid(options)) {
    physical_load_diag(&d, "invalid physical policy load output/options");
    goto fail;
  }
  p.policy_path = resolve(NULL, path, &load, &d);
  if (!p.policy_path)
    goto fail;
  if (!fisics_physical_read_json(p.policy_path,
                                 FISICS_PHYSICAL_POLICY_MAX_BYTES, &root,
                                 &p.policy_receipt, &load, &d))
    goto fail;
  const char *const fields[] = {
      "schema",       "schema_version", "policy_id", "catalog_paths",
      "linear_kinds", "affine_pairs",   "origins", "operation_rules", "representations", "shape_helpers", "frame_contexts", "reference_points", "frames", "frame_transforms", "energy_forms", "energy_transfers"};
  const FisicsJsonValue *schema = fisics_json_object_get(root, "schema");
  const FisicsJsonValue *id = fisics_json_object_get(root, "policy_id");
  const FisicsJsonValue *paths = fisics_json_object_get(root, "catalog_paths");
  uint64_t version = 0;
  const char *detail = NULL;
  if (!schema ||
      schema->type != FISICS_JSON_STRING ||
      strcmp(schema->as.text, "fisiCs.physical_semantics_policy") ||
      !fisics_json_integer_u64(fisics_json_object_get(root, "schema_version"),
                               &version) ||
      (version != 1 && version != 2 && version != 3 && version != 4 && version != 5 && version != 6 && version != 7) ||
      !physical_exact_fields(root, fields, version==1?7:version==7?16:version==6?14:version==5?10:8) || !id || id->type != FISICS_JSON_STRING ||
      !fisics_semantic_metadata_is_canonical_name(id->as.text, &detail) ||
      !paths || paths->type != FISICS_JSON_ARRAY || !paths->as.array.count) {
    physical_load_diag(
        &d, "invalid physical semantics policy schema/identity/catalog paths");
    goto fail;
  }
  if (paths->as.array.count > options.max_catalogs) {
    load.failure = PHYSICAL_LOAD_LIMIT;
    physical_load_diag(&d, "physical policy catalog path limit exceeded");
    goto fail;
  }
  p.schema_version = (unsigned)version;
  p.policy_id = physical_dup_text(&load, id->as.text);
  if (!p.policy_id)
    goto fail;
  char *parent = physical_dup_text(&load, p.policy_path);
  if (!parent)
    goto fail;
  char *slash = strrchr(parent, '/');
  if (slash == parent)
    slash[1] = 0;
  else if (slash)
    *slash = 0;
  p.catalog_count = paths->as.array.count;
  p.catalogs = physical_calloc(&load, p.catalog_count, sizeof(*p.catalogs));
  if (!p.catalogs) {
    free(parent);
    goto fail;
  }
  for (size_t i = 0; i < p.catalog_count; i++) {
    const FisicsJsonValue *v = paths->as.array.items[i];
    if (v->type != FISICS_JSON_STRING) {
      physical_load_diag(&d, "physical policy catalog path must be a string");
      free(parent);
      goto fail;
    }
    p.catalogs[i].path = resolve(parent, v->as.text, &load, &d);
    if (!p.catalogs[i].path) {
      free(parent);
      goto fail;
    }
  }
  free(parent);
  if (p.catalog_count > 1)
    qsort(p.catalogs, p.catalog_count, sizeof(*p.catalogs), compare_inputs);
  for (size_t i = 1; i < p.catalog_count; i++)
    if (!strcmp(p.catalogs[i - 1].path, p.catalogs[i].path)) {
      physical_load_diag(&d, "duplicate canonical physical catalog path %s",
                         p.catalogs[i].path);
      goto fail;
    }
  code = FISICS_DIAG_CODE_PHYSICAL_POLICY_CATALOG;
  for (size_t i = 0; i < p.catalog_count; i++)
    if (!fisics_physical_catalog_append(
            p.catalogs[i].path, &p.kinds, &p.kind_count, &p.contracts,
            &p.contract_count, &load, &p.catalogs[i].receipt, &d))
      goto fail;
  if (!fisics_physical_catalog_canonicalize(p.kinds, &p.kind_count, &d) ||
      !fisics_physical_catalog_validate(p.kinds, p.kind_count, p.contracts,
                                        p.contract_count, &d))
    goto fail;
  code = FISICS_DIAG_CODE_PHYSICAL_POLICY_REFERENCE;
  if (!physical_policy_profiles(root, &p, &load, &d))
    goto fail;
  if (p.schema_version>=2 && !physical_operations_load(root,&p,&load,&d))
    goto fail;
  if (p.schema_version >= 5 && !physical_representations_load(root, &p, &load, &d))
    goto fail;
  if (p.schema_version >= 6 && !physical_frames_load(root,&p,&load,&d))
    goto fail;
  if (p.schema_version == 7 && !physical_energy_load(root,&p,&load,&d))
    goto fail;
  p.fingerprint = physical_policy_fingerprint(&p);
  fisics_json_free(root);
  fisics_physical_policy_free(out);
  *out = p;
  return true;
fail:
  if (load.failure == PHYSICAL_LOAD_ALLOCATION)
    code = FISICS_DIAG_CODE_PHYSICAL_POLICY_ALLOCATION;
  else if (load.failure == PHYSICAL_LOAD_LIMIT)
    code = FISICS_DIAG_CODE_PHYSICAL_POLICY_LIMIT;
  else if (load.failure == PHYSICAL_LOAD_IO)
    code = FISICS_DIAG_CODE_PHYSICAL_POLICY_IO;
  if (diagnostic) {
    diagnostic->code = code;
    snprintf(diagnostic->message, sizeof(diagnostic->message), "%s",
             d.message[0] ? d.message
                          : (load.failure == PHYSICAL_LOAD_ALLOCATION
                                 ? "physical policy allocation failed"
                                 : "invalid physical policy/catalog"));
  }
  fisics_json_free(root);
  fisics_physical_policy_free(&p);
  return false;
}
