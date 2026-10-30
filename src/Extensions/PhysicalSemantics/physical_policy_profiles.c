// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_policy_internal.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"
#include <stdlib.h>
#include <string.h>
static uint32_t kind(const FisicsJsonValue *v, const FisicsPhysicalPolicy *p) {
  return v && v->type == FISICS_JSON_STRING
             ? fisics_physical_policy_find_kind(p, v->as.text)
             : 0;
}
static int compare_origins(const void *a, const void *b) {
  return strcmp(((const FisicsPhysicalOrigin *)a)->id,
                ((const FisicsPhysicalOrigin *)b)->id);
}
bool physical_policy_profiles(const FisicsJsonValue *root,
                              FisicsPhysicalPolicy *p,
                              FisicsPhysicalLoadContext *l,
                              FisicsSemanticProjectDiagnostic *d) {
  const FisicsJsonValue *linear = fisics_json_object_get(root, "linear_kinds");
  const FisicsJsonValue *pairs = fisics_json_object_get(root, "affine_pairs");
  const FisicsJsonValue *origins = fisics_json_object_get(root, "origins");
  if (!linear || linear->type != FISICS_JSON_ARRAY || !pairs ||
      pairs->type != FISICS_JSON_ARRAY || !origins ||
      origins->type != FISICS_JSON_ARRAY) {
    physical_load_diag(d, "physical policy roles must be arrays");
    return false;
  }
  if (linear->as.array.count > l->options.max_kinds ||
      pairs->as.array.count > l->options.max_kinds / 2 ||
      origins->as.array.count > l->options.max_origins) {
    l->failure = PHYSICAL_LOAD_LIMIT;
    physical_load_diag(d, "physical policy role/origin limit exceeded");
    return false;
  }
  p->models = physical_calloc(l, p->kind_count, sizeof(*p->models));
  if (!p->models)
    return false;
  for (size_t i = 0; i < linear->as.array.count; i++) {
    uint32_t k = kind(linear->as.array.items[i], p);
    if (!k) {
      physical_load_diag(d, "unknown physical linear kind at index %zu", i);
      return false;
    }
    if (p->models[k - 1].role) {
      physical_load_diag(d, "duplicate or conflicting physical role for %s",
                         p->kinds[k - 1].id);
      return false;
    }
    p->models[k - 1].role = FISICS_PHYSICAL_LINEAR;
  }
  for (size_t i = 0; i < pairs->as.array.count; i++) {
    const FisicsJsonValue *v = pairs->as.array.items[i];
    const char *const fields[] = {"absolute_kind", "difference_kind"};
    if (!physical_exact_fields(v, fields, 2)) {
      physical_load_diag(d, "invalid physical affine pair");
      return false;
    }
    uint32_t a = kind(fisics_json_object_get(v, "absolute_kind"), p),
             b = kind(fisics_json_object_get(v, "difference_kind"), p);
    if (!a || !b) {
      physical_load_diag(d, "unknown physical affine pair kind");
      return false;
    }
    if (a == b || p->models[a - 1].role || p->models[b - 1].role) {
      physical_load_diag(d, "duplicate or ambiguous physical affine pair role");
      return false;
    }
    if (memcmp(p->kinds[a - 1].dimension, p->kinds[b - 1].dimension, 8)) {
      physical_load_diag(d, "physical affine pair dimensions disagree");
      return false;
    }
    p->models[a - 1] =
        (FisicsPhysicalKindModel){FISICS_PHYSICAL_ABSOLUTE, b, 0};
    p->models[b - 1] =
        (FisicsPhysicalKindModel){FISICS_PHYSICAL_DIFFERENCE, a, 0};
  }
  p->origin_count = origins->as.array.count;
  p->origins = physical_calloc(l, p->origin_count, sizeof(*p->origins));
  if (!p->origins)
    return false;
  for (size_t i = 0; i < p->origin_count; i++) {
    const FisicsJsonValue *v = origins->as.array.items[i];
    const char *const fields[] = {"id", "absolute_kind"};
    const FisicsJsonValue *id = fisics_json_object_get(v, "id");
    uint32_t k = kind(fisics_json_object_get(v, "absolute_kind"), p);
    const char *detail = NULL;
    if (!physical_exact_fields(v, fields, 2) || !id ||
        id->type != FISICS_JSON_STRING ||
        !fisics_semantic_metadata_is_canonical_name(id->as.text, &detail) ||
        !k || p->models[k - 1].role != FISICS_PHYSICAL_ABSOLUTE) {
      physical_load_diag(
          d, "invalid physical origin or unknown/nonabsolute origin kind");
      return false;
    }
    p->origins[i].id = physical_dup_text(l, id->as.text);
    if (!p->origins[i].id)
      return false;
    p->origins[i].absolute_kind = k;
  }
  if (p->origin_count > 1)
    qsort(p->origins, p->origin_count, sizeof(*p->origins), compare_origins);
  for (size_t i = 1; i < p->origin_count; i++)
    if (!strcmp(p->origins[i - 1].id, p->origins[i].id)) {
      physical_load_diag(d, "duplicate physical origin %s", p->origins[i].id);
      return false;
    }
  return true;
}
