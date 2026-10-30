// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_energy_semantics.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "Extensions/Units/units_literal_semantics.h"
#include "Extensions/extension_units_call_contracts.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/PhysicalSemantics/physical_frame_semantics.h"
#include <string.h>
#include "Extensions/PhysicalSemantics/physical_representation_binding.h"
static PhysicalFact get(CompilerContext *c, ASTNode *n) {
  return physical_get(c, n, PHYSICAL_EXPR, 0);
}
static PhysicalFact error_fact(PhysicalFact a) {
  a.state = PHYSICAL_ERROR;
  a.reason = "contradictory physical operands";
  return a;
}
static bool same(PhysicalFact a, PhysicalFact b) {
  return a.type.kind == b.type.kind && a.type.origin == b.type.origin;
}
static PhysicalFact binary(CompilerContext *c, ASTNode *n, const char *op,
                           PhysicalFact a, PhysicalFact b) {
  if (a.state == PHYSICAL_ERROR)
    return a;
  if (b.state == PHYSICAL_ERROR)
    return b;
  PhysicalFact f =
      physical_unknown(n, "unsupported physical operation; P6 rule required");
  if (!strcmp(op, "*") || !strcmp(op, "/"))
    return physical_scalar_expression(c, n, op, a, b);
  bool add = !strcmp(op, "+") || !strcmp(op, "-");
  bool cmp = !strcmp(op, "==") || !strcmp(op, "!=") || !strcmp(op, "<") ||
             !strcmp(op, "<=") || !strcmp(op, ">") || !strcmp(op, ">=");
  if (add || cmp) {
    if (physical_known(a) && physical_known(b) && !a.carrier && !b.carrier && !a.type.representation && !b.type.representation) {
      FisicsPhysicalReferenceRole ar =
          physical_state(c)->policy.models[a.type.kind - 1].role;
      FisicsPhysicalReferenceRole br =
          physical_state(c)->policy.models[b.type.kind - 1].role;
      if (add &&
          (ar == FISICS_PHYSICAL_ABSOLUTE || br == FISICS_PHYSICAL_ABSOLUTE)) {
        f = physical_affine_binary(c, n, op, a, b);
      } else if (add && (ar == FISICS_PHYSICAL_UNPROFILED ||
                         br == FISICS_PHYSICAL_UNPROFILED)) {
        f = physical_unknown(n, "affine/unprofiled arithmetic deferred");
      } else if (!same(a, b)) {
        physical_mismatch(c, n, a.type.kind != b.type.kind ? 4317 : 4318, op, a,
                          b);
        f = error_fact(a);
      } else if (cmp) {
        f = (PhysicalFact){.state = PHYSICAL_CONTROL,
                           .origin = n,
                           .reason =
                               "comparison boolean is an ordinary C control"};
      } else {
        FisicsPhysicalReferenceRole role =
            physical_state(c)->policy.models[a.type.kind - 1].role;
        if (role == FISICS_PHYSICAL_LINEAR ||
            role == FISICS_PHYSICAL_DIFFERENCE) {
          f = physical_merge_trust(a, b);
          f.reason = "same-kind linear operation";
        } else
          f = physical_unknown(n, "affine/unprofiled arithmetic deferred");
      }
    } else
      f = physical_unknown(n, "missing operand kind or unsupported carrier");
    f=physical_frame_binary(c,n,op,a,b,f);
    if(add)f=physical_energy_binary(c,n,a,b,f);
    physical_event(c, n, cmp ? "comparison" : "linear operation", a, b, f);
    if (cmp && f.state != PHYSICAL_ERROR &&
        (physical_state(c)->policy.schema_version<6 || f.state!=PHYSICAL_UNKNOWN))
      f = (PhysicalFact){.state = PHYSICAL_CONTROL,
                         .origin = n,
                         .reason =
                             "comparison boolean is an ordinary C control"};
  } else if (!strcmp(op, "&&") || !strcmp(op, "||"))
    f = (PhysicalFact){.state = PHYSICAL_CONTROL,
                       .origin = n,
                       .reason = "logical result is an ordinary C control"};
  return f;
}
static const ParsedType *type(CompilerContext *c, ASTNode *n) {
  return fisics_extension_lookup_units_symbol_type_binding(c, n);
}
static bool union_access(CompilerContext *c, ASTNode *base) {
  const ParsedType *t = type(c, base);
  return t && (t->kind == TYPE_UNION ||
               (t->inlineStructOrUnionDef &&
                t->inlineStructOrUnionDef->type == AST_UNION_DEFINITION));
}
/* Pointer casts and raw void storage cannot restore a quantity merely by
 * selecting an annotated field in a cast C record type. Initializer identities
 * come from the captured C symbol binding, not a spelling lookup. */
