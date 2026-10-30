// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include <string.h>

PhysicalFact physical_merge_trust(PhysicalFact a, PhysicalFact b) {
  /* Preserve the assumption's loss location even when it is on the right.
     Interface trust takes precedence over literal trust, as at C boundaries. */
  if (b.state == PHYSICAL_DECLARED_TRUST &&
      a.state != PHYSICAL_DECLARED_TRUST) {
    a.state = b.state;
    a.loss = b.loss;
  } else if (b.state == PHYSICAL_LITERAL_TRUST && a.state == PHYSICAL_KNOWN) {
    a.state = b.state;
    a.loss = b.loss;
  } else if (!a.loss && b.loss)
    a.loss = b.loss;
  a.storage_component |= b.storage_component;
  return a;
}

PhysicalFact physical_affine_binary(CompilerContext *c, const ASTNode *n,
                                    const char *op, PhysicalFact a,
                                    PhysicalFact b) {
  const FisicsPhysicalPolicy *p = &physical_state(c)->policy;
  const FisicsPhysicalKindModel *am = &p->models[a.type.kind - 1];
  const FisicsPhysicalKindModel *bm = &p->models[b.type.kind - 1];
  bool aa = am->role == FISICS_PHYSICAL_ABSOLUTE;
  bool ba = bm->role == FISICS_PHYSICAL_ABSOLUTE;
  bool subtract = !strcmp(op, "-");
  PhysicalFact f;
  if (aa && ba) {
    if (a.type.kind != b.type.kind)
      goto incompatible;
    if (a.type.origin != b.type.origin) {
      physical_mismatch(c, n, 4318, op, a, b);
      goto error;
    }
    if (!subtract)
      goto incompatible;
    f = physical_merge_trust(a, b);
    f.type.kind = am->paired_kind;
    f.type.origin = 0;
    f.reason = "same-origin point subtraction yields paired difference";
    return f;
  }
  if (aa && bm->role == FISICS_PHYSICAL_DIFFERENCE &&
      am->paired_kind == b.type.kind && bm->paired_kind == a.type.kind) {
    f = physical_merge_trust(a, b);
    f.reason = "point shifted by paired difference; origin preserved";
    return f;
  }
  if (ba && !subtract && am->role == FISICS_PHYSICAL_DIFFERENCE &&
      bm->paired_kind == a.type.kind && am->paired_kind == b.type.kind) {
    f = physical_merge_trust(b, a);
    f.reason = "paired difference plus point; origin preserved";
    return f;
  }
incompatible:
  /* Neither dimensional equality nor an unprofiled identity is an affine pair.
   */
  physical_mismatch(c, n, 4317, op, a, b);
error:
  f = a;
  f.state = PHYSICAL_ERROR;
  f.reason = "invalid affine physical operation";
  return f;
}
