// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_energy.h"
#include "Extensions/PhysicalSemantics/physical_frames_internal.h"
#include <inttypes.h>
#include <stdlib.h>
uint32_t physical_energy_form_find(const FisicsPhysicalPolicy *p,
                                   const char *id) {
  if (p && id)
    for (size_t i = 0; i < p->energy_form_count; i++)
      if (!strcmp(p->energy_forms[i], id))
        return (uint32_t)i + 1;
  return 0;
}
uint32_t physical_energy_transfer_find(const FisicsPhysicalPolicy *p,
                                       const char *id) {
  if (p && id)
    for (size_t i = 0; i < p->energy_transfer_count; i++)
      if (!strcmp(p->energy_transfers[i].id, id))
        return (uint32_t)i + 1;
  return 0;
}
void physical_energy_free(FisicsPhysicalPolicy *p) {
  for (size_t i = 0; p->energy_forms && i < p->energy_form_count; i++)
    free(p->energy_forms[i]);
  free(p->energy_forms);
  for (size_t i = 0; p->energy_transfers && i < p->energy_transfer_count; i++) {
    free(p->energy_transfers[i].id);
    free(p->energy_transfers[i].process);
  }
  free(p->energy_transfers);
}
static int form_order(const void *a, const void *b) {
  return strcmp(*(char *const *)a, *(char *const *)b);
}
static int transfer_order(const void *a, const void *b) {
  return strcmp(((const PhysicalEnergyTransfer *)a)->id,
                ((const PhysicalEnergyTransfer *)b)->id);
}
bool physical_energy_load(const FisicsJsonValue *root, FisicsPhysicalPolicy *p,
                          FisicsPhysicalLoadContext *l,
                          FisicsSemanticProjectDiagnostic *d) {
  const FisicsJsonValue *forms = fg(root, "energy_forms"),
                        *ts = fg(root, "energy_transfers");
  if (!forms || forms->type != FISICS_JSON_ARRAY || !forms->as.array.count ||
      !ts || ts->type != FISICS_JSON_ARRAY)
    goto invalid;
  if (forms->as.array.count > PHYSICAL_MAX_ENERGY_FORMS ||
      ts->as.array.count > PHYSICAL_MAX_ENERGY_TRANSFERS) {
    l->failure = PHYSICAL_LOAD_LIMIT;
    goto invalid;
  }
  uint32_t k = fisics_physical_policy_find_kind(p, "quantity.energy");
  const int8_t dim[8] = {2, 1, -2, 0, 0, 0, 0, 0};
  if (!k || p->models[k - 1].role != FISICS_PHYSICAL_LINEAR ||
      memcmp(p->kinds[k - 1].dimension, dim, 8))
    goto invalid;
  p->energy_form_count = forms->as.array.count;
  p->energy_forms =
      physical_calloc(l, p->energy_form_count, sizeof(*p->energy_forms));
  if (!p->energy_forms)
    return false;
  for (size_t i = 0; i < p->energy_form_count; i++) {
    const char *id = fs(forms->as.array.items[i]);
    if (!fn(id))
      goto invalid;
    if (!(p->energy_forms[i] = physical_dup_text(l, id)))
      return false;
  }
  qsort(p->energy_forms, p->energy_form_count, sizeof(*p->energy_forms),
        form_order);
  for (size_t i = 1; i < p->energy_form_count; i++)
    if (!strcmp(p->energy_forms[i - 1], p->energy_forms[i]))
      goto invalid;
  uint32_t mixed = physical_energy_form_find(p, "energy.mixed");
  if (!mixed)
    goto invalid;
  p->energy_transfer_count = ts->as.array.count;
  if (p->energy_transfer_count &&
      !(p->energy_transfers = physical_calloc(l, p->energy_transfer_count,
                                              sizeof(*p->energy_transfers))))
    return false;
  const char *const fields[] = {"id", "source_form", "target_form", "process"};
  for (size_t i = 0; i < p->energy_transfer_count; i++) {
    const FisicsJsonValue *v = ts->as.array.items[i];
    PhysicalEnergyTransfer *t = &p->energy_transfers[i];
    const char *id = fs(fg(v, "id")), *process = fs(fg(v, "process"));
    if (!physical_exact_fields(v, fields, 4) || !fn(id) || !fn(process))
      goto invalid;
    t->source = physical_energy_form_find(p, fs(fg(v, "source_form")));
    t->target = physical_energy_form_find(p, fs(fg(v, "target_form")));
    if (!t->source || !t->target || t->source == t->target ||
        t->source == mixed || t->target == mixed)
      goto invalid;
    if (!(t->id = physical_dup_text(l, id)) ||
        !(t->process = physical_dup_text(l, process)))
      return false;
  }
  if (p->energy_transfer_count > 1)
    qsort(p->energy_transfers, p->energy_transfer_count,
          sizeof(*p->energy_transfers), transfer_order);
  for (size_t i = 1; i < p->energy_transfer_count; i++)
    if (!strcmp(p->energy_transfers[i - 1].id, p->energy_transfers[i].id))
      goto invalid;
  uint64_t h = fstart("fisiCs.energy_model.v1");
  h = fnum(h, p->energy_form_count);
  for (size_t i = 0; i < p->energy_form_count; i++)
    h = ftext(h, p->energy_forms[i]);
  h = fnum(h, p->energy_transfer_count);
  for (size_t i = 0; i < p->energy_transfer_count; i++) {
    PhysicalEnergyTransfer *t = &p->energy_transfers[i];
    h = ftext(h, t->id);
    h = ftext(h, p->energy_forms[t->source - 1]);
    h = ftext(h, p->energy_forms[t->target - 1]);
    h = ftext(h, t->process);
  }
  p->energy_fingerprint = h;
  return true;
invalid:
  physical_load_diag(
      d, "invalid energy model: exact forms/transfers, linear SI energy, "
         "energy.mixed and distinct concrete directed endpoints required");
  return false;
}
void physical_energy_write(FILE *out, const FisicsPhysicalPolicy *p) {
  fprintf(out, "{\"version\":1,\"fingerprint\":\"%016" PRIx64 "\",\"forms\":[",
          p->energy_fingerprint);
  for (size_t i = 0; i < p->energy_form_count; i++)
    fprintf(out, "%s\"%s\"", i ? "," : "", p->energy_forms[i]);
  fputs("],\"transfers\":[", out);
  for (size_t i = 0; i < p->energy_transfer_count; i++) {
    const PhysicalEnergyTransfer *t = &p->energy_transfers[i];
    fprintf(out,
            "%s{\"id\":\"%s\",\"source_form\":\"%s\",\"target_form\":\"%s\","
            "\"process\":\"%s\",\"implementation\":\"declared\","
            "\"conservation\":\"unassessed\"}",
            i ? "," : "", t->id, p->energy_forms[t->source - 1],
            p->energy_forms[t->target - 1], t->process);
  }
  fputs("]}", out);
}
bool physical_energy_match(const FisicsJsonValue *j,
                           const FisicsPhysicalPolicy *p) {
  const char *const fields[] = {"version", "fingerprint", "forms", "transfers"};
  uint64_t v;
  char hash[17];
  snprintf(hash, sizeof(hash), "%016" PRIx64, p->energy_fingerprint);
  const FisicsJsonValue *forms = fg(j, "forms"), *ts = fg(j, "transfers");
  if (!physical_exact_fields(j, fields, 4) ||
      !fisics_json_integer_u64(fg(j, "version"), &v) || v != 1 ||
      !fe(fg(j, "fingerprint"), hash) || !forms ||
      forms->type != FISICS_JSON_ARRAY ||
      forms->as.array.count != p->energy_form_count || !ts ||
      ts->type != FISICS_JSON_ARRAY ||
      ts->as.array.count != p->energy_transfer_count)
    return false;
  for (size_t i = 0; i < p->energy_form_count; i++)
    if (!fe(forms->as.array.items[i], p->energy_forms[i]))
      return false;
  const char *const tf[] = {"id",      "source_form",    "target_form",
                            "process", "implementation", "conservation"};
  for (size_t i = 0; i < p->energy_transfer_count; i++) {
    const FisicsJsonValue *a = ts->as.array.items[i];
    const PhysicalEnergyTransfer *t = &p->energy_transfers[i];
    if (!physical_exact_fields(a, tf, 6) || !fe(fg(a, "id"), t->id) ||
        !fe(fg(a, "source_form"), p->energy_forms[t->source - 1]) ||
        !fe(fg(a, "target_form"), p->energy_forms[t->target - 1]) ||
        !fe(fg(a, "process"), t->process) ||
        !fe(fg(a, "implementation"), "declared") ||
        !fe(fg(a, "conservation"), "unassessed"))
      return false;
  }
  return true;
}
