// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include "Extensions/PhysicalSemantics/physical_representation_binding.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/extension_units_expr_table.h"
#include <stdlib.h>
#include <string.h>
/* Exact scalar constructors: a single identity return for coupling context,
 * or a single ordered product for work. Declaration-only, indirect, changed
 * body/signature and context-bearing operands cannot claim checked inference.
 */
static const ParsedType *ret(const ASTNode *f) {
  return f->type == AST_FUNCTION_DEFINITION ? &f->functionDef.returnType
                                            : &f->functionDecl.returnType;
}
static const FisicsPhysicalOperationRule *rule(CompilerContext *c, ASTNode *f) {
  if (!f || (f->type != AST_FUNCTION_DEFINITION &&
             f->type != AST_FUNCTION_DECLARATION))
    return NULL;
  char *id = physical_shape_attribute(ret(f), "fisics::physical_operation");
  const FisicsPhysicalOperationRule *r =
      physical_constructor_rule(&physical_state(c)->policy, id);
  free(id);
  return r;
}
static ASTNode *body(CompilerContext *c, ASTNode *f,
                     const FisicsPhysicalOperationRule *r) {
  if (!r || f->type != AST_FUNCTION_DEFINITION || f->functionDef.isVariadic)
    return NULL;
  unsigned arity = r->right ? 2 : 1;
  if (f->functionDef.paramCount != arity)
    return NULL;
  for (unsigned i = 0; i <= arity; i++) {
    ASTNode *ps = i == arity ? f : f->functionDef.parameters[i];
    if (!ps || (i < arity && (ps->type != AST_VARIABLE_DECLARATION ||
                              ps->varDecl.varCount != 1)))
      return NULL;
    const ParsedType *t = i == arity ? ret(f) : astVarDeclTypeAt(ps, 0);
    PhysicalFact a = physical_type(c, t, ps, false, i == arity);
    if (!physical_known(a) || a.carrier || a.type.origin ||
        a.type.representation || a.declared_context ||
        a.type.kind != (i == arity ? r->kind
                        : i        ? r->right
                                   : r->left) ||
        !t || t->kind != TYPE_PRIMITIVE || t->primitiveType != TOKEN_DOUBLE ||
        t->pointerDepth || t->isLong || t->derivationCount)
      return NULL;
  }
  ASTNode *b = f->functionDef.body;
  if (!b || b->type != AST_BLOCK || b->block.statementCount != 1)
    return NULL;
  ASTNode *n = b->block.statements[0];
  if (!n || n->type != AST_RETURN)
    return NULL;
  ASTNode *v = n->returnStmt.returnValue;
  if (!v)
    return NULL;
  if (arity == 2 && (v->type != AST_BINARY_EXPRESSION || !v->expr.op ||
                     strcmp(v->expr.op, "*")))
    return NULL;
  for (unsigned i = 0; i < arity; i++) {
    ASTNode *id = arity == 1 ? v : i ? v->expr.right : v->expr.left;
    ASTNode *param = f->functionDef.parameters[i]->varDecl.varNames[0];
    if (!id || id->type != AST_IDENTIFIER || !param || !id->valueNode.value ||
        !param->valueNode.value ||
        strcmp(id->valueNode.value, param->valueNode.value))
      return NULL;
  }
  return v;
}
bool physical_constructor_validate(CompilerContext *c, ASTNode *f) {
  const FisicsPhysicalOperationRule *r = rule(c, f);
  if (!r)
    return false;
  if (!body(c, f, r))
    physical_error(c, f, 4312,
                   "scalar construction requires an exact double definition "
                   "and single identity/product return body");
  return true;
}
static PhysicalFact derive(CompilerContext *c, ASTNode *n,
                           const FisicsPhysicalOperationRule *r, PhysicalFact a,
                           PhysicalFact b) {
  PhysicalFact f = physical_unknown(
      n, "constructor operands lack exact kind/dimensional provenance");
  const FisicsUnitsExprResult *u =
      fisics_extension_lookup_units_expr_result(c, n);
  if (physical_known(a) && !a.carrier && !a.type.origin &&
      !a.type.representation && !a.declared_context && a.type.kind == r->left &&
      (!r->right || (physical_known(b) && !b.carrier && !b.type.origin &&
                     !b.type.representation && !b.declared_context &&
                     b.type.kind == r->right)) &&
      u && u->resolved) {
    f = r->right ? physical_merge_trust(a, b) : a;
    f.type = (FisicsPhysicalType){r->kind, 0, 0};
    f.operation_rule = r->id;
    f.reason = "explicit scalar construction; exact signature/body and input "
               "provenance checked";
  }
  physical_event(c, n, "scalar construction", a, r->right ? b : a, f);
  return f;
}
bool physical_constructor_return(CompilerContext *c, ASTNode *n, ASTNode *f) {
  const FisicsPhysicalOperationRule *r = rule(c, f);
  ASTNode *v = r ? body(c, f, r) : NULL;
  if (!v || v != n->returnStmt.returnValue)
    return false;
  PhysicalFact a = physical_expression(c, r->right ? v->expr.left : v), b = {0};
  if (r->right)
    b = physical_expression(c, v->expr.right);
  PhysicalFact out = derive(c, v, r, a, b);
  physical_put(c, v, PHYSICAL_EXPR, 0, out);
  physical_boundary(c, n, "return", physical_get(c, f, PHYSICAL_DECL, 0), out,
                    false);
  return true;
}
bool physical_constructor_call(CompilerContext *c, ASTNode *n,
                               PhysicalFact *out) {
  ASTNode *f =
      fisics_extension_lookup_units_function_binding(c, n->functionCall.callee);
  const FisicsPhysicalOperationRule *r = rule(c, f);
  if (!r)
    return false;
  *out = physical_unknown(
      n,
      "constructor definition/signature/body or direct callee is unverified");
  if (physical_raw_pointer(c, n->functionCall.callee) || !body(c, f, r) ||
      n->functionCall.argumentCount != (r->right ? 2u : 1u))
    return true;
  PhysicalFact a = physical_get(c, n->functionCall.arguments[0], PHYSICAL_EXPR,
                                0),
               b = {0};
  if (r->right)
    b = physical_get(c, n->functionCall.arguments[1], PHYSICAL_EXPR, 0);
  *out = derive(c, n, r, a, b);
  for (unsigned i = 0; i < (r->right ? 2u : 1u); i++) {
    PhysicalFact expected = {.type = {i ? r->right : r->left, 0, 0},
                             .state = PHYSICAL_KNOWN,
                             .origin = f};
    PhysicalFact checked =
        physical_boundary(c, n->functionCall.arguments[i], "argument", expected,
                          i ? b : a, false);
    if (!physical_known(checked))
      *out = checked;
  }
  return true;
}
