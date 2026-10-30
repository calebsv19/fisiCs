// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_energy_semantics.h"
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "Extensions/extension_units_expr_table.h"
#include "Extensions/PhysicalSemantics/physical_frame_semantics.h"
bool physical_math_recognized(CompilerContext *c, const ASTNode *n) {
  FisicsPhysicalState *s = physical_state(c);
  if (!s || s->policy.schema_version < 2 || !n || n->type != AST_FUNCTION_CALL)
    return false;
  FisicsUnitsMathOperation op = fisics_units_math_operation(c, n);
  return (s->policy.schema_version == 7 && physical_cosmic_math_operation(op)) || op == FISICS_UNITS_MATH_FABS || op == FISICS_UNITS_MATH_FMAX ||
      (s->policy.schema_version == 7 && (op == FISICS_UNITS_MATH_FMIN ||
       op == FISICS_UNITS_MATH_EXPM1 || op == FISICS_UNITS_MATH_LOG));
}
bool physical_math_expression(CompilerContext *c, ASTNode *n,
                              PhysicalFact *result) {
  if (!physical_math_recognized(c, n))
    return false;
  FisicsPhysicalState *s = physical_state(c);
  FisicsUnitsMathOperation op = fisics_units_math_operation(c, n);
  size_t count = n->functionCall.argumentCount;
  if (count != (physical_math_unary(op) ? 1u : 2u)) {
    *result = physical_unknown(n, "invalid recognized physical helper arity");
    return true;
  }
  PhysicalFact args[2] = {0};
  const FisicsPhysicalOperationRule *rule = NULL;
  PhysicalFact f =
      physical_unknown(n, "no admitted physical helper rule or operand kind");
  for (size_t i = 0; i < count; i++) {
    args[i] = physical_get(c, n->functionCall.arguments[i], PHYSICAL_EXPR, 0);
    if (physical_known(args[i]) && !args[i].carrier && !args[i].type.representation && !rule)
      rule = physical_operation_rule(&s->policy, op, args[i].type.kind);
  }
  bool transform = physical_cosmic_math_operation(op);
  if (transform) rule = physical_scalar_rule(&s->policy, op, args[0].type.kind, count == 2 ? args[1].type.kind : 0);
  bool known = rule != NULL;
  for (size_t i = 0; i < count; i++) {
    if (args[i].state == PHYSICAL_ERROR) {
      f = args[i];
      known = false;
      break;
    }
    if (!physical_known(args[i]) || (args[i].carrier || args[i].type.representation))
      known = false;
  }
  if (rule && count == 2 && physical_known(args[0]) &&
      physical_known(args[1]) && !args[0].carrier && !args[1].carrier &&
      (args[0].type.kind != args[1].type.kind ||
       args[0].type.origin != args[1].type.origin)) {
    physical_mismatch(c, n,
                      args[0].type.kind != args[1].type.kind ? 4317 : 4318,
                      physical_operation_name(op), args[0], args[1]);
    f = args[0];
    f.state = PHYSICAL_ERROR;
    f.reason = "contradictory physical helper operands";
    known = false;
  }
  const FisicsUnitsExprResult *units =
      fisics_extension_lookup_units_expr_result(c, n);
  bool component=s->policy.schema_version>=6 &&
    physical_frame_context(c,args[0]).axis;
  if (known && ((units && units->resolved) || component)) {
    f = args[0];
    if (count == 2)
      f = physical_merge_trust(f, args[1]);
    f.reason = "identity-checked standard helper; admitted input-derived "
               "physical rule";
    if (transform) {
      f.type = (FisicsPhysicalType){rule->kind, 0, 0};
      /* Scalar norm/math contracts cannot erase a coordinate frame/axis or
       * a reference point. Framed geometry needs its qualified shape helper. */
      for (size_t i = 0; i < count; ++i) {
        PhysicalDeclaredContext v = physical_frame_context(c, args[i]);
        if (args[i].type.origin || v.frame || v.axis || v.applied_at || v.displacement_from || v.displacement_to || v.moment_reference || v.point_subject) {
          f = physical_unknown(n, "scalar math requires unframed operands; use a qualified geometric helper");
          known = false;
        }
      }
      f.declared_context = 0;
    }
    f.operation_rule = rule->id;
  } else if (known) {
    f = physical_unknown(n, "physical helper lacks a valid dimensional result");
    known = false;
  }
  if(!transform && count==2)f=physical_frame_binary(c,n,"conditional",args[0],args[1],f);
  else if(!transform) f=physical_frame_sign(c,n,f);
  if(!transform && count==2)f=physical_energy_binary(c,n,args[0],args[1],f);
  known=known&&physical_known(f);
  for (size_t i = 0; i < count; i++) {
    PhysicalFact argument = known ? args[i] : f;
    physical_event(c, n->functionCall.arguments[i], "argument", args[i],
                   args[i], argument);
  }
  physical_event(c, n, "physical helper", args[0],
                 count == 2 ? args[1] : args[0], f);
  *result = f;
  return true;
}
