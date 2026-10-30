// SPDX-License-Identifier: Apache-2.0
#include "Compiler/physical_contract_catalog.h"
#include "Compiler/semantic_project_json.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"
#include "Extensions/Units/units_parser.h"
#include "Extensions/Units/units_registry.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define PHYSICAL_INPUT_MAX_BYTES FISICS_PHYSICAL_INPUT_MAX_BYTES
#define diag physical_load_diag
#define exact_fields physical_exact_fields
static bool canonical_name(const char *text) {
  const char *detail = NULL;
  return text && fisics_semantic_metadata_is_canonical_name(text, &detail);
}
void fisics_physical_quantity_free(FisicsPhysicalQuantityKind *q) {
  free(q->id);
  free(q->dimension_system);
  free(q->dimension_text);
  free(q->catalog_path);
  memset(q, 0, sizeof(*q));
}

void fisics_physical_contract_free(FisicsPhysicalContract *c) {
  free(c->contract_id);
  free(c->subject_id);
  free(c->quantity_kind);
  free(c->expected_unit);
  free(c->value_form);
  free(c->tensor_shape);
  free(c->reference_kind);
  free(c->reference_subject);
  free(c->period_binary64);
  free(c->frame_id);
  free(c->catalog_path);
  free(c->conversion_model);
  free(c->conversion_scale_binary64);
  free(c->conversion_offset_binary64);
  memset(c, 0, sizeof(*c));
}

static bool copy_nullable_string(const FisicsJsonValue *value, char **out,
                                 FisicsPhysicalLoadContext *load) {
  if (!value)
    return false;
  if (value->type == FISICS_JSON_NULL) {
    *out = NULL;
    return true;
  }
  if (value->type != FISICS_JSON_STRING || !value->as.text[0])
    return false;
  *out = physical_dup_text(load, value->as.text);
  return *out != NULL;
}

