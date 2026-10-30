// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_representation_binding.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/PhysicalSemantics/physical_frame_semantics.h"
#include "Extensions/extension_units_call_contracts.h"
#include <stdlib.h>
#include <string.h>
static const ParsedType *return_type(const ASTNode *fn) {
  return fn->type == AST_FUNCTION_DEFINITION ? &fn->functionDef.returnType
                                             : &fn->functionDecl.returnType;
}
static bool signature(CompilerContext *c, const ASTNode *fn,
                      const FisicsPhysicalShapeHelper *h) {
  if (!fn || (fn->type != AST_FUNCTION_DEFINITION &&
              fn->type != AST_FUNCTION_DECLARATION))
    return false;
  ASTNode **ps = fn->type == AST_FUNCTION_DEFINITION
                     ? fn->functionDef.parameters
                     : fn->functionDecl.parameters;
  size_t count = fn->type == AST_FUNCTION_DEFINITION
                     ? fn->functionDef.paramCount
                     : fn->functionDecl.paramCount;
  unsigned arity=h->operation==PHYSICAL_SHAPE_NORM?1u:2u;
  if (count != arity ||
      (fn->type == AST_FUNCTION_DEFINITION ? fn->functionDef.isVariadic
                                           : fn->functionDecl.isVariadic))
    return false;
  for (unsigned i = 0; i < 3; i++) {
    if(arity==1 && i==1)continue;
    const ParsedType *t = i == 2 ? return_type(fn)
                          : ps[i] && ps[i]->type == AST_VARIABLE_DECLARATION &&
                                  ps[i]->varDecl.varCount == 1
                              ? astVarDeclTypeAt(ps[i], 0)
                              : NULL;
    PhysicalFact f = physical_type(c, t, i == 2 ? fn : ps[i], false, i == 2);
    if (!physical_known(f) || f.type.kind != h->kinds[i] ||
        (f.type.origin && !(physical_state(c)->policy.schema_version>=6 && !strcmp(physical_state(c)->policy.kinds[f.type.kind-1].id,"quantity.position"))) ||
        f.type.representation != h->representations[i] || f.carrier)
      return false;
    if (!h->representations[i]) {
      const FisicsPhysicalRepresentation *r =
          &physical_state(c)->policy.representations[h->representations[0] - 1];
      if (!t || t->kind != TYPE_PRIMITIVE || t->pointerDepth || t->isLong ||
          t->primitiveType !=
              (r->single_precision ? TOKEN_FLOAT : TOKEN_DOUBLE) ||
          t->derivationCount)
        return false;
    }
  }
  return true;
}
void physical_shape_validate_function(CompilerContext *c, ASTNode *fn) {
  const ParsedType *t = return_type(fn);
  char *id = physical_shape_attribute(t, "fisics::physical_operation");
  if (!id)
    return;
  const FisicsPhysicalShapeHelper *h =
      physical_shape_helper_find(&physical_state(c)->policy, id);
  if (!h || !signature(c, fn, h))
    physical_error(c, fn, 4312,
                   "physical operation requires a known v5 helper identity and "
                   "an exact admitted by-value C signature");
  free(id);
}
bool physical_shape_call(CompilerContext *c, ASTNode *n, PhysicalFact *result) {
  if (physical_state(c)->policy.schema_version < 5)
    return false;
  ASTNode *fn =
      fisics_extension_lookup_units_function_binding(c, n->functionCall.callee);
  if (!fn || (fn->type != AST_FUNCTION_DEFINITION &&
              fn->type != AST_FUNCTION_DECLARATION))
    return false;
  char *id =
      physical_shape_attribute(return_type(fn), "fisics::physical_operation");
  if (!id)
    return false;
  const FisicsPhysicalShapeHelper *h =
      physical_shape_helper_find(&physical_state(c)->policy, id);
  free(id);
  PhysicalFact f =
      physical_unknown(n, "helper signature/identity is unverified");
  if (!h || !signature(c, fn, h) || n->functionCall.argumentCount != (h->operation==PHYSICAL_SHAPE_NORM?1u:2u)) {
    *result = f;
    return true;
  }
  unsigned arity=h->operation==PHYSICAL_SHAPE_NORM?1u:2u;
  PhysicalFact a[2]={{0}};
  bool valid = true;
  for (unsigned i = 0; i < arity; i++) {
    ASTNode *arg = n->functionCall.arguments[i];
    PhysicalFact expected = {.type = {h->kinds[i], 0, h->representations[i]},
                             .state = PHYSICAL_KNOWN,
                             .origin = fn};
    if(physical_state(c)->policy.schema_version>=6)
      expected=physical_get(c,n,PHYSICAL_CALL,i);
    a[i] = physical_get(c, arg, PHYSICAL_EXPR, 0);
    if (physical_raw_pointer(c, arg))
      a[i] = physical_unknown(arg, "cast/raw helper argument is unverified");
    PhysicalFact checked =
        physical_boundary(c, arg, "argument", expected, a[i], false);
    if (!physical_known(checked) || a[i].carrier) {
      valid = false;
      f = checked;
    }
  }
  if (valid) {
    if(arity==1)a[1]=a[0];
    f = physical_merge_trust(a[0], a[1]);
    f.type = (FisicsPhysicalType){h->kinds[2], 0, h->representations[2]};
    f.carrier = false;
    f.component_depth = 0;
    f.state = PHYSICAL_DECLARED_TRUST;
    f.loss = n;
    f.origin = fn;
    f.operation_rule = h->id;
    f.reason = "shape/kind/signature checked; helper implementation and common "
               "right-handed Cartesian basis declared, frames unassessed";
  }
  if(physical_state(c)->policy.schema_version>=6 && valid){
    f=physical_frame_shape(c,n,h->operation,a[0],a[1],f);
    if(physical_known(f)){f.state=PHYSICAL_DECLARED_TRUST;f.loss=n;f.origin=fn;f.operation_rule=h->id;f.reason="storage/kind/frame compatibility checked; helper implementation and basis remain declared trust";}
    if(physical_known(f)){
      PhysicalFact declared=physical_get(c,n,PHYSICAL_CALL,FISICS_UNITS_RETURN_CONTRACT_INDEX);
      f=physical_boundary(c,n,"shape helper result",declared,f,false);
    }
  }
  physical_event(c, n, "physical shape helper", a[0], a[1], f);
  *result = f;
  return true;
}
