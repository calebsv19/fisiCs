// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_frame_semantics.h"
#include "Extensions/PhysicalSemantics/physical_representations.h"
#include <string.h>
PhysicalDeclaredContext physical_frame_context(CompilerContext *c,
                                               PhysicalFact f) {
  const PhysicalDeclaredContext *v =
      physical_declared_context(physical_state(c), f.declared_context);
  return v ? *v : (PhysicalDeclaredContext){0};
}
bool physical_frame_geometric(CompilerContext *c, PhysicalFact f) {
  if (physical_state(c)->policy.schema_version < 6)
    return false;
  return physical_frame_context(c, f).frame ||
         (f.type.representation &&
          physical_state(c)
                  ->policy.representations[f.type.representation - 1]
                  .rank == 1);
}
static PhysicalFact store(CompilerContext *c, const ASTNode *n, PhysicalFact f,
                          PhysicalDeclaredContext v) {
  v.transform = 0;
  f.declared_context = physical_frame_intern(c, n, v, &f);
  return f;
}
static PhysicalFact refuse(CompilerContext *c, const ASTNode *n, PhysicalFact f,
                           bool contradiction, const char *reason) {
  if (f.state == PHYSICAL_ERROR)
    return f;
  f.state = contradiction ? PHYSICAL_ERROR : PHYSICAL_UNKNOWN;
  f.reason = reason;
  f.loss = n;
  physical_error(c, n, contradiction ? 4317 : 4319, reason);
  return f;
}
static bool pair(CompilerContext *c, const ASTNode *n, PhysicalFact a,
                 PhysicalFact b, PhysicalFact *f) {
  PhysicalDeclaredContext x = physical_frame_context(c, a),
                          y = physical_frame_context(c, b);
  if (!physical_known(a) || !physical_known(b) || !x.frame || !y.frame) {
    *f = refuse(c, n, *f, false,
                "required frame/axis provenance is unknown; annotations and "
                "trust permissions cannot restore it");
    return false;
  }
  if (x.frame != y.frame) {
    const FisicsPhysicalPolicy *p = &physical_state(c)->policy;
    compiler_report_diag(c, n ? n->location : (SourceRange){0}, DIAG_ERROR,
                         4317, "use an explicit qualified frame transformation",
                         "physical frame mismatch: %s versus %s",
                         p->frames[x.frame - 1].id, p->frames[y.frame - 1].id);
    f->state = PHYSICAL_ERROR;
    f->reason = "contradictory coordinate frames";
    f->loss = n;
    return false;
  }
  if (x.axis != y.axis) {
    *f = refuse(c, n, *f, true,
                "physical component axis mismatch within the same frame");
    return false;
  }
  return true;
}
PhysicalFact physical_frame_boundary(CompilerContext *c, const ASTNode *n,
                                     const char *op, PhysicalFact a,
                                     PhysicalFact b, PhysicalFact f,
                                     bool literal) {
  (void)op;
  if (physical_state(c)->policy.schema_version < 6 ||
      f.state == PHYSICAL_ERROR)
    return f;
  if (!physical_frame_geometric(c, a) && !physical_frame_geometric(c, b))
    return f;
  /* Only an entirely literal tree may adopt the destination's authored frame.
   * This is representation trust, never inferred source provenance. */
  if (literal && physical_known(a) && f.state == PHYSICAL_LITERAL_TRUST)
    return f;
  PhysicalDeclaredContext x = physical_frame_context(c, a),
                          y = physical_frame_context(c, b);
  if (!x.frame && y.frame)
    return refuse(c, n, f, false,
                  "framed value cannot pass through an unframed scalar or "
                  "carrier boundary (context erasure)");
  if (!pair(c, n, a, b, &f))
    return f;
  uint32_t expected[] = {x.applied_at, x.displacement_from, x.displacement_to,
                         x.moment_reference, x.point_subject};
  uint32_t actual[] = {y.applied_at, y.displacement_from, y.displacement_to,
                       y.moment_reference, y.point_subject};
  for (unsigned i = 0; i < 5; i++)
    if (expected[i] && expected[i] != actual[i])
      return refuse(c, n, f, actual[i] != 0,
                    actual[i] ? "physical reference point mismatch at boundary"
                              : "required physical reference is unknown; "
                                "destination metadata cannot supply it");
  if (physical_known(f)) {
    y.flags |= PHYSICAL_FRAME_CHECKED;
    f = store(c, n, f, y);
  }
  return f;
}
PhysicalFact physical_frame_binary(CompilerContext *c, const ASTNode *n,
                                   const char *op, PhysicalFact a,
                                   PhysicalFact b, PhysicalFact f) {
  if (physical_state(c)->policy.schema_version < 6 ||
      f.state == PHYSICAL_ERROR)
    return f;
  if (!physical_frame_geometric(c, a) && !physical_frame_geometric(c, b))
    return f;
  if (!pair(c, n, a, b, &f))
    return f;
  if (!physical_known(f) && f.state != PHYSICAL_CONTROL)
    return f;
  PhysicalFact point_result;
  if(physical_point_operation(c,n,op,a,b,&point_result)) return point_result;
  PhysicalDeclaredContext x = physical_frame_context(c, a),
                          y = physical_frame_context(c, b);
  x.flags |= PHYSICAL_FRAME_CHECKED;
  if (f.state == PHYSICAL_CONTROL) {
    x = (PhysicalDeclaredContext){.flags = PHYSICAL_FRAME_CHECKED |
                                           PHYSICAL_FRAME_INVARIANT};
    return store(c, n, f, x);
  }
  const FisicsPhysicalPolicy *p = &physical_state(c)->policy;
  const char *kind = p->kinds[f.type.kind - 1].id;
  if (!strcmp(kind, "quantity.position"))
    return refuse(c, n, f, false,
                  "framed position arithmetic is deferred to the bounded point "
                  "checkpoint");
  if (!strcmp(kind, "quantity.torque") &&
      x.moment_reference != y.moment_reference)
    return refuse(
        c, n, f, x.moment_reference && y.moment_reference,
        "torque addition requires compatible moment reference points");
  if (!strcmp(kind, "quantity.force") && x.applied_at != y.applied_at)
    x.applied_at = 0;
  if (!strcmp(kind, "quantity.displacement") &&
      (strcmp(op, "conditional") ||
       x.displacement_from != y.displacement_from ||
       x.displacement_to != y.displacement_to))
    x.displacement_from = x.displacement_to = 0;
  return store(c, n, f, x);
}
PhysicalFact physical_frame_scale(CompilerContext *c, const ASTNode *n,
                                  const char *op, PhysicalFact a,
                                  PhysicalFact b, PhysicalFact f) {
  if (physical_state(c)->policy.schema_version < 6 ||
      f.state == PHYSICAL_ERROR)
    return f;
  bool ga = physical_frame_geometric(c, a), gb = physical_frame_geometric(c, b);
  if (!ga && !gb)
    return f;
  PhysicalFact vector = ga ? a : b, ratio = ga ? b : a;
  const FisicsPhysicalPolicy *p = &physical_state(c)->policy;
  if (ga == gb || (!ga && !strcmp(op, "/")) || !physical_known(vector) ||
      !physical_known(ratio) || !ratio.type.kind ||
      strcmp(p->kinds[ratio.type.kind - 1].id, "quantity.ratio") ||
      ratio.carrier || ratio.type.representation ||
      physical_frame_context(c, ratio).frame)
    return refuse(c, n, f, false,
                  "framed scalar products require a known unframed ratio; a "
                  "component product is not a qualified dot/cross operation");
  PhysicalDeclaredContext v = physical_frame_context(c, vector);
  if (!v.frame)
    return refuse(c, n, f, false,
                  "ratio scaling cannot manufacture missing frame provenance");
  if (!physical_known(f))
    return f;
  v.displacement_from = v.displacement_to = 0;
  v.flags |= PHYSICAL_FRAME_CHECKED;
  return store(c, n, f, v);
}
PhysicalFact physical_frame_project(CompilerContext *c, const ASTNode *n,
                                    PhysicalFact f, unsigned axis) {
  if (physical_state(c)->policy.schema_version < 6)
    return f;
  PhysicalDeclaredContext v = physical_frame_context(c, f);
  if (!v.frame || !axis) {
    f.state = PHYSICAL_UNKNOWN;
    f.reason = "component frame or constant axis is unknown";
    f.loss = n;
    v.axis = 0;
  } else {
    v.axis = axis;
    v.flags |= PHYSICAL_FRAME_CHECKED;
  }
  return store(c, n, f, v);
}
PhysicalFact physical_frame_sign(CompilerContext *c, const ASTNode *n,
                                 PhysicalFact f) {
  if (physical_state(c)->policy.schema_version < 6 ||
      !physical_frame_geometric(c, f))
    return f;
  PhysicalDeclaredContext v = physical_frame_context(c, f);
  v.displacement_from = v.displacement_to = 0;
  return store(c, n, f, v);
}
PhysicalFact physical_frame_shape(CompilerContext *c, const ASTNode *n,
                                  unsigned operation, PhysicalFact a,
                                  PhysicalFact b, PhysicalFact f) {
  if (physical_state(c)->policy.schema_version < 6)
    return f;
  PhysicalFact point_result;
  if((operation==PHYSICAL_SHAPE_ADD || operation==PHYSICAL_SHAPE_SUB) &&
     physical_point_operation(c,n,operation==PHYSICAL_SHAPE_ADD?"+":"-",a,b,&point_result)) return point_result;
  if (operation == PHYSICAL_SHAPE_SCALE)
    return physical_frame_scale(c, n, "*", a, b, f);
  if (operation == PHYSICAL_SHAPE_ADD || operation == PHYSICAL_SHAPE_SUB)
    return physical_frame_binary(
        c, n, operation == PHYSICAL_SHAPE_ADD ? "+" : "-", a, b, f);
  if (!pair(c, n, a, b, &f))
    return f;
  if (!physical_known(f))
    return f;
  if (operation == PHYSICAL_SHAPE_DOT || operation == PHYSICAL_SHAPE_NORM) {
    PhysicalDeclaredContext v = {.flags = PHYSICAL_FRAME_CHECKED |
                                          PHYSICAL_FRAME_INVARIANT};
    return store(c, n, f, v);
  }
  PhysicalDeclaredContext x = physical_frame_context(c, a),
                          y = physical_frame_context(c, b);
  /* Cosmic crosses are free vectors, not force moments. Preserve the common
   * frame but do not invent an application/moment point or directed endpoint. */
  if(physical_state(c)->policy.schema_version==7 &&
     strcmp(physical_state(c)->policy.kinds[f.type.kind-1].id,"quantity.torque")) {
    x.axis=x.applied_at=x.displacement_from=x.displacement_to=x.moment_reference=x.point_subject=0;
    x.flags|=PHYSICAL_FRAME_CHECKED;return store(c,n,f,x);
  }
  if (!x.displacement_from || !x.displacement_to || !y.applied_at)
    return refuse(c, n, f, false,
                  "cross moment requires known directed displacement endpoints "
                  "and force application point");
  if (x.displacement_to != y.applied_at)
    return refuse(c, n, f, true,
                  "cross moment displacement endpoint differs from force "
                  "application point");
  x.moment_reference = x.displacement_from;
  x.displacement_from = x.displacement_to = x.applied_at = 0;
  x.flags |= PHYSICAL_FRAME_CHECKED;
  return store(c, n, f, x);
}

