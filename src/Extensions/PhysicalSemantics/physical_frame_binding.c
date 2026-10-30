// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_frame_binding.h"
#include "Extensions/PhysicalSemantics/physical_representation_binding.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
const PhysicalDeclaredContext *
physical_declared_context(const FisicsPhysicalState *s, uint32_t id) {
  return s && id && id <= s->declared_context_count
             ? &s->declared_contexts[id - 1]
             : NULL;
}
static bool between(const ASTAttribute *a, char from[256], char to[256]) {
  const char *s = a->payload + strlen("fisics::between_points");
#define SPACE()                                                                \
  while (isspace((unsigned char)*s))                                           \
  s++
  SPACE();
  if (*s++ != '(')
    return false;
  for (unsigned n = 0; n < 2; n++) {
    SPACE();
    if (*s++ != '"')
      return false;
    char *v = n ? to : from;
    size_t k = 0;
    while (*s && *s != '"') {
      if (k >= 255)
        return false;
      v[k++] = *s++;
    }
    v[k] = 0;
    if (*s++ != '"' || !fisics_semantic_metadata_is_canonical_name(v, NULL))
      return false;
    SPACE();
    if (*s++ != (n ? ')' : ','))
      return false;
  }
  SPACE();
  return !*s;
#undef SPACE
}
uint32_t physical_frame_bind(CompilerContext *c, const ParsedType *t,
                             const ASTNode *n, PhysicalFact *fact,
                             bool diagnose) {
  static const char *const names[] = {"fisics::frame",
                                      "fisics::component_axis",
                                      "fisics::applied_at",
                                      "fisics::between_points",
                                      "fisics::moment_reference",
                                      "fisics::point_id",
                                      "fisics::frame_transform"};
  PhysicalDeclaredContext v = {0};
  bool any = false;
  FisicsPhysicalState *s = physical_state(c);
  const FisicsPhysicalPolicy *p = &s->policy;
  for (unsigned family = 0; family < 7; family++) {
    unsigned count = 0;
    const ASTAttribute *a = NULL;
    for (size_t i = 0; i < t->attributeCount; i++) {
      const ASTAttribute *at = t->attributes[i];
      size_t len = strlen(names[family]);
      if (at && at->syntax == AST_ATTRIBUTE_SYNTAX_CXX && at->payload &&
          !strncmp(at->payload, names[family], len) &&
          (!at->payload[len] || at->payload[len] == '(' ||
           isspace((unsigned char)at->payload[len]))) {
        count++;
        a = at;
      }
    }
    if (!count)
      continue;
    any = true;
    if (count != 1 || p->schema_version < 6)
      goto invalid;
    if (family == 3) {
      char from[256], to[256];
      if (!between(a, from, to))
        goto invalid;
      v.displacement_from = physical_reference_point_find(p, from);
      v.displacement_to = physical_reference_point_find(p, to);
      if (!v.displacement_from || !v.displacement_to)
        goto invalid;
      continue;
    }
    char *id = physical_shape_attribute(t, names[family]);
    if (!id)
      goto invalid;
    uint32_t handle = 0;
    switch (family) {
    case 0:
      handle = v.frame = physical_frame_find(p, id);
      break;
    case 1:
      handle = v.axis = !strcmp(id, "axis.x")   ? 1
                        : !strcmp(id, "axis.y") ? 2
                        : !strcmp(id, "axis.z") ? 3
                                                : 0;
      break;
    case 2:
      handle = v.applied_at = physical_reference_point_find(p, id);
      break;
    case 4:
      handle = v.moment_reference = physical_reference_point_find(p, id);
      break;
    case 5:
      handle = v.point_subject = physical_reference_point_find(p, id);
      break;
    case 6:
      handle = v.transform = physical_frame_transform_find(p, id);
      break;
    }
    free(id);
    if (!handle)
      goto invalid;
  }
  if (!any)
    return 0;
  if (!physical_known(*fact) || !v.frame)
    goto invalid;
  const char *kind = p->kinds[fact->type.kind - 1].id;
  bool force = !strcmp(kind, "quantity.force"),
       displacement = !strcmp(kind, "quantity.displacement"),
       torque = !strcmp(kind, "quantity.torque"),
       position = !strcmp(kind, "quantity.position");
  bool cosmic_axial=physical_cosmic_vector_kind(p,fact->type.kind,true);
  bool cosmic=cosmic_axial || physical_cosmic_vector_kind(p,fact->type.kind,false);
  if (!cosmic && !force && !displacement && !torque && !position)
    goto invalid;
  const PhysicalFrame *f = &p->frames[v.frame - 1];
  if (fact->type.representation) {
    const FisicsPhysicalRepresentation *r =
        &p->representations[fact->type.representation - 1];
    if (v.axis || r->rank != 1 || r->extent != f->dimension ||
        r->axial != (torque || cosmic_axial))
      goto invalid;
  } else if (!v.axis || v.axis > f->dimension || fact->carrier)
    goto invalid;
  if ((v.applied_at && !force) || (v.moment_reference && !torque) ||
      (v.point_subject && !position) || (v.displacement_from && !displacement))
    goto invalid;
  if (position && (!f->affine_origin || fact->type.origin != f->affine_origin))
    goto invalid;
  uint32_t points[] = {v.applied_at, v.displacement_from, v.displacement_to,
                       v.moment_reference, v.point_subject};
  for (unsigned i = 0; i < 5; i++)
    if (points[i] && p->reference_points[points[i] - 1].context != f->context)
      goto invalid;
  if (v.transform && (!n || (n->type != AST_FUNCTION_DECLARATION &&
                             n->type != AST_FUNCTION_DEFINITION)))
    goto invalid;
  return physical_frame_intern(c,n,v,fact);
invalid:
  if (diagnose)
    physical_error(
        c, n, 4312,
        "invalid frame declaration: v6 identity, quantity role, frame/axis, "
        "reference context or declaration target mismatch");
  fact->state = PHYSICAL_ERROR;
  return 0;
}
bool physical_frame_function_valid(CompilerContext *c, ASTNode *fn) {
  const ParsedType *ret = fn->type == AST_FUNCTION_DEFINITION
                              ? &fn->functionDef.returnType
                              : &fn->functionDecl.returnType;
  char *id = physical_shape_attribute(ret, "fisics::frame_transform");
  if (!id)
    return false;
  FisicsPhysicalState *s = physical_state(c);
  uint32_t tid = physical_frame_transform_find(&s->policy, id);
  free(id);
  ASTNode **ps = fn->type == AST_FUNCTION_DEFINITION
                     ? fn->functionDef.parameters
                     : fn->functionDecl.parameters;
  size_t count = fn->type == AST_FUNCTION_DEFINITION
                     ? fn->functionDef.paramCount
                     : fn->functionDecl.paramCount;
  bool variadic = fn->type == AST_FUNCTION_DEFINITION
                      ? fn->functionDef.isVariadic
                      : fn->functionDecl.isVariadic;
  if (!tid || count != 1 || variadic || !ps[0] ||
      ps[0]->type != AST_VARIABLE_DECLARATION || ps[0]->varDecl.varCount != 1)
    goto invalid;
  const PhysicalFrameTransform *x = &s->policy.frame_transforms[tid - 1];
  bool point=x->operation==PHYSICAL_MAP_POINT;
  PhysicalDeclaredContext input = {0};
  for (unsigned i = 0; i < 2; i++) {
    PhysicalFact f = physical_type(c, i ? ret : astVarDeclTypeAt(ps[0], 0),
                                   i ? fn : ps[0], false, i);
    const PhysicalDeclaredContext *v =
        physical_declared_context(s, f.declared_context);
    if (!physical_known(f) || !v || v->frame != (i ? x->target : x->source) ||
        f.type.kind != (i ? x->output_kind : x->input_kind) ||
        f.type.representation !=
            (i ? x->output_representation : x->input_representation) ||
        f.type.origin != (point ? s->policy.frames[(i?x->target:x->source)-1].affine_origin : 0) || f.carrier || v->axis || (!i && v->transform))
      goto invalid;
    if (!i)
      input = *v;
    if (i && (v->transform != tid || v->applied_at != input.applied_at ||
              v->displacement_from != input.displacement_from ||
              v->displacement_to != input.displacement_to ||
              v->moment_reference != input.moment_reference ||
              v->point_subject != input.point_subject))
      goto invalid;
  }
  return true;
invalid:
  return false;
}
void physical_frame_validate_function(CompilerContext *c, ASTNode *fn) {
  const ParsedType *ret = fn->type == AST_FUNCTION_DEFINITION
                              ? &fn->functionDef.returnType
                              : &fn->functionDecl.returnType;
  char *id = physical_shape_attribute(ret, "fisics::frame_transform");
  if (!id) return;
  free(id);
  if (!physical_frame_function_valid(c, fn))
    physical_error(c, fn, 4312,
                 "frame_transform requires an exact unary by-value 3D double "
                 "vector or point signature with directed source/target frame "
                 "declarations and preserved physical references");
}
