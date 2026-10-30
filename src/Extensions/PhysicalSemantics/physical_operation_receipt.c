// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include <inttypes.h>
#include <string.h>
static const char *role(FisicsPhysicalReferenceRole r) {
  return r == FISICS_PHYSICAL_ABSOLUTE     ? "absolute"
         : r == FISICS_PHYSICAL_DIFFERENCE ? "difference"
                                           : "linear";
}
/* IDs are already validated canonical names; no JSON escaping is necessary. */
void physical_operations_write(FILE *out, const FisicsPhysicalPolicy *p) {
  fprintf(out, "{\"version\":%u,\"fingerprint\":\"%016" PRIx64 "\",\"rules\":[",
          p->schema_version >= 4 ? 3u : p->schema_version >= 3 ? 2u : 1u, p->operation_fingerprint);
  for (size_t i = 0; i < p->operation_count; i++) {
    const FisicsPhysicalOperationRule *r = &p->operations[i];
    const char *kind = p->kinds[r->kind - 1].id;
    bool fabs_rule = (physical_math_unary(r->operation) || r->operation == PHYSICAL_COUPLING_TR || r->operation == PHYSICAL_COUPLING_RT);
    if (i)
      fputc(',', out);
    fprintf(out,
            "{\"id\":\"%s\",\"operation\":\"%s\",\"operand_kinds\":[\"%s\"",
            r->id, physical_operation_name(r->operation), p->kinds[r->left - 1].id);
    if (!fabs_rule)
      fprintf(out, ",\"%s\"", p->kinds[r->right - 1].id);
    fprintf(out,
            "],\"result_kind\":\"%s\",\"reference_role\":\"%s\",\"origin_"
            "policy\":\"%s\",\"provenance\":\"inputs\"}",
            kind, role(p->models[r->kind - 1].role),
            physical_operation_origin(r->operation));
  }
  fputs("]}", out);
}
static const FisicsJsonValue *get(const FisicsJsonValue *v, const char *k) {
  return fisics_json_object_get(v, k);
}
static bool eq(const FisicsJsonValue *v, const char *s) {
  return v && v->type == FISICS_JSON_STRING && !strcmp(v->as.text, s);
}
bool physical_operations_match(const FisicsJsonValue *j,
                               const FisicsPhysicalPolicy *p) {
  const char *const fields[] = {"version", "fingerprint", "rules"};
  int version = 0;
  char hash[17];
  snprintf(hash, sizeof(hash), "%016" PRIx64, p->operation_fingerprint);
  const FisicsJsonValue *rules = get(j, "rules");
  if (!physical_exact_fields(j, fields, 3) ||
      !fisics_json_integer_int(get(j, "version"), &version) || version != (p->schema_version >= 4 ? 3 : p->schema_version >= 3 ? 2 : 1) ||
      !eq(get(j, "fingerprint"), hash) || !rules ||
      rules->type != FISICS_JSON_ARRAY ||
      rules->as.array.count != p->operation_count)
    return false;
  const char *const rf[] = {"id",          "operation",      "operand_kinds",
                            "result_kind", "reference_role", "origin_policy",
                            "provenance"};
  for (size_t i = 0; i < p->operation_count; i++) {
    const FisicsPhysicalOperationRule *r = &p->operations[i];
    const FisicsJsonValue *v = rules->as.array.items[i],
                          *args = get(v, "operand_kinds");
    bool one = (physical_math_unary(r->operation) || r->operation == PHYSICAL_COUPLING_TR || r->operation == PHYSICAL_COUPLING_RT);
    const char *kind = p->kinds[r->kind - 1].id;
    if (!physical_exact_fields(v, rf, 7) || !eq(get(v, "id"), r->id) ||
        !eq(get(v, "operation"), physical_operation_name(r->operation)) ||
        !eq(get(v, "result_kind"), kind) ||
        !eq(get(v, "reference_role"), role(p->models[r->kind - 1].role)) ||
        !eq(get(v, "origin_policy"), physical_operation_origin(r->operation)) ||
        !eq(get(v, "provenance"), "inputs") || !args ||
        args->type != FISICS_JSON_ARRAY ||
        args->as.array.count != (one ? 1u : 2u))
      return false;
    for (size_t a = 0; a < args->as.array.count; a++)
      if (!eq(args->as.array.items[a], p->kinds[(a ? r->right : r->left) - 1].id))
        return false;
  }
  return true;
}
