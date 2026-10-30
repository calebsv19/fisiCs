// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_energy_semantics.h"
#include "Extensions/PhysicalSemantics/physical_representation_binding.h"
#include "Extensions/extension_units_expr_bindings.h"
#include <stdlib.h>
static uint32_t signature(CompilerContext *c, ASTNode *fn) {
  bool def = fn->type == AST_FUNCTION_DEFINITION;
  const ParsedType *ret =
      def ? &fn->functionDef.returnType : &fn->functionDecl.returnType;
  char *id = physical_shape_attribute(ret, "fisics::energy_transfer");
  uint32_t h = physical_energy_transfer_find(&physical_state(c)->policy, id);
  free(id);
  if (!h)
    return 0;
  ASTNode **ps = def ? fn->functionDef.parameters : fn->functionDecl.parameters;
  size_t count = def ? fn->functionDef.paramCount : fn->functionDecl.paramCount;
  if (count != 1 ||
      (def ? fn->functionDef.isVariadic : fn->functionDecl.isVariadic) ||
      !ps[0] || ps[0]->type != AST_VARIABLE_DECLARATION ||
      ps[0]->varDecl.varCount != 1)
    return 0;
  const PhysicalEnergyTransfer *t =
      &physical_state(c)->policy.energy_transfers[h - 1];
  for (unsigned i = 0; i < 2; i++) {
    const ParsedType *type = i ? ret : astVarDeclTypeAt(ps[0], 0);
    PhysicalFact f = physical_type(c, type, i ? fn : ps[0], false, i);
    PhysicalDeclaredContext v = physical_frame_context(c, f);
    if (!physical_known(f) || f.carrier || f.type.representation ||
        type->kind != TYPE_PRIMITIVE || type->primitiveType != TOKEN_DOUBLE ||
        type->isLong || type->pointerDepth ||
        v.energy_form != (i ? t->target : t->source) ||
        v.energy_transfer != (i ? h : 0))
      return 0;
  }
  return h;
}
void physical_energy_validate_function(CompilerContext *c, ASTNode *fn) {
  const ParsedType *ret = fn->type == AST_FUNCTION_DEFINITION
                              ? &fn->functionDef.returnType
                              : &fn->functionDecl.returnType;
  char *id = physical_shape_attribute(ret, "fisics::energy_transfer");
  if (!id)
    return;
  free(id);
  if (!signature(c, fn))
    physical_error(c, fn, 4312,
                   "energy_transfer requires an exact directed unary double "
                   "by-value source/target energy-form signature");
}
bool physical_energy_transfer_call(CompilerContext *c, ASTNode *n,
                                   PhysicalFact *out) {
  if (physical_state(c)->policy.schema_version != 7 ||
      physical_raw_pointer(c, n->functionCall.callee))
    return false;
  ASTNode *fn =
      fisics_extension_lookup_units_function_binding(c, n->functionCall.callee);
  if (!fn || (fn->type != AST_FUNCTION_DECLARATION &&
              fn->type != AST_FUNCTION_DEFINITION))
    return false;
  const ParsedType *ret = fn->type == AST_FUNCTION_DEFINITION
                              ? &fn->functionDef.returnType
                              : &fn->functionDecl.returnType;
  char *id = physical_shape_attribute(ret, "fisics::energy_transfer");
  if (!id)
    return false;
  free(id);
  uint32_t h = signature(c, fn);
  if (!h || n->functionCall.argumentCount != 1) {
    *out = physical_unknown(n, "invalid energy transfer signature");
    out->state = PHYSICAL_ERROR;
    return true;
  }
  ASTNode **ps = fn->type == AST_FUNCTION_DEFINITION
                     ? fn->functionDef.parameters
                     : fn->functionDecl.parameters;
  PhysicalFact expected =
      physical_type(c, astVarDeclTypeAt(ps[0], 0), ps[0], false, false);
  PhysicalFact actual =
      physical_get(c, n->functionCall.arguments[0], PHYSICAL_EXPR, 0);
  PhysicalFact checked = physical_boundary(c, n->functionCall.arguments[0],
                                           "argument", expected, actual, false);
  if (!physical_known(checked)) {
    *out = checked;
    return true;
  }
  *out = physical_type(c, ret, fn, false, true);
  PhysicalDeclaredContext v = physical_frame_context(c, *out);
  v.energy_transfer = 0;
  out->declared_context = physical_frame_intern(c, n, v, out);
  if (out->state == PHYSICAL_ERROR)
    return true;
  out->state = PHYSICAL_DECLARED_TRUST;
  out->origin = actual.origin;
  out->loss = n;
  out->operation_rule = physical_state(c)->policy.energy_transfers[h - 1].id;
  out->reason = "directed energy transfer contract; implementation and "
                "conservation unassessed";
  physical_event(c, n, "energy transfer", expected, actual, *out);
  return true;
}
