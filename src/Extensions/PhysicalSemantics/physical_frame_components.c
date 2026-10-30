// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_frame_binding.h"
#include "Extensions/PhysicalSemantics/physical_representations.h"
#include "Syntax/symbol_table.h"
#include <string.h>
/* Optional component annotations must refine the enclosing authored contract.
 */
void physical_frame_components(CompilerContext *c, const ParsedType *t,
                               const ASTNode *n, PhysicalFact *f,
                               bool diagnose) {
  if (!f->type.representation)
    return;
  FisicsPhysicalState *s = physical_state(c);
  if (!s->policy.representations[f->type.representation - 1].record)
    return;
  const ASTNode *d = t->inlineStructOrUnionDef;
  if (!d && t->kind == TYPE_STRUCT && t->userTypeName)
    d = cc_tag_definition(c, CC_TAG_STRUCT, t->userTypeName);
  if (!d || d->type != AST_STRUCT_DEFINITION)
    return;
  /* Interning child declarations may grow the table; keep a value copy. */
  const PhysicalDeclaredContext *enclosing =
      physical_declared_context(s, f->declared_context);
  PhysicalDeclaredContext parent =
      enclosing ? *enclosing : (PhysicalDeclaredContext){0};
  unsigned axis = 0;
  for (size_t i = 0; i < d->structDef.fieldCount; i++) {
    ASTNode *entry = d->structDef.fields[i];
    if (!entry || entry->type != AST_VARIABLE_DECLARATION)
      continue;
    for (size_t j = 0; j < entry->varDecl.varCount; j++) {
      axis++;
      PhysicalFact child =
          physical_type(c, astVarDeclTypeAt(entry, j), entry, false, true);
      const PhysicalDeclaredContext *v =
          physical_declared_context(s, child.declared_context);
      if (!v)
        continue;
      if (v->frame != parent.frame || v->axis != axis || v->transform ||
          (v->applied_at && v->applied_at != parent.applied_at) ||
          (v->moment_reference &&
           v->moment_reference != parent.moment_reference) ||
          (v->point_subject && v->point_subject != parent.point_subject) ||
          (v->displacement_from &&
           (v->displacement_from != parent.displacement_from ||
            v->displacement_to != parent.displacement_to))) {
        if (diagnose)
          physical_error(c, n, 4312,
                         "component frame, axis or reference declaration "
                         "contradicts enclosing carrier");
        f->state = PHYSICAL_ERROR;
        return;
      }
    }
  }
}
