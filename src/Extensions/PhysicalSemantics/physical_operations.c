// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"
#include "Extensions/Units/units_model.h"
#include <stdlib.h>
#include <string.h>
static const char *string(const FisicsJsonValue *j) {
  return j && j->type == FISICS_JSON_STRING ? j->as.text : NULL;
}
static const FisicsJsonValue *get(const FisicsJsonValue *j, const char *name) {
  return fisics_json_object_get(j, name);
}
static int by_id(const void *a, const void *b) {
  return strcmp(((const FisicsPhysicalOperationRule *)a)->id,
                ((const FisicsPhysicalOperationRule *)b)->id);
}
static int by_key(const void *a, const void *b) {
  const FisicsPhysicalOperationRule *x = a, *y = b;
  if (x->operation != y->operation)
    return x->operation < y->operation ? -1 : 1;
  if (x->left != y->left) return x->left < y->left ? -1 : 1;
  if (x->right != y->right) return x->right < y->right ? -1 : 1;
  return x->kind < y->kind ? -1 : x->kind > y->kind;
}
const FisicsPhysicalOperationRule *
physical_operation_rule(const FisicsPhysicalPolicy *p,
                        FisicsUnitsMathOperation op,
                        FisicsPhysicalKindId kind) {
  FisicsPhysicalOperationRule key = {.operation = op, .kind = kind, .left = kind,
      .right = physical_math_unary(op) ? 0 : kind};
  size_t lo = 0, hi = p ? p->operation_count : 0;
  while (lo < hi) {
    size_t m = lo + (hi - lo) / 2;
    int c = by_key(&p->operations[m], &key);
    if (c < 0)
      lo = m + 1;
    else
      hi = m;
  }
  return p && lo < p->operation_count && !by_key(&p->operations[lo], &key)
             ? &p->operations[lo]
             : NULL;
}
bool physical_operations_load(const FisicsJsonValue *root,
                              FisicsPhysicalPolicy *p,
                              FisicsPhysicalLoadContext *load,
                              FisicsSemanticProjectDiagnostic *d) {
  const FisicsJsonValue *list = get(root, "operation_rules");
  if (!list || list->type != FISICS_JSON_ARRAY) {
    physical_load_diag(d, "operation_rules must be an array");
    return false;
  }
  if (list->as.array.count > FISICS_PHYSICAL_MAX_OPERATION_RULES) {
    load->failure = PHYSICAL_LOAD_LIMIT;
    physical_load_diag(d, "physical operation rule limit exceeded");
    return false;
  }
  p->operation_count = list->as.array.count;
  if (!p->operation_count) {
    p->operation_fingerprint = physical_operations_fingerprint(p);
    return true;
  }
  p->operations =
      physical_calloc(load, p->operation_count, sizeof(*p->operations));
  if (!p->operations)
    return false;
  const char *const fields[] = {
      "id",          "operation",      "operand_kinds",
      "result_kind", "reference_role", "origin_policy",
      "provenance"};
  for (size_t i = 0; i < p->operation_count; i++) {
    const FisicsJsonValue *j = list->as.array.items[i];
    const char *id = string(get(j, "id")), *op = string(get(j, "operation")),
               *result = string(get(j, "result_kind")),
               *role = string(get(j, "reference_role")),
               *origin = string(get(j, "origin_policy")),
               *provenance = string(get(j, "provenance"));
    const FisicsJsonValue *args = get(j, "operand_kinds");
    const char *detail = NULL;
    if (!physical_exact_fields(j, fields, 7) || !id ||
        !fisics_semantic_metadata_is_canonical_name(id, &detail) || !op ||
        !result || !role || !origin || !provenance ||
        strcmp(provenance, "inputs") || !args ||
        args->type != FISICS_JSON_ARRAY)
      goto invalid;
    FisicsUnitsMathOperation operation =
        !strcmp(op, "fabs")   ? FISICS_UNITS_MATH_FABS
        : !strcmp(op, "fmax") ? FISICS_UNITS_MATH_FMAX
        : p->schema_version == 7 && !strcmp(op, "fmin") ? FISICS_UNITS_MATH_FMIN
        : p->schema_version == 7 && !strcmp(op, "expm1") ? FISICS_UNITS_MATH_EXPM1
        : p->schema_version == 7 && !strcmp(op, "log") ? FISICS_UNITS_MATH_LOG
        : p->schema_version == 7 && !strcmp(op, "sqrt") ? FISICS_UNITS_MATH_SQRT
        : p->schema_version == 7 && !strcmp(op, "hypot") ? FISICS_UNITS_MATH_HYPOT
        : p->schema_version == 7 && !strcmp(op, "atan2") ? FISICS_UNITS_MATH_ATAN2
                              : p->schema_version >= 3 && !strcmp(op, "multiply") ? PHYSICAL_MULTIPLY
        : p->schema_version >= 3 && !strcmp(op, "divide") ? PHYSICAL_DIVIDE
        : p->schema_version == 7 && !strcmp(op, "frame_coupling_tr") ? PHYSICAL_COUPLING_TR
        : p->schema_version == 7 && !strcmp(op, "frame_coupling_rt") ? PHYSICAL_COUPLING_RT
        : p->schema_version == 7 && !strcmp(op, "translation_work") ? PHYSICAL_TRANSLATION_WORK
        : p->schema_version == 7 && !strcmp(op, "rotation_work") ? PHYSICAL_ROTATION_WORK
        : p->schema_version == 7 && !strcmp(op, "transverse_line_moment") ? PHYSICAL_LINE_MOMENT
                              : FISICS_UNITS_MATH_NONE;
    size_t arity = physical_math_unary(operation) || operation == PHYSICAL_COUPLING_TR || operation == PHYSICAL_COUPLING_RT ? 1 : 2;
    if (!operation || args->as.array.count != arity)
      goto invalid;
    FisicsPhysicalKindId k = fisics_physical_policy_find_kind(p, result);
    if (!k)
      goto invalid;
    FisicsPhysicalKindId operands[2] = {0};
    bool constructor = physical_constructor_operation(operation);
    bool transform = physical_cosmic_math_operation(operation);
    bool scalar = operation == PHYSICAL_MULTIPLY || operation == PHYSICAL_DIVIDE || constructor;
    for (size_t n = 0; n < arity; n++) {
      const char *name = string(args->as.array.items[n]);
      FisicsPhysicalKindId a =
          name ? fisics_physical_policy_find_kind(p, name) : 0;
      if (!a)
        goto invalid;
      FisicsDim8 input, output;
      memcpy(input.e, p->kinds[a - 1].dimension, 8);
      memcpy(output.e, p->kinds[k - 1].dimension, 8);
      operands[n] = a;
      if (!scalar && !transform && (!fisics_dim_equal(input, output) || a != k))
        goto invalid;
    }
    FisicsPhysicalReferenceRole expected =
        !strcmp(role, "linear")       ? FISICS_PHYSICAL_LINEAR
        : !strcmp(role, "difference") ? FISICS_PHYSICAL_DIFFERENCE
        : !strcmp(role, "absolute")   ? FISICS_PHYSICAL_ABSOLUTE
                                      : FISICS_PHYSICAL_UNPROFILED;
    if (!expected || p->models[k - 1].role != expected ||
        (operation == FISICS_UNITS_MATH_FABS &&
         (expected == FISICS_PHYSICAL_ABSOLUTE ||
          strcmp(origin, "preserve"))) ||
        ((operation == FISICS_UNITS_MATH_FMAX || operation == FISICS_UNITS_MATH_FMIN) && strcmp(origin, "same")) ||
        ((operation == FISICS_UNITS_MATH_EXPM1 || operation == FISICS_UNITS_MATH_LOG) &&
         (strcmp(p->kinds[k-1].id,"quantity.ratio") ||
          memcmp(p->kinds[k-1].dimension,(int8_t[8]){0},8) ||
          expected != FISICS_PHYSICAL_LINEAR || strcmp(origin,"none"))))
      goto invalid;
    if (transform && (strcmp(origin, "none") || !physical_cosmic_math_contract(p, operation, operands[0], operands[1], k))) goto invalid;
    if (scalar && (strcmp(origin, "none") ||
        !(constructor ? physical_structural_contract(p, operation, operands[0], operands[1], k) : physical_scalar_contract(p, operation, operands[0], operands[1], k))))
      goto invalid;
    p->operations[i] =
        (FisicsPhysicalOperationRule){.operation = operation, .kind = k,
          .left = operands[0], .right = operands[1]};
    p->operations[i].id = physical_dup_text(load, id);
    if (!p->operations[i].id)
      return false;
  }
  qsort(p->operations, p->operation_count, sizeof(*p->operations), by_id);
  for (size_t i = 1; i < p->operation_count; i++)
    if (!strcmp(p->operations[i - 1].id, p->operations[i].id))
      goto invalid;
  qsort(p->operations, p->operation_count, sizeof(*p->operations), by_key);
  for (size_t i = 1; i < p->operation_count; i++)
    if (p->operations[i-1].operation == p->operations[i].operation &&
        p->operations[i-1].left == p->operations[i].left &&
        p->operations[i-1].right == p->operations[i].right)
      goto invalid;
  p->operation_fingerprint = physical_operations_fingerprint(p);
  return true;
invalid:
  physical_load_diag(d,
                     "invalid, duplicate or ambiguous physical operation rule "
                     "(outside the bounded helper/scalar model)");
  return false;
}
static uint64_t number(uint64_t h, uint64_t n) {
  for (unsigned i = 0; i < 8; i++) {
    h = (h ^ (unsigned char)n) * UINT64_C(1099511628211);
    n >>= 8;
  }
  return h;
}
static uint64_t text(uint64_t h, const char *s) {
  h = number(h, strlen(s));
  for (; *s; s++)
    h = (h ^ (unsigned char)*s) * UINT64_C(1099511628211);
  return h;
}
uint64_t physical_operations_fingerprint(const FisicsPhysicalPolicy *p) {
  uint64_t h =
      text(UINT64_C(1469598103934665603), p->schema_version >= 4 ? "fisiCs.physical_mechanics.v1" : p->schema_version == 3 ? "fisiCs.physical_scalars.v1" : "fisiCs.physical_helpers.v1");
  h = number(h, p->operation_count);
  for (size_t i = 0; i < p->operation_count; i++) {
    const FisicsPhysicalOperationRule *r = &p->operations[i];
    h = text(h, r->id);
    h = text(h, physical_operation_name(r->operation));
    if (p->schema_version >= 3) {
      h = text(h, p->kinds[r->left - 1].id);
      h = text(h, r->right ? p->kinds[r->right - 1].id : "");
    }
    h = text(h, p->kinds[r->kind - 1].id);
    h = number(h, p->models[r->kind - 1].role);
    h = text(h, physical_operation_origin(r->operation));
    h = text(h, "inputs");
  }
  return h;
}
