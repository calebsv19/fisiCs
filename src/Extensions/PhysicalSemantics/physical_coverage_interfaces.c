// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "Extensions/extension_units_expr_bindings.h"
#include <string.h>
/* Called only for P3's source-defined void/identical-record-pointer boundary.
   Every field must additionally have a valid physical contract, and raw alias
   restoration remains unverified even when its C signature looks identical. */
bool physical_coverage_interface(CompilerContext *c, const ASTNode *call) {
  if (!call || call->type != AST_FUNCTION_CALL)
    return false;
  const ASTNode *fn = fisics_extension_lookup_units_function_binding(
      c, call->functionCall.callee);
  if (!fn || fn->type != AST_FUNCTION_DEFINITION)
    return false;
  for (size_t i = 0; i < call->functionCall.argumentCount; i++) {
    const ASTNode *arg = call->functionCall.arguments[i];
    if (!arg || arg->type != AST_IDENTIFIER || physical_raw_pointer(c, arg))
      return false;
    const ParsedType *t =
        fisics_extension_lookup_units_symbol_type_binding(c, arg);
    if (!t)
      return false;
    const ASTNode *record = t->inlineStructOrUnionDef;
    if (!record && t->kind == TYPE_STRUCT && t->userTypeName)
      record = cc_tag_definition(c, CC_TAG_STRUCT, t->userTypeName);
    if (!record || record->type != AST_STRUCT_DEFINITION ||
        !record->structDef.fieldCount)
      return false;
    for (size_t j = 0; j < record->structDef.fieldCount; j++) {
      const ASTNode *field = record->structDef.fields[j];
      if (!field || field->type != AST_VARIABLE_DECLARATION)
        return false;
      for (size_t k = 0; k < field->varDecl.varCount; k++)
        if (!physical_known(physical_get(c, field, PHYSICAL_DECL, k)))
          return false;
    }
  }
  return call->functionCall.argumentCount > 0;
}
