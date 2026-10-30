// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_frame_semantics.h"
#include "Extensions/PhysicalSemantics/physical_representation_binding.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/extension_units_call_contracts.h"
#include <stdlib.h>

/* This checks a declaration and its actual input. It does not authenticate a
 * function body, numeric basis, observer state, or successful runtime path. */
bool physical_frame_transform_call(CompilerContext *c, ASTNode *n,
                                    PhysicalFact *result) {
  FisicsPhysicalState *s = physical_state(c);
  if (s->policy.schema_version < 6 || physical_raw_pointer(c,n->functionCall.callee))
    return false;
  ASTNode *fn = fisics_extension_lookup_units_function_binding(c,n->functionCall.callee);
  if (!fn || (fn->type != AST_FUNCTION_DEFINITION && fn->type != AST_FUNCTION_DECLARATION))
    return false;
  const ParsedType *ret = fn->type == AST_FUNCTION_DEFINITION
                            ? &fn->functionDef.returnType : &fn->functionDecl.returnType;
  char *id = physical_shape_attribute(ret,"fisics::frame_transform");
  if (!id) return false;
  uint32_t tid = physical_frame_transform_find(&s->policy,id);
  free(id);
  PhysicalFact f = physical_unknown(n,"unverified directed transform signature or identity");
  if (!tid || n->functionCall.argumentCount != 1 || !physical_frame_function_valid(c,fn)) {
    *result=f;
    return true;
  }
  const PhysicalFrameTransform *t = &s->policy.frame_transforms[tid-1];
  ASTNode *arg=n->functionCall.arguments[0];
  PhysicalFact a=physical_get(c,arg,PHYSICAL_EXPR,0);
  if (physical_raw_pointer(c,arg)) a=physical_unknown(arg,"raw transform argument is unverified");
  PhysicalFact expected=physical_get(c,n,PHYSICAL_CALL,0);
  PhysicalFact checked=physical_boundary(c,arg,"argument",expected,a,false);
  f=checked;
  if (physical_known(checked)) {
    /* Keep actual references, including optional references absent from the
     * signature. A rotation changes only coordinates, never physical points. */
    PhysicalDeclaredContext v=physical_frame_context(c,a);
    if (!v.frame || v.frame!=t->source || v.axis || a.carrier ||
        a.type.kind!=t->input_kind || a.type.representation!=t->input_representation) {
      f=physical_unknown(n,"rotation requires a known exact source vector");
    } else {
      v.frame=t->target;
      v.transform=0;
      v.flags=PHYSICAL_FRAME_CHECKED;
      f=a;
      if(t->operation==PHYSICAL_MAP_POINT) f.type.origin=s->policy.frames[t->target-1].affine_origin;
      f.state=PHYSICAL_DECLARED_TRUST;
      f.origin=fn;
      f.loss=n;
      f.operation_rule=t->id;
      f.reason="directed transform signature and frame compatibility checked; implementation and numeric basis remain declared trust";
      f.declared_context=physical_frame_intern(c,n,v,&f);
      PhysicalFact declared=physical_get(c,n,PHYSICAL_CALL,FISICS_UNITS_RETURN_CONTRACT_INDEX);
      f=physical_boundary(c,n,"rotation result",declared,f,false);
    }
  }
  physical_event(c,n,"directed vector rotation",expected,a,f);
  *result=f;
  return true;
}
