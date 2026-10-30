// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_energy_semantics.h"
#include "Extensions/PhysicalSemantics/physical_representation_binding.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
static bool energy(CompilerContext *c, PhysicalFact f) {
  const FisicsPhysicalPolicy *p = &physical_state(c)->policy;
  return p->schema_version == 7 && f.type.kind &&
         !strcmp(p->kinds[f.type.kind - 1].id, "quantity.energy");
}
static PhysicalFact store(CompilerContext *c, const ASTNode *n, PhysicalFact f,
                          uint32_t form) {
  PhysicalDeclaredContext v = physical_frame_context(c, f);
  v.energy_form = form;
  v.energy_transfer = 0;
  f.declared_context = physical_frame_intern(c, n, v, &f);
  return f;
}
static PhysicalFact refuse(CompilerContext *c, const ASTNode *n, PhysicalFact f,
                           const char *why) {
  if (f.state != PHYSICAL_ERROR)
    physical_error(c, n, 4317, why);
  f.state = PHYSICAL_ERROR;
  f.reason = why;
  f.loss = n;
  return f;
}
static bool scalar(const ParsedType *t) {
  return t && t->kind == TYPE_PRIMITIVE && !t->pointerDepth &&
         !parsedTypeIsDirectArray(t) && !t->isFunctionPointer &&
         !t->isComplex && !t->isImaginary && !t->isLong &&
         (t->primitiveType == TOKEN_DOUBLE || t->primitiveType == TOKEN_FLOAT);
}
void physical_energy_bind(CompilerContext *c, const ParsedType *t,
                          const ASTNode *n, PhysicalFact *f, bool diagnose) {
  static const char *const names[] = {"fisics::energy_form",
                                      "fisics::energy_transfer"};
  PhysicalDeclaredContext v = physical_frame_context(c, *f);
  bool any = false;
  for (unsigned k = 0; k < 2; k++) {
    unsigned count = 0;
    size_t len = strlen(names[k]);
    for (size_t i = 0; i < t->attributeCount; i++) {
      const ASTAttribute *a = t->attributes[i];
      if (a && a->payload && a->syntax == AST_ATTRIBUTE_SYNTAX_CXX &&
          !strncmp(a->payload, names[k], len) &&
          (a->payload[len] == '(' || isspace((unsigned char)a->payload[len]) ||
           !a->payload[len]))
        count++;
    }
    if (!count)
      continue;
    any = true;
    char *id = physical_shape_attribute(t, names[k]);
    uint32_t h =
        k ? physical_energy_transfer_find(&physical_state(c)->policy, id)
          : physical_energy_form_find(&physical_state(c)->policy, id);
    free(id);
    if (count != 1 || !h || !energy(c, *f) || !physical_known(*f) ||
        !scalar(t) || f->type.representation || f->carrier ||
        (k && (!n || (n->type != AST_FUNCTION_DECLARATION &&
                      n->type != AST_FUNCTION_DEFINITION))))
      goto invalid;
    if (k)
      v.energy_transfer = h;
    else
      v.energy_form = h;
  }
  if (!any)
    return;
  if (v.energy_transfer && !v.energy_form)
    goto invalid;
  f->declared_context = physical_frame_intern(c, n, v, f);
  return;
invalid:
  if (diagnose)
    physical_error(c, n, 4312,
                   "energy qualifiers require v7 known scalar float/double "
                   "energy; transfer requires a function result with a form");
  f->state = PHYSICAL_ERROR;
}
PhysicalFact physical_energy_boundary(CompilerContext *c, const ASTNode *n,
                                      PhysicalFact a, PhysicalFact b,
                                      PhysicalFact f, bool literal) {
  if (!energy(c, a) || f.state == PHYSICAL_ERROR)
    return f;
  uint32_t x = physical_frame_context(c, a).energy_form,
           y = physical_frame_context(c, b).energy_form;
  if (literal && f.state == PHYSICAL_LITERAL_TRUST)
    return f;
  if (x && physical_known(f) && x != y &&
      !(y && x == physical_energy_form_find(&physical_state(c)->policy,
                                            "energy.mixed")))
    return refuse(c, n, f,
                  "energy form boundary cannot narrow, restore or relabel a "
                  "contribution; use an explicit directed transfer");
  return f;
}
PhysicalFact physical_energy_binary(CompilerContext *c, const ASTNode *n,
                                    PhysicalFact a, PhysicalFact b,
                                    PhysicalFact f) {
  if (!energy(c, f) || !physical_known(f) || !energy(c, a) || !energy(c, b))
    return f;
  uint32_t x = physical_frame_context(c, a).energy_form,
           y = physical_frame_context(c, b).energy_form;
  uint32_t form = !x || !y ? 0
                  : x == y ? x
                           : physical_energy_form_find(
                                 &physical_state(c)->policy, "energy.mixed");
  f = store(c, n, f, form);
  f.reason =
      form ? "energy contributions aggregate; mixed result is not a conversion"
           : "energy form unknown on an operand";
  return f;
}
PhysicalFact physical_energy_scale(CompilerContext *c, const ASTNode *n,
                                   const char *op, PhysicalFact a,
                                   PhysicalFact b, PhysicalFact f) {
  if (physical_state(c)->policy.schema_version != 7 || !physical_known(f))
    return f;
  if (energy(c, f)) {
    const FisicsPhysicalPolicy *p = &physical_state(c)->policy;
    bool ar = a.type.kind &&
              !strcmp(p->kinds[a.type.kind - 1].id, "quantity.ratio"),
         br = b.type.kind &&
              !strcmp(p->kinds[b.type.kind - 1].id, "quantity.ratio");
    uint32_t form = energy(c, a) && br
                        ? physical_frame_context(c, a).energy_form
                    : !strcmp(op, "*") && ar && energy(c, b)
                        ? physical_frame_context(c, b).energy_form
                        : 0;
    return store(c, n, f, form);
  }
  /* A quotient or other derived quantity must not retain its input qualifier.
   */
  if (physical_frame_context(c, f).energy_form)
    return store(c, n, f, 0);
  return f;
}
PhysicalFact physical_energy_call_result(CompilerContext *c, ASTNode *n,
                                         PhysicalFact f) {
  if (!energy(c, f) || !physical_known(f))
    return f;
  uint32_t target = physical_frame_context(c, f).energy_form;
  if (!target)
    return store(c, n, f, 0);
  for (size_t i = 0; i < n->functionCall.argumentCount; i++) {
    PhysicalFact a =
        physical_get(c, n->functionCall.arguments[i], PHYSICAL_EXPR, 0);
    if (energy(c, a) && physical_frame_context(c, a).energy_form != target)
      return refuse(c, n, f,
                    "ordinary energy helper cannot relabel an input "
                    "contribution without an exact energy_transfer contract");
  }
  return store(c, n, f, target);
}