/* A declared result can carry implementation trust, but cannot be an implicit
 * reframe/reaxis/rebase adapter. Directed transforms have their own later gate.
 */
PhysicalFact physical_frame_call_result(CompilerContext *c, const ASTNode *n,
                                        PhysicalFact argument,
                                        PhysicalFact result) {
  if (physical_state(c)->policy.schema_version < 6 ||
      !physical_known(result) || !physical_frame_geometric(c, result))
    return result;
  PhysicalDeclaredContext out = physical_frame_context(c, result),
                          in = physical_frame_context(c, argument);
  if (!out.frame)
    return refuse(c, n, result, false,
                  "function result requires a known declared frame");
  if (!physical_frame_geometric(c, argument))
    return result;
  if (!physical_known(argument) || !in.frame)
    return refuse(
        c, n, result, false,
        "ordinary function cannot restore unknown input frame provenance");
  if (out.frame != in.frame || (out.axis && in.axis && out.axis != in.axis))
    return refuse(c, n, result, true,
                  "ordinary function cannot relabel coordinate frame or "
                  "component axis; a qualified operation contract is required");
  if (argument.type.kind == result.type.kind) {
    uint32_t required[] = {out.applied_at, out.displacement_from,
                           out.displacement_to, out.moment_reference,
                           out.point_subject};
    uint32_t supplied[] = {in.applied_at, in.displacement_from,
                           in.displacement_to, in.moment_reference,
                           in.point_subject};
    for (unsigned i = 0; i < 5; i++)
      if (required[i] && required[i] != supplied[i])
        return refuse(c, n, result, supplied[i] != 0,
                      "ordinary function cannot invent or rebase a physical "
                      "point reference");
  }
  out.flags |= PHYSICAL_FRAME_CHECKED;
  return store(c, n, result, out);
}
