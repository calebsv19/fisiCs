// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_energy_semantics.h"
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "Extensions/extension_units_expr_table.h"
#include "Extensions/PhysicalSemantics/physical_frame_semantics.h"
PhysicalFact physical_scalar_expression(CompilerContext *c, ASTNode *n,
                                        const char *op, PhysicalFact a,
                                        PhysicalFact b) {
  PhysicalFact f =
      physical_unknown(n, "no admitted scalar rule or operand kind");
  FisicsPhysicalState *s = physical_state(c);
  if (!s || s->policy.schema_version < 3)
    return physical_unknown(n,
                            "unsupported physical operation; P6 rule required");
  if (physical_known(a) && physical_known(b) && !a.carrier && !b.carrier && !a.type.representation && !b.type.representation &&
      !a.type.origin && !b.type.origin) {
    const FisicsPhysicalOperationRule *r = physical_scalar_rule(
        &s->policy, op[0] == '*' ? PHYSICAL_MULTIPLY : PHYSICAL_DIVIDE,
        a.type.kind, b.type.kind);
    const FisicsUnitsExprResult *units =
        fisics_extension_lookup_units_expr_result(c, n);
    /* A validated representation component already has its carrier's Dim8
     * contract even when the generic C field itself is unannotated. The loaded
     * finite rule checked operand/result Dim8; do not infer from C field names. */
    bool component=s->policy.schema_version>=6 &&
      (physical_frame_context(c,a).axis || physical_frame_context(c,b).axis ||
       a.storage_component || b.storage_component);
    if (r && ((units && units->resolved) || component)) {
      f = physical_merge_trust(a, b);
      f.type = (FisicsPhysicalType){r->kind, 0, 0};
      f.carrier = false;
      f.operation_rule = r->id;
      f.reason = "explicit finite scalar rule; input provenance preserved";
    }
  }
  f=physical_frame_scale(c,n,op,a,b,f);
  f=physical_energy_scale(c,n,op,a,b,f);
  physical_event(c, n, "physical scalar operation", a, b, f);
  return f;
}