static bool raw_pointer(CompilerContext *c, ASTNode *n, unsigned depth) {
  if (!n)
    return false;
  if (depth > 32)
    return true;
  if (n->type == AST_CAST_EXPRESSION)
    return n->castExpr.castType.pointerDepth > 0 ||
           raw_pointer(c, n->castExpr.expression, depth + 1);
  if (n->type == AST_IDENTIFIER) {
    const ParsedType *t = type(c, n);
    if (t && t->kind == TYPE_PRIMITIVE && !t->isFunctionPointer &&
        t->pointerDepth > 0 && t->primitiveType == TOKEN_VOID)
      return true;
    DesignatedInit *init =
        fisics_extension_lookup_units_symbol_initializer_binding(c, n);
    return init && init->expression && init->expression != n &&
           raw_pointer(c, init->expression, depth + 1);
  }
  if (n->type == AST_POINTER_ACCESS || n->type == AST_DOT_ACCESS)
    return raw_pointer(c, n->memberAccess.base, depth + 1);
  if (n->type == AST_TERNARY_EXPRESSION)
    return raw_pointer(c, n->ternaryExpr.trueExpr, depth + 1) ||
           raw_pointer(c, n->ternaryExpr.falseExpr, depth + 1);
  if (n->type == AST_COMMA_EXPRESSION && n->commaExpr.exprCount)
    return raw_pointer(c, n->commaExpr.expressions[n->commaExpr.exprCount - 1],
                       depth + 1);
  if (n->type == AST_COMPOUND_LITERAL)
    for (size_t i = 0; i < n->compoundLiteral.entryCount; i++)
      if (n->compoundLiteral.entries[i] &&
          raw_pointer(c, n->compoundLiteral.entries[i]->expression, depth + 1))
        return true;
  if (n->type == AST_ARRAY_ACCESS)
    return raw_pointer(c, n->arrayAccess.array, depth + 1);
  if (n->type == AST_UNARY_EXPRESSION)
    return raw_pointer(c, n->expr.left, depth + 1);
  return false;
}
bool physical_raw_pointer(CompilerContext *c,const ASTNode *n){return raw_pointer(c,(ASTNode*)n,0);}
static PhysicalFact access(CompilerContext *c, ASTNode *n) {
  PhysicalFact snapshot;
  if (physical_snapshot_access(c,n,&snapshot)) return snapshot;
  const ParsedType *t = type(c, n);
  const ASTNode *origin = fisics_units_lookup_origin(c, n);
  PhysicalFact f = physical_type(c, t, origin ? origin : n, false, true);
  ASTNode *base = NULL;
  if (n->type == AST_DOT_ACCESS || n->type == AST_POINTER_ACCESS)
    base = n->memberAccess.base;
  ASTNode *storage = base;
  if (n->type == AST_ARRAY_ACCESS)
    storage = n->arrayAccess.array;
  if (n->type == AST_POINTER_DEREFERENCE)
    storage = n->pointerDeref.pointer;
  if (n->type == AST_UNARY_EXPRESSION)
    storage = n->expr.left;
  if (raw_pointer(c, storage, 0))
    return physical_unknown(n, "raw pointer/cast storage is unverified");
  if (base && union_access(c, base))
    return physical_unknown(n, "union/type-punning storage is unverified");
  PhysicalFact component;
  if (physical_representation_access(c,n,&component)) {
    if (f.type.kind && (f.type.kind!=component.type.kind || f.type.origin!=component.type.origin)) {
      physical_error(c,n,4316,"component contract contradicts homogeneous representation");
      component.state=PHYSICAL_ERROR;
    }
    return component;
  }
  if (base) {
    const ParsedType *bt = type(c, base);
    ASTNode *def = bt ? bt->inlineStructOrUnionDef : NULL;
    if (def && def->type == AST_STRUCT_DEFINITION)
      for (size_t i = 0; i < def->structDef.fieldCount; i++) {
        ASTNode *field = def->structDef.fields[i];
        if (!field || field->type != AST_VARIABLE_DECLARATION)
          continue;
        for (size_t j = 0; j < field->varDecl.varCount; j++) {
          ASTNode *id = field->varDecl.varNames[j];
          if (id && id->valueNode.value &&
              !strcmp(id->valueNode.value, n->memberAccess.field))
            f.origin = id;
        }
      }
  }
  if(physical_state(c)->policy.schema_version>=6 && base &&
     physical_frame_context(c,f).frame && !physical_known(physical_get(c,base,PHYSICAL_EXPR,0)))
    return physical_unknown(n,"framed field access requires a verified enclosing carrier");
  return f;
}
void physical_callable_assignment(CompilerContext *c,
                                  const ParsedType *expected, ASTNode *value,
                                  ASTNode *owner) {
  if (!expected || !value)
    return;
  const ParsedType *a = NULL, *b = NULL;
  size_t na = 0, nb = 0;
  bool va = false, vb = false, ha = false, hb = false;
  if (!parsedTypeGetEffectiveFunctionPointerSignature(expected, &a, &na, &va,
                                                      &ha))
    return;
  const ParsedType *actual = type(c, value);
  ASTNode *fn = fisics_extension_lookup_units_function_binding(c, value);
  if (actual && parsedTypeGetEffectiveFunctionPointerSignature(actual, &b, &nb,
                                                               &vb, &hb)) {
    if (na == nb)
      for (size_t i = 0; i < na; i++)
        physical_boundary(c, value, "callback parameter",
                          physical_type(c, &a[i], owner, false, true),
                          physical_type(c, &b[i], value, false, true), false);
  } else if (fn && (fn->type == AST_FUNCTION_DECLARATION ||
                    fn->type == AST_FUNCTION_DEFINITION)) {
    ASTNode **ps = fn->type == AST_FUNCTION_DEFINITION
                       ? fn->functionDef.parameters
                       : fn->functionDecl.parameters;
    size_t count = fn->type == AST_FUNCTION_DEFINITION
                       ? fn->functionDef.paramCount
                       : fn->functionDecl.paramCount;
    size_t slot = 0;
    for (size_t i = 0; i < count; i++)
      if (ps[i] && ps[i]->type == AST_VARIABLE_DECLARATION)
        for (size_t j = 0; j < ps[i]->varDecl.varCount; j++, slot++)
          if (slot < na)
            physical_boundary(c, value, "callback parameter",
                              physical_type(c, &a[slot], owner, false, true),
                              physical_type(c, astVarDeclTypeAt(ps[i], j),
                                            ps[i], false, true),
                              false);
  }
}
PhysicalFact physical_expression(CompilerContext *c, ASTNode *n) {
  PhysicalFact f = physical_unknown(n, "missing physical metadata");
  if (physical_mutable_value(c,n,&f)) {physical_put(c,n,PHYSICAL_EXPR,0,f);return f;}
  switch (n->type) {
  case AST_IDENTIFIER:
  case AST_ARRAY_ACCESS:
  case AST_POINTER_DEREFERENCE:
  case AST_POINTER_ACCESS:
  case AST_DOT_ACCESS:
    f = access(c, n);
    break;
  case AST_NUMBER_LITERAL:
  case AST_CHAR_LITERAL:
    f = physical_unknown(n, "bare literal has no inferred physical kind");
    break;
  case AST_CAST_EXPRESSION:
    if (n->castExpr.castType.kind == TYPE_PRIMITIVE &&
        n->castExpr.castType.pointerDepth == 0 &&
        !n->castExpr.castType.isFunctionPointer &&
        !n->castExpr.castType.isComplex && !n->castExpr.castType.isImaginary &&
        n->castExpr.castType.primitiveType != TOKEN_VOID)
      f = get(c, n->castExpr.expression);
    else
      f = physical_unknown(n, "unsupported cast loses physical identity");
    break;
  case AST_BINARY_EXPRESSION:
    f = binary(c, n, n->expr.op ? n->expr.op : "", get(c, n->expr.left),
               get(c, n->expr.right));
    break;
  case AST_ASSIGNMENT: {
    if (physical_mutable_first_write(c,n,&f)) break;
    PhysicalFact a = get(c, n->assignment.target),
                 b = get(c, n->assignment.value);
    const char *op = n->assignment.op ? n->assignment.op : "=";
    if (strcmp(op, "=")) {
      const char *simple = !strcmp(op, "+=")   ? "+"
                           : !strcmp(op, "-=") ? "-"
                                               : !strcmp(op, "*=") ? "*"
                           : !strcmp(op, "/=") ? "/"
                                               : "?";
      b = binary(c, n, simple, a, b);
    }
    f = physical_boundary(
        c, n, "assignment", a, b,
        !strcmp(op, "=") && !physical_mutable_store(c,n) && fisics_units_is_literal_value(n->assignment.value));
    physical_callable_assignment(c, type(c, n->assignment.target),
                                 n->assignment.value, n);
    break;
  }
  case AST_TERNARY_EXPRESSION: {
    PhysicalFact a = get(c, n->ternaryExpr.trueExpr),
                 b = get(c, n->ternaryExpr.falseExpr);
    if (physical_known(a) && physical_known(b)) {
      if(physical_state(c)->policy.schema_version==7 && a.type.kind==b.type.kind &&
         a.type.kind==fisics_physical_policy_find_kind(&physical_state(c)->policy,"quantity.energy")){
        f=physical_energy_binary(c,n,a,b,physical_merge_trust(a,b));
        physical_event(c,n,"conditional merge",a,b,f);
      }else f = physical_boundary(c, n, "conditional merge", a, b, false);
      if (f.state != PHYSICAL_ERROR)
        f = physical_frame_binary(c,n,"conditional",a,b,physical_merge_trust(a,b));
      f=physical_energy_binary(c,n,a,b,f);
    } else {
      f = physical_unknown(n, "conditional branch kind is unverified");
      if (a.state == PHYSICAL_ERROR || b.state == PHYSICAL_ERROR)
        f.state = PHYSICAL_ERROR;
      physical_event(c, n, "conditional merge", a, b, f);
    }
    break;
  }
  case AST_COMMA_EXPRESSION:
    if (n->commaExpr.exprCount)
      f = get(c, n->commaExpr.expressions[n->commaExpr.exprCount - 1]);
    break;
  case AST_SEQUENCE:
    f = get(c, n->expr.right);
    break;
  case AST_UNARY_EXPRESSION: {
    const char *op = n->expr.op ? n->expr.op : "";
    PhysicalFact a = get(c, n->expr.left);
    if (!strcmp(op, "*") && n->expr.left &&
        n->expr.left->type == AST_UNARY_EXPRESSION && n->expr.left->expr.op &&
        !strcmp(n->expr.left->expr.op, "&"))
      f = get(c, n->expr.left->expr.left);
    else if (!strcmp(op, "*"))
      f = access(c, n);
    else if (!strcmp(op, "+") || !strcmp(op, "-")) {
      if (physical_known(a) && !a.carrier && !a.type.representation) {
        FisicsPhysicalReferenceRole role =
            physical_state(c)->policy.models[a.type.kind - 1].role;
        if (role == FISICS_PHYSICAL_LINEAR ||
            role == FISICS_PHYSICAL_DIFFERENCE)
          f = a;
        else
          f = physical_unknown(n, "point/unprofiled sign operation deferred");
      } else
        f = a;
    } else if (!strcmp(op, "!"))
      f = (PhysicalFact){.state = PHYSICAL_CONTROL,
                         .origin = n,
                         .reason = "logical result is an ordinary C control"};
    else
      f = physical_unknown(n, "unsupported unary/pointer operation");
    if(!strcmp(op,"-"))f=physical_frame_sign(c,n,f);
    break;
  }
  case AST_FUNCTION_CALL: {
    if(physical_constructor_call(c,n,&f)||physical_energy_transfer_call(c,n,&f)||physical_frame_transform_call(c,n,&f)||physical_shape_call(c,n,&f)||physical_math_expression(c,n,&f))break;
    if (raw_pointer(c, n->functionCall.callee, 0)) {
      f = physical_unknown(n, "cast/untracked callable boundary is unverified");
      for (size_t i = 0; i < n->functionCall.argumentCount; i++)
        physical_event(c, n->functionCall.arguments[i], "argument", f,
                       get(c, n->functionCall.arguments[i]), f);
      break;
    }
    bool unbound_shape=false, frame_call_valid=true;
    for (size_t i = 0; i < n->functionCall.argumentCount; i++) {
      PhysicalFact expected = physical_get(c, n, PHYSICAL_CALL, i);
      ASTNode *arg = n->functionCall.arguments[i];
      unbound_shape |= expected.type.representation || get(c,arg).type.representation;
      PhysicalFact checked=physical_boundary(c, arg, "argument", expected, get(c, arg), false);
      PhysicalFact actual=get(c,arg);
      if((expected.type.kind||actual.type.kind||physical_frame_geometric(c,expected)||physical_frame_geometric(c,actual))&&!physical_known(checked))frame_call_valid=false;
    }
    f = physical_get(c, n, PHYSICAL_CALL, FISICS_UNITS_RETURN_CONTRACT_INDEX);
    if(physical_state(c)->policy.schema_version>=6){
      PhysicalDeclaredContext context=physical_frame_context(c,f);
      ASTNode *fn=fisics_extension_lookup_units_function_binding(c,n->functionCall.callee);
      if(!frame_call_valid||!fn||context.transform){
        f=physical_unknown(n,context.transform?"directed transform requires a resolved exact helper contract":"unverified direct function frame boundary");break;
      }
    }
    if(physical_state(c)->policy.schema_version>=6)
      for(size_t i=0;i<n->functionCall.argumentCount;i++)
        f=physical_frame_call_result(c,n,get(c,n->functionCall.arguments[i]),f);
    if ((f.type.representation || unbound_shape) && physical_state(c)->policy.schema_version<6) {
      f=physical_unknown(n,"represented call requires an explicit validated shape helper contract");
      break;
    }
    f=physical_energy_call_result(c,n,f);
    if (physical_known(f)) {
      f.state = PHYSICAL_DECLARED_TRUST;
      f.reason = "declared call result; implementation not proved";
      f.loss = n;
    }
    break;
  }
  case AST_SIZEOF:
  case AST_ALIGNOF:
    f = (PhysicalFact){.state = PHYSICAL_CONTROL,
                       .origin = n,
                       .reason = "ordinary C size control"};
    break;
  default:
    break;
  }
  physical_put(c, n, PHYSICAL_EXPR, 0, f);
  return f;
}
