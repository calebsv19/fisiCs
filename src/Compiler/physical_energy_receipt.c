// SPDX-License-Identifier: Apache-2.0
#include "Compiler/physical_energy_receipt.h"
#include <string.h>
static const FisicsJsonValue *g(const FisicsJsonValue *j, const char *k) {
  return fisics_json_object_get(j, k);
}
static const char *s(const FisicsJsonValue *j) {
  return j && j->type == FISICS_JSON_STRING ? j->as.text : NULL;
}
static bool eq(const FisicsJsonValue *j, const char *v) {
  const char *a = s(j);
  return a && v && !strcmp(a, v);
}
static bool null(const FisicsJsonValue *j) {
  return j && j->type == FISICS_JSON_NULL;
}
void physical_energy_fact_write(FILE *out, CompilerContext *c, PhysicalFact f) {
  uint32_t form = physical_frame_context(c, f).energy_form;
  fputs(",\"energy_form\":", out);
  physical_json_string(
      out, form ? physical_state(c)->policy.energy_forms[form - 1] : NULL);
}
bool physical_energy_fact_match(const FisicsJsonValue *f,
                                const FisicsPhysicalPolicy *p) {
  const FisicsJsonValue *j = g(f, "energy_form");
  const char *id = s(j);
  return id ? physical_energy_form_find(p, id) &&
                  eq(g(f, "kind"), "quantity.energy") &&
                  null(g(f, "representation")) && !g(f, "carrier")->as.boolean
            : null(j);
}
bool physical_energy_signature_match(const FisicsJsonValue *d,
                                     const FisicsPhysicalPolicy *p) {
  const FisicsJsonValue *j = g(d, "energy_transfer");
  const char *id = s(j);
  if (!id)
    return null(j);
  uint32_t h = physical_energy_transfer_find(p, id);
  const FisicsJsonValue *slots = g(d, "slots");
  if (!h || !eq(g(d, "entity"), "function") || !slots ||
      slots->type != FISICS_JSON_ARRAY || slots->as.array.count != 2 ||
      g(d, "variadic")->as.boolean || !g(d, "prototype")->as.boolean)
    return false;
  const PhysicalEnergyTransfer *t = &p->energy_transfers[h - 1];
  for (unsigned i = 0; i < 2; i++) {
    const FisicsJsonValue *slot = slots->as.array.items[i],
                          *f = g(slot, "fact");
    if (!eq(g(slot, "surface"), "scalar") ||
        !eq(g(slot, "c_scalar"), "double") ||
        !eq(g(f, "kind"), "quantity.energy") ||
        !eq(g(f, "energy_form"),
            p->energy_forms[(i ? t->source : t->target) - 1]))
      return false;
  }
  return true;
}
