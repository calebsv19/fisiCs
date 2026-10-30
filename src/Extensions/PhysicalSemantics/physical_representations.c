// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_representations.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
static const FisicsJsonValue *get(const FisicsJsonValue *j, const char *k) {
  return fisics_json_object_get(j, k);
}
static const char *str(const FisicsJsonValue *j) {
  return j && j->type == FISICS_JSON_STRING ? j->as.text : NULL;
}
static bool eq(const FisicsJsonValue *j, const char *s) {
  const char *v = str(j);
  return v && !strcmp(v, s);
}
static bool name(const char *s) {
  const char *d = NULL;
  return s && fisics_semantic_metadata_is_canonical_name(s, &d);
}
static uint64_t num(uint64_t h, uint64_t n) {
  for (unsigned i = 0; i < 8; i++) {
    h = (h ^ (unsigned char)n) * UINT64_C(1099511628211);
    n >>= 8;
  }
  return h;
}
static uint64_t txt(uint64_t h, const char *s) {
  h = num(h, strlen(s));
  for (; *s; s++)
    h = (h ^ (unsigned char)*s) * UINT64_C(1099511628211);
  return h;
}
static int rep_order(const void *a, const void *b) {
  return strcmp(((const FisicsPhysicalRepresentation *)a)->id,
                ((const FisicsPhysicalRepresentation *)b)->id);
}
static int helper_order(const void *a, const void *b) {
  return strcmp(((const FisicsPhysicalShapeHelper *)a)->id,
                ((const FisicsPhysicalShapeHelper *)b)->id);
}
uint32_t physical_representation_find(const FisicsPhysicalPolicy *p,
                                      const char *id) {
  if (!p || !id)
    return 0;
  for (size_t i = 0; i < p->representation_count; i++)
    if (!strcmp(p->representations[i].id, id))
      return (uint32_t)i + 1;
  return 0;
}
const FisicsPhysicalShapeHelper *
physical_shape_helper_find(const FisicsPhysicalPolicy *p, const char *id) {
  if (p && id)
    for (size_t i = 0; i < p->shape_helper_count; i++)
      if (!strcmp(p->shape_helpers[i].id, id))
        return &p->shape_helpers[i];
  return NULL;
}
void physical_representations_free(FisicsPhysicalPolicy *p) {
  for (size_t i = 0; p->representations && i < p->representation_count; i++) {
    free(p->representations[i].id);
    for (unsigned k = 0; k < 3; k++)
      free(p->representations[i].fields[k]);
  }
  free(p->representations);
  for (size_t i = 0; p->shape_helpers && i < p->shape_helper_count; i++)
    free(p->shape_helpers[i].id);
  free(p->shape_helpers);
}
static bool linear(const FisicsPhysicalPolicy *p, uint32_t k) {
  return k && (p->models[k - 1].role == FISICS_PHYSICAL_LINEAR ||
               p->models[k - 1].role == FISICS_PHYSICAL_DIFFERENCE);
}
static bool kind(const FisicsPhysicalPolicy *p, uint32_t k, const char *s,
                 const int8_t d[8]) {
  return k && !strcmp(p->kinds[k - 1].id, s) &&
         !memcmp(p->kinds[k - 1].dimension, d, 8);
}
static bool contract(const FisicsPhysicalPolicy *p,
                     const FisicsPhysicalShapeHelper *h) {
  uint32_t a = h->kinds[0], b = h->kinds[1], k = h->kinds[2],
           ar = h->representations[0], br = h->representations[1],
           kr = h->representations[2];
  if (physical_cosmic_shape_contract(p,h)) return true;
  if (p->schema_version>=6 && ar && ar==br && ar==kr) {
    const FisicsPhysicalRepresentation *v=&p->representations[ar-1];
    uint32_t position=fisics_physical_policy_find_kind(p,"quantity.position");
    uint32_t delta=position?p->models[position-1].paired_kind:0;
    if(v->record && !v->single_precision && v->rank==1 && v->extent==3 && !v->axial && position && delta) {
      if(h->operation==PHYSICAL_SHAPE_SUB && a==position && b==position && k==delta) return true;
      if((h->operation==PHYSICAL_SHAPE_ADD || h->operation==PHYSICAL_SHAPE_SUB) && a==position && b==delta && k==position) return true;
      if(h->operation==PHYSICAL_SHAPE_ADD && a==delta && b==position && k==position) return true;
    }
  }
  if (!linear(p, a) || !linear(p, b) || !linear(p, k) || !ar)
    return false;
  const FisicsPhysicalRepresentation *r = &p->representations[ar - 1];
  /* Helpers pass records by value; array parameters decay and are not admitted.
   */
  if (!r->record || r->rank != 1)
    return false;
  if (h->operation == PHYSICAL_SHAPE_ADD || h->operation == PHYSICAL_SHAPE_SUB)
    return a == b && a == k && ar == br && ar == kr;
  static const int8_t zero[8] = {0}, force[8] = {1, 1, -2}, length[8] = {1},
                      energy[8] = {2, 1, -2};
  if (h->operation == PHYSICAL_SHAPE_SCALE)
    return kind(p, b, "quantity.ratio", zero) && !br && a == k && ar == kr;
  if (!br)
    return false;
  const FisicsPhysicalRepresentation *s = &p->representations[br - 1];
  if (!s->record || s->rank != 1 || s->extent != r->extent ||
      s->single_precision != r->single_precision || r->axial || s->axial)
    return false;
  if (h->operation == PHYSICAL_SHAPE_DOT)
    return kind(p, a, "quantity.force", force) &&
           kind(p, b, "quantity.displacement", length) &&
           kind(p, k, "quantity.energy", energy) && !kr;
  if (h->operation == PHYSICAL_SHAPE_CROSS && kr) {
    const FisicsPhysicalRepresentation *t = &p->representations[kr - 1];
    return r->extent == 3 && t->record && t->rank == 1 && t->extent == 3 &&
           t->axial && t->single_precision == r->single_precision &&
           kind(p, a, "quantity.displacement", length) &&
           kind(p, b, "quantity.force", force) &&
           kind(p, k, "quantity.torque", energy);
  }
  return false;
}
bool physical_representations_load(const FisicsJsonValue *root,
                                   FisicsPhysicalPolicy *p,
                                   FisicsPhysicalLoadContext *l,
                                   FisicsSemanticProjectDiagnostic *d) {
  const FisicsJsonValue *reps = get(root, "representations"),
                        *helpers = get(root, "shape_helpers");
  if (!reps || reps->type != FISICS_JSON_ARRAY || !helpers ||
      helpers->type != FISICS_JSON_ARRAY)
    goto invalid;
  if (reps->as.array.count > PHYSICAL_MAX_REPRESENTATIONS ||
      helpers->as.array.count > PHYSICAL_MAX_SHAPE_HELPERS) {
    l->failure = PHYSICAL_LOAD_LIMIT;
    goto invalid;
  }
  p->representation_count = reps->as.array.count;
  p->shape_helper_count = helpers->as.array.count;
  if (p->representation_count &&
      !(p->representations = physical_calloc(l, p->representation_count,
                                             sizeof(*p->representations))))
    return false;
  const char *const rf[] = {"id",     "storage", "scalar", "rank",
                            "extent", "fields",  "parity"};
  for (size_t i = 0; i < p->representation_count; i++) {
    const FisicsJsonValue *j = reps->as.array.items[i],
                          *fields = get(j, "fields");
    const char *id = str(get(j, "id"));
    int rank = 0, extent = 0;
    if (!physical_exact_fields(j, rf, 7) || !name(id) ||
        (!eq(get(j, "storage"), "record") && !eq(get(j, "storage"), "array")) ||
        (!eq(get(j, "scalar"), "float") && !eq(get(j, "scalar"), "double")) ||
        !fisics_json_integer_int(get(j, "rank"), &rank) ||
        !fisics_json_integer_int(get(j, "extent"), &extent) ||
        (rank != 1 && rank != 2) || (extent != 2 && extent != 3) || !fields ||
        fields->type != FISICS_JSON_ARRAY)
      goto invalid;
    FisicsPhysicalRepresentation *r = &p->representations[i];
    r->record = eq(get(j, "storage"), "record");
    r->single_precision = eq(get(j, "scalar"), "float");
    r->axial = eq(get(j, "parity"), "axial");
    r->rank = rank;
    r->extent = extent;
    if ((rank == 1 && !eq(get(j, "parity"), "polar") && !r->axial) ||
        (rank == 2 && !eq(get(j, "parity"), "tensor")) ||
        (r->axial && extent != 3) ||
        fields->as.array.count !=
            (r->record ? (rank == 1 ? (size_t)extent : 1) : 0))
      goto invalid;
    r->id = physical_dup_text(l, id);
    if (!r->id)
      return false;
    r->field_count = (unsigned)fields->as.array.count;
    uint64_t h = txt(UINT64_C(1469598103934665603), "fisiCs.representation.v1");
    h = txt(h, id);
    h = num(h, r->record);
    h = num(h, r->single_precision);
    h = num(h, r->axial);
    h = num(h, rank);
    h = num(h, extent);
    h = num(h, r->field_count);
    for (unsigned k = 0; k < r->field_count; k++) {
      const char *s = str(fields->as.array.items[k]);
      if (!name(s) || strchr(s, '.'))
        goto invalid;
      for (unsigned a = 0; a < k; a++)
        if (!strcmp(s, r->fields[a]))
          goto invalid;
      r->fields[k] = physical_dup_text(l, s);
      if (!r->fields[k])
        return false;
      h = txt(h, s);
    }
    r->fingerprint = h;
  }
  if (p->representation_count > 1)
    qsort(p->representations, p->representation_count,
          sizeof(*p->representations), rep_order);
  for (size_t i = 1; i < p->representation_count; i++)
    if (!strcmp(p->representations[i - 1].id, p->representations[i].id))
      goto invalid;
  if (p->shape_helper_count &&
      !(p->shape_helpers = physical_calloc(l, p->shape_helper_count,
                                           sizeof(*p->shape_helpers))))
    return false;
  const char *const hf[] = {"id",
                            "operation",
                            "operand_kinds",
                            "operand_representations",
                            "result_kind",
                            "result_representation",
                            "preconditions",
                            "provenance"};
  static const char *ops[] = {"", "add", "subtract", "scale", "dot", "cross", "norm"};
  for (size_t i = 0; i < p->shape_helper_count; i++) {
    const FisicsJsonValue *j = helpers->as.array.items[i],
                          *ks = get(j, "operand_kinds"),
                          *rs = get(j, "operand_representations");
    const char *id = str(get(j, "id"));
    unsigned arity = eq(get(j,"operation"),"norm") && p->schema_version==7 ? 1u : 2u;
    if (!physical_exact_fields(j, hf, 8) || !name(id) || !ks ||
        ks->type != FISICS_JSON_ARRAY || ks->as.array.count != arity || !rs ||
        rs->type != FISICS_JSON_ARRAY || rs->as.array.count != arity ||
        !eq(get(j, "preconditions"), "common_cartesian_basis_right_handed") ||
        !eq(get(j, "provenance"), "declared_implementation_and_basis"))
      goto invalid;
    FisicsPhysicalShapeHelper *h = &p->shape_helpers[i];
    for (unsigned k = 1; k <= 6; k++)
      if (eq(get(j, "operation"), ops[k]))
        h->operation = (PhysicalShapeOperation)k;
    for (unsigned k = 0; k < 3; k++) {
      if(arity==1 && k==1) continue;
      const FisicsJsonValue *kj = k < 2 ? ks->as.array.items[k]
                                        : get(j, "result_kind"),
                            *rj = k < 2 ? rs->as.array.items[k]
                                        : get(j, "result_representation");
      h->kinds[k] = fisics_physical_policy_find_kind(p, str(kj));
      if (!h->kinds[k] || !rj)
        goto invalid;
      if (rj->type != FISICS_JSON_NULL) {
        h->representations[k] = physical_representation_find(p, str(rj));
        if (!h->representations[k])
          goto invalid;
      }
    }
    if (!h->operation || physical_constructor_rule(p,id) || !contract(p, h))
      goto invalid;
    h->id = physical_dup_text(l, id);
    if (!h->id)
      return false;
    uint64_t hash =
        txt(UINT64_C(1469598103934665603), "fisiCs.shape_helper.v1");
    hash = txt(hash, id);
    hash = num(hash, h->operation);
    for (unsigned k = 0; k < 3; k++) {
      if(arity==1 && k==1) {hash=num(hash,0);continue;}
      hash = txt(hash, p->kinds[h->kinds[k] - 1].id);
      hash = num(hash, p->models[h->kinds[k] - 1].role);
      hash = num(hash,
                 h->representations[k]
                     ? p->representations[h->representations[k] - 1].fingerprint
                     : 0);
    }
    hash = txt(hash, "common_cartesian_basis_right_handed");
    h->fingerprint = txt(hash, "declared_implementation_and_basis");
  }
  if (p->shape_helper_count > 1)
    qsort(p->shape_helpers, p->shape_helper_count, sizeof(*p->shape_helpers),
          helper_order);
  for (size_t i = 1; i < p->shape_helper_count; i++)
    if (!strcmp(p->shape_helpers[i - 1].id, p->shape_helpers[i].id))
      goto invalid;
  uint64_t hash =
      txt(UINT64_C(1469598103934665603), "fisiCs.representation_model.v1");
  hash = num(hash, p->representation_count);
  for (size_t i = 0; i < p->representation_count; i++)
    hash = num(hash, p->representations[i].fingerprint);
  hash = num(hash, p->shape_helper_count);
  for (size_t i = 0; i < p->shape_helper_count; i++)
    hash = num(hash, p->shape_helpers[i].fingerprint);
  p->representation_fingerprint = hash;
  return true;
invalid:
  physical_load_diag(d, "invalid, duplicate or unsupported physical "
                        "representation/helper contract");
  return false;
}
void physical_representations_write(FILE *out, const FisicsPhysicalPolicy *p) {
  fprintf(out,
          "{\"version\":1,\"fingerprint\":\"%016" PRIx64
          "\",\"representations\":[",
          p->representation_fingerprint);
  for (size_t i = 0; i < p->representation_count; i++) {
    const FisicsPhysicalRepresentation *r = &p->representations[i];
    fprintf(out, "%s{\"id\":\"%s\",\"fingerprint\":\"%016" PRIx64 "\"}",
            i ? "," : "", r->id, r->fingerprint);
  }
  fputs("],\"helpers\":[", out);
  for (size_t i = 0; i < p->shape_helper_count; i++)
    fprintf(out, "%s{\"id\":\"%s\",\"fingerprint\":\"%016" PRIx64 "\"}",
            i ? "," : "", p->shape_helpers[i].id,
            p->shape_helpers[i].fingerprint);
  fputs("]}", out);
}
bool physical_representations_match(const FisicsJsonValue *j,
                                    const FisicsPhysicalPolicy *p) {
  const char *const fs[] = {"version", "fingerprint", "representations",
                            "helpers"},
                    *const entry[] = {"id", "fingerprint"};
  int v = 0;
  char fp[17];
  snprintf(fp, sizeof(fp), "%016" PRIx64, p->representation_fingerprint);
  if (!physical_exact_fields(j, fs, 4) ||
      !fisics_json_integer_int(get(j, "version"), &v) || v != 1 ||
      !eq(get(j, "fingerprint"), fp))
    return false;
  for (unsigned k = 0; k < 2; k++) {
    const FisicsJsonValue *a = get(j, k ? "helpers" : "representations");
    size_t count = k ? p->shape_helper_count : p->representation_count;
    if (!a || a->type != FISICS_JSON_ARRAY || a->as.array.count != count)
      return false;
    for (size_t i = 0; i < count; i++) {
      snprintf(fp, sizeof(fp), "%016" PRIx64,
               k ? p->shape_helpers[i].fingerprint
                 : p->representations[i].fingerprint);
      if (!physical_exact_fields(a->as.array.items[i], entry, 2) ||
          !eq(get(a->as.array.items[i], "id"),
              k ? p->shape_helpers[i].id : p->representations[i].id) ||
          !eq(get(a->as.array.items[i], "fingerprint"), fp))
        return false;
    }
  }
  return true;
}
