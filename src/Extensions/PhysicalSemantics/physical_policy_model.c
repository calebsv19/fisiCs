// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_policy_internal.h"
#include "Extensions/PhysicalSemantics/physical_representations.h"
#include "Extensions/Units/units_registry.h"
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include "Extensions/PhysicalSemantics/physical_frames.h"
#include "Extensions/PhysicalSemantics/physical_energy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void fisics_physical_policy_free(FisicsPhysicalPolicy *p) {
  if (!p)
    return;
  physical_energy_free(p);
  physical_frames_free(p);
  physical_representations_free(p);
  free(p->policy_id);
  free(p->policy_path);
  for (size_t i = 0; p->catalogs && i < p->catalog_count; i++)
    free(p->catalogs[i].path);
  free(p->catalogs);
  for (size_t i = 0; i < p->kind_count; i++)
    fisics_physical_quantity_free(&p->kinds[i]);
  free(p->kinds);
  for (size_t i = 0; i < p->contract_count; i++)
    fisics_physical_contract_free(&p->contracts[i]);
  free(p->contracts);
  for (size_t i = 0; p->origins && i < p->origin_count; i++)
    free(p->origins[i].id);
  free(p->origins);
  free(p->models);
  for(size_t i=0;p->operations && i<p->operation_count;i++)free(p->operations[i].id);
  free(p->operations);
  memset(p, 0, sizeof(*p));
}
FisicsPhysicalKindId
fisics_physical_policy_find_kind(const FisicsPhysicalPolicy *p,
                                 const char *id) {
  if (!p || !id)
    return 0;
  const FisicsPhysicalQuantityKind *k =
      fisics_physical_catalog_find(p->kinds, p->kind_count, id);
  return k ? (FisicsPhysicalKindId)(k - p->kinds + 1) : 0;
}
FisicsPhysicalOriginId
fisics_physical_policy_find_origin(const FisicsPhysicalPolicy *p,
                                   const char *id) {
  if (!p || !id)
    return 0;
  size_t lo = 0, hi = p->origin_count;
  while (lo < hi) {
    size_t m = lo + (hi - lo) / 2;
    int c = strcmp(p->origins[m].id, id);
    if (c < 0)
      lo = m + 1;
    else
      hi = m;
  }
  return lo < p->origin_count && !strcmp(p->origins[lo].id, id)
             ? (uint32_t)(lo + 1)
             : 0;
}
bool fisics_physical_policy_make_type(const FisicsPhysicalPolicy *p, uint32_t k,
                                      uint32_t o, FisicsPhysicalType *out,
                                      FisicsPhysicalPolicyDiagnostic *d) {
  if (d)
    memset(d, 0, sizeof(*d));
  const char *reason = NULL;
  if (!p || !out || !k || k > p->kind_count || o > p->origin_count)
    reason = "unknown physical kind/origin handle";
  else if (p->models[k - 1].role == FISICS_PHYSICAL_ABSOLUTE) {
    if (!o || p->origins[o - 1].absolute_kind != k)
      reason = "absolute physical type requires a matching explicit origin";
  } else if (o)
    reason = "linear/difference/unprofiled physical type cannot carry a point "
             "origin";
  if (reason) {
    if (d) {
      d->code = FISICS_DIAG_CODE_PHYSICAL_POLICY_REFERENCE;
      snprintf(d->message, sizeof(d->message), "%s", reason);
    }
    return false;
  }
  *out = (FisicsPhysicalType){k, o, 0};
  return true;
}
static uint64_t byte(uint64_t h, unsigned x) {
  return (h ^ (unsigned char)x) * UINT64_C(1099511628211);
}
static uint64_t number(uint64_t h, uint64_t x) {
  for (unsigned i = 0; i < 8; i++) {
    h = byte(h, (unsigned)x);
    x >>= 8;
  }
  return h;
}
static uint64_t text(uint64_t h, const char *s) {
  size_t n = strlen(s);
  h = number(h, n);
  for (size_t i = 0; i < n; i++)
    h = byte(h, (unsigned char)s[i]);
  return h;
}
uint64_t physical_policy_fingerprint(FisicsPhysicalPolicy *p) {
  uint64_t h = text(UINT64_C(1469598103934665603),
                    "fisiCs.physical_semantics_policy.v1");
  p->registry_fingerprint = fisics_unit_registry_fingerprint();
  h = text(h, p->policy_id);
  h = number(h, p->registry_fingerprint);
  h = number(h, p->kind_count);
  for (size_t i = 0; i < p->kind_count; i++) {
    FisicsPhysicalQuantityKind *k = &p->kinds[i];
    uint64_t q =
        text(UINT64_C(1469598103934665603), "fisiCs.physical_quantity.v1");
    q = text(q, k->id);
    q = text(q, k->dimension_system);
    for (unsigned j = 0; j < 8; j++)
      q = byte(q, (unsigned char)k->dimension[j]);
    q = text(q, k->dimension_text);
    p->models[i].definition_fingerprint = q;
    h = number(h, q);
    h = number(h, p->models[i].role);
    uint32_t pair = p->models[i].paired_kind;
    h = text(h, pair ? p->kinds[pair - 1].id : "");
  }
  h = number(h, p->origin_count);
  for (size_t i = 0; p->origins && i < p->origin_count; i++) {
    h = text(h, p->origins[i].id);
    h = text(h, p->kinds[p->origins[i].absolute_kind - 1].id);
  }
  if(p->schema_version>=2){
    h=text(h,p->schema_version == 7 ? "fisiCs.physical_semantics_policy.v7" : p->schema_version == 6 ? "fisiCs.physical_semantics_policy.v6" : p->schema_version == 5 ? "fisiCs.physical_semantics_policy.v5" : p->schema_version == 4 ? "fisiCs.physical_semantics_policy.v4" : p->schema_version == 3 ? "fisiCs.physical_semantics_policy.v3" : "fisiCs.physical_semantics_policy.v2");
    h=number(h,p->operation_fingerprint);
    if(p->schema_version>=5)h=number(h,p->representation_fingerprint);
    if(p->schema_version>=6)h=number(h,p->frame_fingerprint);
    if(p->schema_version==7)h=number(h,p->energy_fingerprint);
  }
  return h;
}