bool fisics_physical_catalog_append(
    const char *path, FisicsPhysicalQuantityKind **quantities,
    size_t *quantity_count, FisicsPhysicalContract **contracts,
    size_t *contract_count, FisicsPhysicalLoadContext *load,
    FisicsPhysicalInputReceipt *receipt, FisicsSemanticProjectDiagnostic *d) {
  if (!quantities || !quantity_count || !contracts || !contract_count)
    return false;
  FisicsPhysicalInputReceipt input = {0};
  FisicsJsonValue *root = NULL;
  if (!fisics_physical_read_json(path, PHYSICAL_INPUT_MAX_BYTES, &root, &input,
                                 load, d))
    return false;
  static const char *const fields[] = {"schema", "schema_version", "catalog_id",
                                       "quantity_kinds", "contracts"};
  uint64_t version = 0;
  const FisicsJsonValue *schema = fisics_json_object_get(root, "schema");
  const FisicsJsonValue *catalog_id =
      fisics_json_object_get(root, "catalog_id");
  const FisicsJsonValue *qs = fisics_json_object_get(root, "quantity_kinds");
  const FisicsJsonValue *cs = fisics_json_object_get(root, "contracts");
  if (!exact_fields(root, fields, 5u) || !schema ||
      schema->type != FISICS_JSON_STRING ||
      strcmp(schema->as.text, "fisiCs.physical_contract_catalog") != 0 ||
      !fisics_json_integer_u64(fisics_json_object_get(root, "schema_version"),
                               &version) ||
      version != 1u || !catalog_id || catalog_id->type != FISICS_JSON_STRING ||
      !canonical_name(catalog_id->as.text) || !qs ||
      qs->type != FISICS_JSON_ARRAY || !cs || cs->type != FISICS_JSON_ARRAY) {
    diag(d, "invalid or oversized physical contract catalog: %s", path);
    goto fail;
  }
  if (qs->as.array.count >
          (load ? load->options.max_kinds : FISICS_PHYSICAL_MAX_KINDS) ||
      cs->as.array.count > (load ? load->options.max_contracts
                                 : FISICS_PHYSICAL_MAX_CONTRACTS) ||
      *quantity_count >
          (load ? load->options.max_kinds : FISICS_PHYSICAL_MAX_KINDS) -
              qs->as.array.count ||
      *contract_count >
          (load ? load->options.max_contracts : FISICS_PHYSICAL_MAX_CONTRACTS) -
              cs->as.array.count) {
    if (load)
      load->failure = PHYSICAL_LOAD_LIMIT;
    diag(d, "invalid or oversized physical contract catalog: %s", path);
    goto fail;
  }
  size_t qbase = *quantity_count, cbase = *contract_count;
  if (qs->as.array.count) {
    FisicsPhysicalQuantityKind *qgrown =
        (FisicsPhysicalQuantityKind *)physical_realloc(
            load, *quantities,
            (qbase + qs->as.array.count) * sizeof(**quantities));
    if (!qgrown) {
      diag(d, "out of memory reading quantity kinds");
      goto fail;
    }
    *quantities = qgrown;
    memset(*quantities + qbase, 0, qs->as.array.count * sizeof(**quantities));
  }
  *quantity_count = qbase + qs->as.array.count;
  for (size_t i = 0; i < qs->as.array.count; ++i) {
    FisicsPhysicalQuantityKind *q = &(*quantities)[qbase + i];
    const FisicsJsonValue *v = qs->as.array.items[i];
    static const char *const qf[] = {"id", "dimension_system",
                                     "dimension_vector", "dimension_text"};
    const FisicsJsonValue *id = fisics_json_object_get(v, "id");
    const FisicsJsonValue *sys = fisics_json_object_get(v, "dimension_system");
    const FisicsJsonValue *vec = fisics_json_object_get(v, "dimension_vector");
    const FisicsJsonValue *text = fisics_json_object_get(v, "dimension_text");
    if (!exact_fields(v, qf, 4u) || !id || id->type != FISICS_JSON_STRING ||
        !canonical_name(id->as.text) || !sys ||
        sys->type != FISICS_JSON_STRING ||
        strcmp(sys->as.text, "fisiCs.si_dim8.v1") != 0 || !vec ||
        vec->type != FISICS_JSON_ARRAY || vec->as.array.count != 8u || !text ||
        text->type != FISICS_JSON_STRING || !text->as.text[0]) {
      diag(d, "quantity_kinds[%zu] is invalid in %s", i, path);
      goto fail_added;
    }
    for (size_t j = 0; j < 8u; ++j) {
      int n = 0;
      if (!fisics_json_integer_int(vec->as.array.items[j], &n) || n < -128 ||
          n > 127) {
        diag(d, "quantity dimension exponent is invalid");
        goto fail_added;
      }
      q->dimension[j] = (int8_t)n;
    }
    q->id = physical_dup_text(load, id->as.text);
    q->dimension_system = physical_dup_text(load, sys->as.text);
    q->dimension_text = physical_dup_text(load, text->as.text);
    q->catalog_path = physical_dup_text(load, path);
    if (!q->id || !q->dimension_system || !q->dimension_text ||
        !q->catalog_path) {
      diag(d, "out of memory reading quantity kind");
      goto fail_added;
    }
    FisicsDim8 parsed_dim = {{0}};
    char *parse_error = NULL;
    if (!fisics_units_parse_dim_expr(q->dimension_text, &parsed_dim,
                                     &parse_error) ||
        memcmp(parsed_dim.e, q->dimension, 8u) != 0) {
      diag(
          d,
          "quantity kind %s dimension_text disagrees with dimension_vector%s%s",
          q->id, parse_error ? ": " : "", parse_error ? parse_error : "");
      free(parse_error);
      goto fail_added;
    }
    free(parse_error);
  }
  if (cs->as.array.count) {
    FisicsPhysicalContract *cgrown = (FisicsPhysicalContract *)physical_realloc(
        load, *contracts, (cbase + cs->as.array.count) * sizeof(**contracts));
    if (!cgrown) {
      diag(d, "out of memory reading physical contracts");
      goto fail_added;
    }
    *contracts = cgrown;
    memset(*contracts + cbase, 0, cs->as.array.count * sizeof(**contracts));
  }
  *contract_count = cbase + cs->as.array.count;
  for (size_t i = 0; i < cs->as.array.count; ++i) {
    FisicsPhysicalContract *c = &(*contracts)[cbase + i];
    const FisicsJsonValue *v = cs->as.array.items[i];
    static const char *const cf[] = {"contract_id",   "subject",
                                     "quantity_kind", "unit_policy",
                                     "value_form",    "reference_semantics",
                                     "frame_id"};
    const FisicsJsonValue *id = fisics_json_object_get(v, "contract_id");
    const FisicsJsonValue *subject = fisics_json_object_get(v, "subject");
    const FisicsJsonValue *quantity =
        fisics_json_object_get(v, "quantity_kind");
    if (!exact_fields(v, cf, 7u) || !id || id->type != FISICS_JSON_STRING ||
        !canonical_name(id->as.text) || !quantity ||
        quantity->type != FISICS_JSON_STRING ||
        !canonical_name(quantity->as.text)) {
      diag(d, "contracts[%zu] has invalid identity in %s", i, path);
      goto fail_added;
    }
    static const char *const sf[] = {"kind", "id"};
    const FisicsJsonValue *sk = fisics_json_object_get(subject, "kind");
    const FisicsJsonValue *si = fisics_json_object_get(subject, "id");
    if (!exact_fields(subject, sf, 2u) || !sk ||
        sk->type != FISICS_JSON_STRING || !si ||
        si->type != FISICS_JSON_STRING || !si->as.text[0]) {
      diag(d, "contract subject is invalid");
      goto fail_added;
    }
    if (strcmp(sk->as.text, "semantic_id") == 0) {
      if (!canonical_name(si->as.text)) {
        diag(d, "semantic contract subject is not canonical");
        goto fail_added;
      }
      c->subject_kind = FISICS_PHYSICAL_SUBJECT_SEMANTIC_ID;
    } else if (strcmp(sk->as.text, "source_identity") == 0)
      c->subject_kind = FISICS_PHYSICAL_SUBJECT_SOURCE_IDENTITY;
    else {
      diag(d, "unknown physical subject kind");
      goto fail_added;
    }
    const FisicsJsonValue *policy = fisics_json_object_get(v, "unit_policy");
    static const char *const pf[] = {"mode", "unit"};
    const FisicsJsonValue *mode = fisics_json_object_get(policy, "mode");
    const FisicsJsonValue *unit = fisics_json_object_get(policy, "unit");
    if (!exact_fields(policy, pf, 2u) || !mode ||
        mode->type != FISICS_JSON_STRING || !unit) {
      diag(d, "physical unit_policy is invalid");
      goto fail_added;
    }
    if (strcmp(mode->as.text, "dimension_only") == 0) {
      c->unit_policy = FISICS_PHYSICAL_UNIT_DIMENSION_ONLY;
      if (unit->type != FISICS_JSON_NULL) {
        diag(d, "dimension_only unit must be null");
        goto fail_added;
      }
    } else if (strcmp(mode->as.text, "exact_unit") == 0)
      c->unit_policy = FISICS_PHYSICAL_UNIT_EXACT;
    else if (strcmp(mode->as.text, "convertible_unit") == 0)
      c->unit_policy = FISICS_PHYSICAL_UNIT_CONVERTIBLE;
    else {
      diag(d, "unknown physical unit policy");
      goto fail_added;
    }
    if (c->unit_policy != FISICS_PHYSICAL_UNIT_DIMENSION_ONLY) {
      const FisicsUnitDef *def = NULL;
      if (unit->type != FISICS_JSON_STRING ||
          !fisics_unit_lookup(unit->as.text, &def) || !def) {
        diag(d, "physical contract expected unit is unknown");
        goto fail_added;
      }
      c->expected_unit = physical_dup_text(load, def->name);
      if (!c->expected_unit)
        goto fail_added;
    }
    const FisicsJsonValue *form = fisics_json_object_get(v, "value_form");
    static const char *const vf[] = {"kind", "component_count", "tensor_rank",
                                     "tensor_shape"};
    const FisicsJsonValue *fk = fisics_json_object_get(form, "kind");
    const FisicsJsonValue *components =
        fisics_json_object_get(form, "component_count");
    const FisicsJsonValue *rank = fisics_json_object_get(form, "tensor_rank");
    const FisicsJsonValue *shape = fisics_json_object_get(form, "tensor_shape");
    if (!exact_fields(form, vf, 4u) || !fk || fk->type != FISICS_JSON_STRING ||
        !components || !rank || !shape || shape->type != FISICS_JSON_ARRAY) {
      diag(d, "physical value_form is invalid");
      goto fail_added;
    }
    if (strcmp(fk->as.text, "scalar") && strcmp(fk->as.text, "vector") &&
        strcmp(fk->as.text, "tensor") && strcmp(fk->as.text, "opaque")) {
      diag(d, "unknown physical value form");
      goto fail_added;
    }
    c->value_form = physical_dup_text(load, fk->as.text);
    if (!c->value_form)
      goto fail_added;
    if (components->type == FISICS_JSON_NUMBER) {
      if (!fisics_json_integer_size(components, &c->component_count) ||
          c->component_count == 0 || c->component_count > 65536u)
        goto bad_form;
      c->has_component_count = true;
    } else if (components->type != FISICS_JSON_NULL)
      goto bad_form;
    if (rank->type == FISICS_JSON_NUMBER) {
      if (!fisics_json_integer_size(rank, &c->tensor_rank) ||
          c->tensor_rank == 0 || c->tensor_rank > 8u)
        goto bad_form;
      c->has_tensor_rank = true;
    } else if (rank->type != FISICS_JSON_NULL)
      goto bad_form;
    c->tensor_shape_count = shape->as.array.count;
    if (c->tensor_shape_count) {
      c->tensor_shape = (size_t *)physical_calloc(load, c->tensor_shape_count,
                                                  sizeof(size_t));
      if (!c->tensor_shape)
        goto fail_added;
      for (size_t j = 0; j < c->tensor_shape_count; ++j)
        if (!fisics_json_integer_size(shape->as.array.items[j],
                                      &c->tensor_shape[j]) ||
            c->tensor_shape[j] == 0 || c->tensor_shape[j] > 65536u)
          goto bad_form;
    }
    if ((!strcmp(c->value_form, "vector") &&
         (!c->has_component_count || c->has_tensor_rank ||
          c->tensor_shape_count)) ||
        (!strcmp(c->value_form, "tensor") &&
         (!c->has_tensor_rank || c->tensor_shape_count != c->tensor_rank ||
          c->has_component_count)) ||
        ((strcmp(c->value_form, "vector") && strcmp(c->value_form, "tensor")) &&
         (c->has_component_count || c->has_tensor_rank ||
          c->tensor_shape_count)))
      goto bad_form;
    const FisicsJsonValue *ref =
        fisics_json_object_get(v, "reference_semantics");
    static const char *const rf[] = {"kind", "reference_subject",
                                     "period_binary64"};
    const FisicsJsonValue *rk = fisics_json_object_get(ref, "kind");
    const FisicsJsonValue *rs =
        fisics_json_object_get(ref, "reference_subject");
    const FisicsJsonValue *period =
        fisics_json_object_get(ref, "period_binary64");
    if (!exact_fields(ref, rf, 3u) || !rk || rk->type != FISICS_JSON_STRING ||
        (strcmp(rk->as.text, "linear") && strcmp(rk->as.text, "absolute") &&
         strcmp(rk->as.text, "difference") && strcmp(rk->as.text, "relative") &&
         strcmp(rk->as.text, "cyclic")) ||
        !copy_nullable_string(rs, &c->reference_subject, load) ||
        !copy_nullable_string(period, &c->period_binary64, load))
      goto bad_ref;
    c->reference_kind = physical_dup_text(load, rk->as.text);
    if (!c->reference_kind)
      goto fail_added;
    if (!strcmp(c->reference_kind, "relative")) {
      if (!c->reference_subject || !canonical_name(c->reference_subject) ||
          c->period_binary64)
        goto bad_ref;
    } else if (!strcmp(c->reference_kind, "cyclic")) {
      char *end = NULL;
      double p = c->period_binary64 ? strtod(c->period_binary64, &end) : 0.0;
      if (!c->period_binary64 || !end || *end || !isfinite(p) || p <= 0.0 ||
          c->reference_subject)
        goto bad_ref;
    } else if (c->reference_subject || c->period_binary64)
      goto bad_ref;
    if (!copy_nullable_string(fisics_json_object_get(v, "frame_id"),
                              &c->frame_id, load) ||
        (c->frame_id && !canonical_name(c->frame_id))) {
      diag(d, "physical frame_id is invalid");
      goto fail_added;
    }
    c->contract_id = physical_dup_text(load, id->as.text);
    c->subject_id = physical_dup_text(load, si->as.text);
    c->quantity_kind = physical_dup_text(load, quantity->as.text);
    c->catalog_path = physical_dup_text(load, path);
    if (!c->contract_id || !c->subject_id || !c->quantity_kind ||
        !c->catalog_path)
      goto fail_added;
    continue;
  bad_form:
    diag(d, "physical value_form invariants are invalid");
    goto fail_added;
  bad_ref:
    diag(d, "physical reference_semantics invariants are invalid");
    goto fail_added;
  }
  fisics_json_free(root);
  if (receipt)
    *receipt = input;
  return true;
fail_added:
  for (size_t i = qbase; i < *quantity_count; ++i)
    fisics_physical_quantity_free(&(*quantities)[i]);
  *quantity_count = qbase;
  for (size_t i = cbase; i < *contract_count; ++i)
    fisics_physical_contract_free(&(*contracts)[i]);
  *contract_count = cbase;
fail:
  fisics_json_free(root);
  return false;
}

