// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_frame_semantics.h"
#include <string.h>
/* Only a declared position/paired difference participates. Coordinates never
 * acquire their origin, frame, or physical point subject from a destination. */
bool physical_point_operation(CompilerContext *c, const ASTNode *n,
                              const char *op, PhysicalFact a, PhysicalFact b,
                              PhysicalFact *out) {
  FisicsPhysicalPolicy *p = &physical_state(c)->policy;
  if (p->schema_version < 6 || !a.type.kind || !b.type.kind)
    return false;
  bool pa = !strcmp(p->kinds[a.type.kind - 1].id, "quantity.position");
  bool pb = !strcmp(p->kinds[b.type.kind - 1].id, "quantity.position");
  if (!pa && !pb)
    return false;
  if (strcmp(op, "+") && strcmp(op, "-"))
    return false;
  PhysicalDeclaredContext x = physical_frame_context(c, a),
                          y = physical_frame_context(c, b);
  PhysicalFact f = physical_unknown(
      n, "point operation requires known compatible frames and origins");
  if (!physical_known(a) || !physical_known(b) || !x.frame || !y.frame)
    goto unknown;
  if (x.frame != y.frame || x.axis != y.axis ||
      a.type.representation != b.type.representation)
    goto bad;
  f = physical_affine_binary(c, n, op, a, b);
  if (!physical_known(f)) {
    *out = f;
    return true;
  }
  PhysicalDeclaredContext v = pa ? x : y;
  v.transform = 0;
  v.flags = PHYSICAL_FRAME_CHECKED;
  v.point_subject = 0;
  v.displacement_from = v.displacement_to = 0;
  if (pa && pb) {
    v.displacement_from = y.point_subject;
    v.displacement_to = x.point_subject;
    if (!v.displacement_from || !v.displacement_to)
      v.displacement_from = v.displacement_to = 0;
  }
  f.declared_context = physical_frame_intern(c, n, v, &f);
  f.reason = "same-frame affine point operation checked; physical reference "
             "identities are authored premises";
  *out = f;
  return true;
unknown:
  physical_error(c, n, 4319, f.reason);
  *out = f;
  return true;
bad:
  physical_error(c, n, 4317,
                 "point operation frame, axis or representation mismatch");
  f.state = PHYSICAL_ERROR;
  *out = f;
  return true;
}
