// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_energy_semantics.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "Extensions/Units/units_literal_semantics.h"
#include "Extensions/extension_units_call_contracts.h"
#include "Extensions/PhysicalSemantics/physical_frame_binding.h"
#include <string.h>
#include "Extensions/PhysicalSemantics/physical_representation_binding.h"
static void declaration(CompilerContext *c, ASTNode *n) {
  if (!n || n->type != AST_VARIABLE_DECLARATION)
    return;
  for (size_t i = 0; i < n->varDecl.varCount; i++) {
    const ParsedType *t = astVarDeclTypeAt(n, i);
    ASTNode *id = n->varDecl.varNames ? n->varDecl.varNames[i] : n;
    physical_validate_type(c, t, id ? id : n, false, 0);
    physical_put(c, n, PHYSICAL_DECL, i,
                 physical_type(c, t, id ? id : n, false, false));
  }
}
static const ParsedType *field(ASTNode *def, const char *name, size_t ordinal,
                               ASTNode **owner) {
  size_t k = 0;
  for (size_t i = 0; i < def->structDef.fieldCount; i++) {
    ASTNode *f = def->structDef.fields[i];
    if (!f || f->type != AST_VARIABLE_DECLARATION)
      continue;
    for (size_t j = 0; j < f->varDecl.varCount; j++, k++) {
      ASTNode *id = f->varDecl.varNames[j];
      if ((name && id && id->valueNode.value &&
           !strcmp(name, id->valueNode.value)) ||
          (!name && k == ordinal)) {
        *owner = id ? id : f;
        return astVarDeclTypeAt(f, j);
      }
    }
  }
  return NULL;
}
void physical_initializer(CompilerContext *c, const ParsedType *t,
                          ASTNode *owner, ASTNode *value, unsigned depth) {
  if (!t || !value)
    return;
  if (depth > 64) {
    physical_error(c, owner, 4312,
                   "physical initializer nesting exceeds supported scope");
    return;
  }
  PhysicalFact snapshot;
  if (value->type==AST_COMPOUND_LITERAL && (physical_snapshot_compound(c,value,&snapshot) || physical_mutable_value(c,value,&snapshot))) {
    physical_event(c,value,"initializer",physical_unknown(owner,"snapshot storage structure"),snapshot,snapshot);
    return;
  }
  if (physical_representation_initializer(c,t,owner,value)) return;
  if (value->type != AST_COMPOUND_LITERAL) {
    physical_boundary(c, value, "initializer",
                      physical_type(c, t, owner, false, false),
                      physical_get(c, value, PHYSICAL_EXPR, 0),
                      fisics_units_is_literal_value(value));
    physical_callable_assignment(c, t, value, owner);
    return;
  }
  if (parsedTypeIsDirectArray(t)) {
    ParsedType elem = parsedTypeArrayElementType(t);
    for (size_t i = 0; i < value->compoundLiteral.entryCount; i++) {
      DesignatedInit *e = value->compoundLiteral.entries[i];
      if (e)
        physical_initializer(c, &elem, owner, e->expression, depth + 1);
    }
    parsedTypeFree(&elem);
    return;
  }
  ASTNode *def = t->inlineStructOrUnionDef;
  if (def && def->type == AST_STRUCT_DEFINITION) {
    size_t ordinal = 0;
    for (size_t i = 0; i < value->compoundLiteral.entryCount; i++) {
      DesignatedInit *e = value->compoundLiteral.entries[i];
      if (!e)
        continue;
      ASTNode *fo = owner;
      const ParsedType *ft = field(def, e->fieldName, ordinal, &fo);
      if (ft)
        physical_initializer(c, ft, fo, e->expression, depth + 1);
      else
        physical_event(
            c, value, "initializer",
            physical_unknown(owner, "aggregate target unresolved"),
            physical_get(c, e->expression, PHYSICAL_EXPR, 0),
            physical_unknown(e->expression,
                             "aggregate initializer mapping is unverified"));
      if (!e->fieldName)
        ordinal++;
    }
    return;
  }
  if (t->kind == TYPE_PRIMITIVE) {
    for (size_t i = 0; i < value->compoundLiteral.entryCount; i++) {
      DesignatedInit *e = value->compoundLiteral.entries[i];
      if (e)
        physical_initializer(c, t, owner, e->expression, depth + 1);
    }
    return;
  }
  physical_event(
      c, value, "initializer",
      physical_unknown(owner, "aggregate target unresolved"),
      physical_unknown(value, "aggregate initializer"),
      physical_unknown(value, "union or unsupported aggregate initialization"));
}
void physical_walk(CompilerContext *c, ASTNode *n, ASTNode *function,
                   unsigned depth) {
  if (!n)
    return;
  if (depth > 512) {
    physical_error(
        c, n, 4312,
        "physical AST traversal limit reached; analysis is incomplete");
    physical_state(c)->failed = true;
    return;
  }
#define WALK(x) physical_walk(c, (x), function, depth + 1)
  switch (n->type) {
  case AST_PROGRAM:
  case AST_BLOCK:
    for (size_t i = 0; i < n->block.statementCount; i++)
      WALK(n->block.statements[i]);
    break;
  case AST_FUNCTION_DECLARATION:
  case AST_FUNCTION_DEFINITION: {
    physical_function_attributes(c, n);
    ParsedType *ret = n->type == AST_FUNCTION_DEFINITION
                          ? &n->functionDef.returnType
                          : &n->functionDecl.returnType;
    physical_validate_type(c, ret, n, false, 0);
    if (!physical_constructor_validate(c,n)) physical_shape_validate_function(c,n);
    physical_frame_validate_function(c,n);
    physical_energy_validate_function(c,n);
    physical_put(c, n, PHYSICAL_DECL, 0, physical_type(c, ret, n, false, true));
    ASTNode **params = n->type == AST_FUNCTION_DEFINITION
                           ? n->functionDef.parameters
                           : n->functionDecl.parameters;
    size_t count = n->type == AST_FUNCTION_DEFINITION
                       ? n->functionDef.paramCount
                       : n->functionDecl.paramCount;
    for (size_t i = 0; i < count; i++)
      WALK(params[i]);
    if (n->type == AST_FUNCTION_DEFINITION)
      physical_snapshot_prepare(c,n);
    if (n->type == AST_FUNCTION_DEFINITION)
      physical_mutable_prepare(c,n);
    if (n->type == AST_FUNCTION_DEFINITION)
      physical_walk(c, n->functionDef.body, n, depth + 1);
    break;
  }
  case AST_VARIABLE_DECLARATION:
    declaration(c, n);
    for (size_t i = 0; i < n->varDecl.varCount; i++) {
      const ParsedType *t = astVarDeclTypeAt(n, i);
      if (t && t->inlineStructOrUnionDef)
        WALK(t->inlineStructOrUnionDef);
      DesignatedInit *init =
          n->varDecl.initializers ? n->varDecl.initializers[i] : NULL;
      if (init) {
        WALK(init->indexExpr);
        WALK(init->expression);
        physical_initializer(c, t,
                             n->varDecl.varNames ? n->varDecl.varNames[i] : n,
                             init->expression, 0);
      }
    }
    WALK(n->varDecl.arraySize);
    WALK(n->varDecl.bitFieldWidth);
    break;
  case AST_TYPEDEF:
    physical_validate_type(c, &n->typedefStmt.baseType, n, true, 0);
    if (n->typedefStmt.baseType.inlineStructOrUnionDef)
      WALK(n->typedefStmt.baseType.inlineStructOrUnionDef);
    break;
  case AST_STRUCT_DEFINITION:
  case AST_UNION_DEFINITION: {
    ParsedType target = {.attributes = n->attributes,
                         .attributeCount = n->attributeCount};
    physical_validate_type(c, &target, n, true, 0);
    for (size_t i = 0; i < n->structDef.fieldCount; i++)
      declaration(c, n->structDef.fields[i]);
    break;
  }
  case AST_ENUM_DEFINITION:
    for (size_t i = 0; i < n->enumDef.memberCount; i++)
      WALK(n->enumDef.values[i]);
    break;
  case AST_IF_STATEMENT:
    WALK(n->ifStmt.condition);
    WALK(n->ifStmt.thenBranch);
    WALK(n->ifStmt.elseBranch);
    break;
  case AST_FOR_LOOP:
    WALK(n->forLoop.initializer);
    WALK(n->forLoop.condition);
    WALK(n->forLoop.increment);
    WALK(n->forLoop.body);
    break;
  case AST_WHILE_LOOP:
    WALK(n->whileLoop.condition);
    WALK(n->whileLoop.body);
    break;
  case AST_SWITCH:
    WALK(n->switchStmt.condition);
    for (size_t i = 0; i < n->switchStmt.caseListSize; i++)
      WALK(n->switchStmt.caseList[i]);
    break;
  case AST_CASE:
    WALK(n->caseStmt.caseValue);
    for (size_t i = 0; i < n->caseStmt.caseBodySize; i++)
      WALK(n->caseStmt.caseBody[i]);
    break;
  case AST_LABEL_DECLARATION:
    WALK(n->label.statement);
    break;
  case AST_RETURN:
    if (physical_constructor_return(c,n,function)) break;
    WALK(n->returnStmt.returnValue);
    if (function)
      physical_boundary(
          c, n, "return", physical_get(c, function, PHYSICAL_DECL, 0),
          physical_get(c, n->returnStmt.returnValue, PHYSICAL_EXPR, 0),
          fisics_units_is_literal_value(n->returnStmt.returnValue));
    break;
  case AST_CONDITIONAL_DIRECTIVE:
    WALK(n->conditionalDirective.body);
    break;
  case AST_STATEMENT_EXPRESSION:
    WALK(n->statementExpr.block);
    physical_put(c, n, PHYSICAL_EXPR, 0,
                 physical_unknown(n, "statement expression is unverified"));
    break;
  case AST_BINARY_EXPRESSION:
  case AST_UNARY_EXPRESSION:
  case AST_SIZEOF:
  case AST_ALIGNOF:
  case AST_SEQUENCE:
    WALK(n->expr.left);
    WALK(n->expr.right);
    physical_expression(c, n);
    break;
  case AST_ASSIGNMENT:
    WALK(n->assignment.target);
    WALK(n->assignment.value);
    physical_expression(c, n);
    break;
  case AST_TERNARY_EXPRESSION:
    WALK(n->ternaryExpr.condition);
    WALK(n->ternaryExpr.trueExpr);
    WALK(n->ternaryExpr.falseExpr);
    physical_expression(c, n);
    break;
  case AST_COMMA_EXPRESSION:
    for (size_t i = 0; i < n->commaExpr.exprCount; i++)
      WALK(n->commaExpr.expressions[i]);
    physical_expression(c, n);
    break;
  case AST_CAST_EXPRESSION:
    WALK(n->castExpr.expression);
    physical_validate_type(c, &n->castExpr.castType, n, true, 0);
    physical_expression(c, n);
    break;
  case AST_ARRAY_ACCESS:
    WALK(n->arrayAccess.array);
    WALK(n->arrayAccess.index);
    physical_expression(c, n);
    break;
  case AST_POINTER_DEREFERENCE:
    WALK(n->pointerDeref.pointer);
    physical_expression(c, n);
    break;
  case AST_POINTER_ACCESS:
  case AST_DOT_ACCESS:
    WALK(n->memberAccess.base);
    physical_expression(c, n);
    break;
  case AST_FUNCTION_CALL:
    WALK(n->functionCall.callee);
    for (size_t i = 0; i < n->functionCall.argumentCount; i++)
      WALK(n->functionCall.arguments[i]);
    physical_expression(c, n);
    break;
  case AST_COMPOUND_LITERAL:
    for (size_t i = 0; i < n->compoundLiteral.entryCount; i++) {
      DesignatedInit *e = n->compoundLiteral.entries[i];
      if (e) {
        WALK(e->indexExpr);
        WALK(e->expression);
      }
    }
    if (n->compoundLiteral.literalType.kind != TYPE_INVALID) {
      physical_validate_type(c, &n->compoundLiteral.literalType, n, true, 0);
      physical_initializer(c, &n->compoundLiteral.literalType, n, n, 0);
    }
    { PhysicalFact snapshot;
      physical_put(c,n,PHYSICAL_EXPR,0,(physical_snapshot_compound(c,n,&snapshot) || physical_mutable_value(c,n,&snapshot))?snapshot:
                   physical_unknown(n,"aggregate expression kind is unverified"));
    }
    break;
  case AST_NUMBER_LITERAL:
  case AST_CHAR_LITERAL:
  case AST_IDENTIFIER:
    physical_expression(c, n);
    break;
  default:
    break;
  }
#undef WALK
}
void fisics_physical_run(ASTNode *root, CompilerContext *c) {
  if (fisics_physical_enabled(c) && physical_state(c))
    physical_walk(c, root, NULL, 0);
}