static int compare_quantities(const void *l, const void *r) {
  const FisicsPhysicalQuantityKind *a = l;
  const FisicsPhysicalQuantityKind *b = r;
  int c = strcmp(a->id, b->id);
  return c ? c : strcmp(a->catalog_path, b->catalog_path);
}
static int compare_contract_ids(const void *l, const void *r) {
  return strcmp(((const FisicsPhysicalContract *)l)->contract_id,
                ((const FisicsPhysicalContract *)r)->contract_id);
}
static int compare_contract_subjects(const void *l, const void *r) {
  const FisicsPhysicalContract *a = l;
  const FisicsPhysicalContract *b = r;
  if (a->subject_kind != b->subject_kind)
    return a->subject_kind < b->subject_kind ? -1 : 1;
  int c = strcmp(a->subject_id, b->subject_id);
  return c ? c : strcmp(a->contract_id, b->contract_id);
}

static bool same_quantity(const FisicsPhysicalQuantityKind *a,
                          const FisicsPhysicalQuantityKind *b) {
  return strcmp(a->dimension_system, b->dimension_system) == 0 &&
         memcmp(a->dimension, b->dimension, 8u) == 0 &&
         strcmp(a->dimension_text, b->dimension_text) == 0;
}

bool fisics_physical_catalog_canonicalize(FisicsPhysicalQuantityKind *kinds,
                                          size_t *count,
                                          FisicsSemanticProjectDiagnostic *d) {
  if (*count > 1)
    qsort(kinds, *count, sizeof(*kinds), compare_quantities);
  size_t write = 0;
  for (size_t i = 0; i < *count; ++i) {
    if (write && strcmp(kinds[write - 1u].id, kinds[i].id) == 0) {
      if (!same_quantity(&kinds[write - 1u], &kinds[i])) {
        diag(d, "conflicting quantity kind %s in %s and %s", kinds[i].id,
             kinds[write - 1u].catalog_path, kinds[i].catalog_path);
        return false;
      }
      fisics_physical_quantity_free(&kinds[i]);
      continue;
    }
    if (write != i) {
      kinds[write] = kinds[i];
      memset(&kinds[i], 0, sizeof(kinds[i]));
    }
    ++write;
  }
  *count = write;
  return true;
}

const FisicsPhysicalQuantityKind *
fisics_physical_catalog_find(const FisicsPhysicalQuantityKind *kinds,
                             size_t count, const char *id) {
  if (!id)
    return NULL;
  size_t lo = 0, hi = count;
  while (lo < hi) {
    size_t mid = lo + (hi - lo) / 2;
    int c = strcmp(kinds[mid].id, id);
    if (c < 0)
      lo = mid + 1;
    else
      hi = mid;
  }
  return lo < count && !strcmp(kinds[lo].id, id) ? &kinds[lo] : NULL;
}
bool fisics_physical_catalog_unique_contracts(
    FisicsPhysicalContract *cs, size_t count,
    FisicsSemanticProjectDiagnostic *d) {
  if (count > 1)
    qsort(cs, count, sizeof(*cs), compare_contract_ids);
  for (size_t i = 1; i < count; i++)
    if (!strcmp(cs[i - 1].contract_id, cs[i].contract_id)) {
      diag(d, "duplicate physical contract_id %s in %s and %s",
           cs[i].contract_id, cs[i - 1].catalog_path, cs[i].catalog_path);
      return false;
    }
  if (count > 1)
    qsort(cs, count, sizeof(*cs), compare_contract_subjects);
  for (size_t i = 1; i < count; i++)
    if (cs[i - 1].subject_kind == cs[i].subject_kind &&
        !strcmp(cs[i - 1].subject_id, cs[i].subject_id)) {
      diag(d, "duplicate physical contract subject %s in %s and %s",
           cs[i].subject_id, cs[i - 1].catalog_path, cs[i].catalog_path);
      return false;
    }
  return true;
}
bool fisics_physical_catalog_validate(const FisicsPhysicalQuantityKind *kinds,
                                      size_t nk, FisicsPhysicalContract *cs,
                                      size_t nc,
                                      FisicsSemanticProjectDiagnostic *d) {
  if (!fisics_physical_catalog_unique_contracts(cs, nc, d))
    return false;
  for (size_t i = 0; i < nc; i++) {
    const FisicsPhysicalQuantityKind *q =
        fisics_physical_catalog_find(kinds, nk, cs[i].quantity_kind);
    if (!q) {
      diag(d, "physical contract %s references unknown quantity kind %s",
           cs[i].contract_id, cs[i].quantity_kind);
      return false;
    }
    if (cs[i].unit_policy != FISICS_PHYSICAL_UNIT_DIMENSION_ONLY) {
      const FisicsUnitDef *unit = NULL;
      if (!fisics_unit_lookup(cs[i].expected_unit, &unit) || !unit ||
          memcmp(unit->dim.e, q->dimension, 8)) {
        diag(d,
             "physical contract %s expected unit disagrees with quantity "
             "dimension",
             cs[i].contract_id);
        return false;
      }
    }
  }
  return true;
}
