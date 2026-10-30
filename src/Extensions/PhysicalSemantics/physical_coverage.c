// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_energy_semantics.h"
#include "Extensions/PhysicalSemantics/physical_coverage.h"
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "Extensions/PhysicalSemantics/physical_representations.h"
#include "Extensions/Units/units_literal_semantics.h"
#include "Extensions/PhysicalSemantics/physical_frame_semantics.h"
#include <stdlib.h>
#include <string.h>
static unsigned trust(PhysicalFact f) {
  return f.state == PHYSICAL_LITERAL_TRUST    ? 1
         : f.state == PHYSICAL_DECLARED_TRUST ? 2
                                              : 0;
}
static size_t find(const PhysicalCoverage *r, const ASTNode *n) {
  for (size_t i = 0; i < r->ast.expressionCount; i++)
    if (r->ast.expressions[i].node == n)
      return i;
  return SIZE_MAX;
}
static bool same(const char *a, const char *b) { return a && !strcmp(a, b); }
static const PhysicalBoundary *event(CompilerContext *c, const ASTNode *n,
                                     const char *op, const ASTNode *owner) {
  FisicsPhysicalState *s = physical_state(c);
  for (size_t i = s->boundary_count; i > 0; i--) {
    const PhysicalBoundary *b = &s->boundaries[i - 1];
    if (b->node == n && same(b->operation, op) &&
        (!owner || b->expected.origin == owner))
      return b;
  }
  return NULL;
}
void physical_coverage_free(PhysicalCoverage *r) {
  fisics_units_coverage_destroy(&r->ast);
  free(r->rows);
  free(r->expressions);
  free(r->trust);
  memset(r, 0, sizeof(*r));
}
bool physical_coverage_collect(CompilerContext *c, const ASTNode *root,
                               const char *name, PhysicalCoveragePolicy policy,
                               PhysicalCoverage *r) {
  memset(r, 0, sizeof(*r));
  r->policy = policy;
  r->controls_valid = true;
  if (!physical_state(c) || !fisics_units_collect_coverage_structural_controls(
                                c, root, name, &r->ast))
    return false;
  r->rows = calloc(r->ast.rowCount ? r->ast.rowCount : 1, sizeof(*r->rows));
  r->expressions = calloc(r->ast.expressionCount ? r->ast.expressionCount : 1,
                          sizeof(*r->expressions));
  r->trust = calloc(r->ast.expressionCount ? r->ast.expressionCount : 1,
                    sizeof(*r->trust));
  if (!r->rows || !r->expressions || !r->trust ||
      getenv("FISICS_TEST_FAIL_PHYSICAL_COVERAGE_ALLOC"))
    return false;
  r->complete = r->ast.enumerationComplete && !r->ast.failed &&
                !physical_state(c)->failed;
  if (physical_state(c)->policy.schema_version>=5 && !r->ast.failed &&
      !physical_state(c)->failed && r->ast.unsupportedNodes) {
    size_t closed=0;
    for(size_t i=0;i<r->ast.rowCount;i++) {
      const FisicsCoverageRow *row=&r->ast.rows[i];
      const ASTNode *site=row->assessment.obligation.site;
      if(same(row->operation,"unsupported") && site && site->type==AST_COMPOUND_LITERAL &&
         (physical_known(physical_get(c,site,PHYSICAL_EXPR,0)) ||
          (physical_get(c,site,PHYSICAL_EXPR,0).storage_structure &&
           (physical_get(c,site,PHYSICAL_EXPR,0).state==PHYSICAL_CONTROL ||
            physical_get(c,site,PHYSICAL_EXPR,0).state==PHYSICAL_LITERAL_TRUST ||
            physical_get(c,site,PHYSICAL_EXPR,0).state==PHYSICAL_DECLARED_TRUST)))) ++closed;
    }
    if(closed==r->ast.unsupportedNodes)r->complete=true;
  }
  /* P3 owns only stable AST keys/enumeration. Physical facts and verdicts are
     independent; no units checked/trust counter is promoted into physical
     proof. */
  for (size_t i = 0; i < r->ast.expressionCount; i++) {
    const FisicsCoverageExpression *e = &r->ast.expressions[i];
    const ASTNode *n = e->node;
    PhysicalFact f = physical_get(c, n, PHYSICAL_EXPR, 0);
    unsigned t = trust(f);
    /* Only the exact scalar *(&identifier) shape has a proven reference edge.
     * Keep its address row required and checked as a C reference operation,
     * not as a quantity-valued pointer. Unresolved aliases stay unknown. */
    if(n->type==AST_UNARY_EXPRESSION && same(n->expr.op,"&") &&
       n->expr.left && n->expr.left->type==AST_IDENTIFIER) {
      PhysicalFact object=physical_get(c,n->expr.left,PHYSICAL_EXPR,0);
      bool paired=false;
      for(size_t j=i+1;j<r->ast.expressionCount;j++) {
        const ASTNode* parent=r->ast.expressions[j].node;
        if(parent->type==AST_UNARY_EXPRESSION && same(parent->expr.op,"*") &&
           parent->expr.left==n) {paired=true;break;}
      }
      if(paired && physical_known(object) && !object.carrier &&
         !object.type.representation) {
        f=(PhysicalFact){.state=PHYSICAL_CONTROL,.origin=n,
                        .reason="matched direct scalar address/dereference"};
      }
    }
    bool propagate =
        n->type == AST_BINARY_EXPRESSION || (physical_state(c)->policy.schema_version >= 3 &&
         n->type == AST_ASSIGNMENT && n->assignment.op &&
         (!strcmp(n->assignment.op,"*=") || !strcmp(n->assignment.op,"/="))) ||
        n->type == AST_UNARY_EXPRESSION ||
        n->type == AST_CAST_EXPRESSION || n->type == AST_TERNARY_EXPRESSION ||
        n->type == AST_COMMA_EXPRESSION || physical_math_recognized(c,n) ||
        (n->type==AST_FUNCTION_CALL && f.operation_rule && (physical_shape_helper_find(&physical_state(c)->policy,f.operation_rule) || physical_frame_transform_find(&physical_state(c)->policy,f.operation_rule) || physical_energy_transfer_find(&physical_state(c)->policy,f.operation_rule)));
    if (propagate)
      for (size_t j = 0; j < e->dependencyCount; j++) {
        if (n->type == AST_TERNARY_EXPRESSION && j == 0)
          continue;
        size_t k = find(r, e->dependencies[j]);
        if (k >= i) {
          r->complete = false;
          continue;
        }
        t |= r->trust[k];
        PhysicalFact child = r->expressions[k];
        if (child.state == PHYSICAL_ERROR)
          f = child;
        else if ((physical_known(f) || f.state == PHYSICAL_CONTROL) &&
                 !physical_known(child) && child.state != PHYSICAL_CONTROL) {
          f = physical_unknown(n, "upstream physical dependency is unverified");
          f.loss = child.loss ? child.loss : e->dependencies[j];
        }
      }
    if (f.state == PHYSICAL_CONTROL && n->type == AST_BINARY_EXPRESSION) {
      const PhysicalBoundary *b = event(c, n, "comparison", NULL);
      if (b && b->result.state == PHYSICAL_UNKNOWN) {
        f = b->result;
      }
    }
    r->expressions[i] = f;
    r->trust[i] = t;
  }
  for (size_t i = 0; i < r->ast.rowCount; i++) {
    const FisicsCoverageRow *a = &r->ast.rows[i];
    const FisicsCoverageObligation *o = &a->assessment.obligation;
    const ASTNode *site = o->site, *node = site;
    const char *op = NULL;
    const ASTNode *owner = NULL;
    switch (o->boundary) {
    case FISICS_COVERAGE_INITIALIZER:
      op = "initializer";
      owner = site;
      if (a->expression < r->ast.expressionCount)
        node = r->ast.expressions[a->expression].node;
      break;
    case FISICS_COVERAGE_ASSIGNMENT:
      op = "assignment";
      break;
    case FISICS_COVERAGE_ARGUMENT:
      op = "argument";
      if (site && site->type == AST_FUNCTION_CALL &&
          o->slot < site->functionCall.argumentCount)
        node = site->functionCall.arguments[o->slot];
      break;
    case FISICS_COVERAGE_RETURN:
      op = "return";
      break;
    default:
      break;
    }
    const PhysicalBoundary *b = op ? event(c, node, op, owner) : NULL;
    PhysicalFact f =
        b ? b->result
          : physical_unknown(node, "physical boundary has no supported rule");
    if (physical_state(c)->policy.schema_version>=5 && site &&
        site->type==AST_COMPOUND_LITERAL && !b)
      f=physical_get(c,site,PHYSICAL_EXPR,0);
    unsigned t = trust(f);
    if (a->expression < r->ast.expressionCount) {
      PhysicalFact e = r->expressions[a->expression];
      t |= r->trust[a->expression];
      if (o->boundary == FISICS_COVERAGE_OPERATION)
        f = e;
      else if (b && f.state != PHYSICAL_LITERAL_TRUST &&
               (e.state == PHYSICAL_UNKNOWN || e.state == PHYSICAL_ERROR))
        f = e;
    }
    /* A pure literal subexpression may use representation trust only when it
       belongs to an already-authored literal boundary; mixed arithmetic never
       adopts a neighboring quantity kind. Do not mark it as inferred physics.
     */
    if (o->boundary == FISICS_COVERAGE_OPERATION &&
        a->expression < r->ast.expressionCount &&
        fisics_units_is_literal_value(site)) {
      for (size_t j = 0; j < r->ast.rowCount; j++) {
        const FisicsCoverageRow *parent = &r->ast.rows[j];
        if (parent->expression >= r->ast.expressionCount ||
            parent->assessment.obligation.boundary == FISICS_COVERAGE_OPERATION)
          continue;
        const ASTNode *value = r->ast.expressions[parent->expression].node;
        const PhysicalBoundary *boundary =
            event(c, value, "initializer", parent->assessment.obligation.site);
        if (!boundary && parent->assessment.obligation.boundary ==
                             FISICS_COVERAGE_ASSIGNMENT)
          boundary =
              event(c, parent->assessment.obligation.site, "assignment", NULL);
        if (!boundary &&
            parent->assessment.obligation.boundary == FISICS_COVERAGE_RETURN)
          boundary =
              event(c, parent->assessment.obligation.site, "return", NULL);
        /* Match a dependency descendant, not merely another literal nearby. */
        size_t start = parent->expression, target = a->expression;
        bool descendant = start == target;
        bool seen[16384] = {false};
        if (start < 16384)
          seen[start] = true;
        for (size_t k = start + 1; k > 0; k--) {
          size_t e = k - 1;
          if (e >= 16384 || !seen[e])
            continue;
          const FisicsCoverageExpression *ex = &r->ast.expressions[e];
          for (size_t d = 0; d < ex->dependencyCount; d++) {
            size_t dep = find(r, ex->dependencies[d]);
            if (dep == target)
              descendant = true;
            if (dep < e && dep < 16384)
              seen[dep] = true;
          }
        }
        if (descendant && boundary &&
            boundary->result.state == PHYSICAL_LITERAL_TRUST) {
          f = boundary->result;
          t = 1;
          break;
        }
      }
    }
    bool interface_trust =
        physical_state(c)->policy.schema_version<6 &&
        a->cInterface && site && physical_coverage_interface(c, site);
    if (interface_trust) {
      f = physical_unknown(
          site, "source-defined void/physical-record interface trust");
      f.state = PHYSICAL_DECLARED_TRUST;
      f.loss = site;
      t = 2;
    }
    /* Literal boundaries deliberately interpret literal expression trees; their
       bare leaves do not acquire kinds. Interface permission never waives
       unknown. */
    if (f.state == PHYSICAL_LITERAL_TRUST)
      t = 1;
    const ParsedType *return_type = &r->ast.function->functionDef.returnType;
    const ASTNode *returned =
        site && site->type == AST_RETURN ? site->returnStmt.returnValue : NULL;
    bool comparison_return =
        o->boundary == FISICS_COVERAGE_RETURN &&
        return_type->kind == TYPE_PRIMITIVE &&
        return_type->primitiveType == TOKEN_BOOL &&
        !return_type->pointerDepth && returned &&
        returned->type == AST_BINARY_EXPRESSION && returned->expr.op &&
        (same(returned->expr.op, "<") || same(returned->expr.op, ">") ||
         same(returned->expr.op, "<=") || same(returned->expr.op, ">=") ||
         same(returned->expr.op, "==") || same(returned->expr.op, "!=") ||
         same(returned->expr.op, "&&") || same(returned->expr.op, "||"));
    bool ctrl = a->scalarControl || comparison_return;
    bool conflict = false;
    if (ctrl) {
      conflict = f.type.kind || f.state == PHYSICAL_ERROR ||
                 a->assessment.state == FISICS_COVERAGE_CONTRADICTORY ||
                 (o->expected.hasDimension &&
                  o->expected.origin == FISICS_COVERAGE_DECLARED) ||
                 (o->actual.hasDimension &&
                  o->actual.origin == FISICS_COVERAGE_DECLARED);
      if (a->expression < r->ast.expressionCount) {
        size_t e = a->expression;
        conflict = conflict || r->expressions[e].type.kind;
        if (o->boundary == FISICS_COVERAGE_OPERATION) {
          const FisicsCoverageExpression *ex = &r->ast.expressions[e];
          for (size_t j = 0; j < ex->dependencyCount; j++) {
            size_t k = find(r, ex->dependencies[j]);
            if (k < r->ast.expressionCount && r->expressions[k].type.kind)
              conflict = true;
          }
        }
      }
      if (conflict) {
        r->controls_valid = false;
        f.reason =
            "ordinary C control carries physical metadata or a contradiction";
      }
    }
    /* Geometric reads/results require a frame even without a destination
     * boundary. Declared implementation permission cannot fill that facet. */
    if(physical_frame_geometric(c,f) && !physical_frame_context(c,f).frame){
      f.state=PHYSICAL_UNKNOWN;
      f.reason="required geometric frame identity is unknown";
      f.loss=node;
    }
    PhysicalCoverageRow *row = &r->rows[i];
    *row = (PhysicalCoverageRow){
        .fact = f, .trust = t, .control = ctrl, .required = !ctrl};
    if (ctrl) {
      ++r->controls;
      continue;
    }
    ++r->required;
    bool known = interface_trust || physical_known(f) ||
                 (o->boundary == FISICS_COVERAGE_INITIALIZER && b &&
                  (f.state == PHYSICAL_CONTROL || f.state == PHYSICAL_LITERAL_TRUST ||
                   f.state == PHYSICAL_DECLARED_TRUST) && f.storage_structure) ||
                 (o->boundary == FISICS_COVERAGE_OPERATION &&
                  (f.state == PHYSICAL_CONTROL ||
                   (f.storage_structure && (f.state == PHYSICAL_LITERAL_TRUST ||
                                            f.state == PHYSICAL_DECLARED_TRUST))));
    bool units_ok = a->assessment.state != FISICS_COVERAGE_CONTRADICTORY;
    if (known && units_ok && !t) {
      row->checked = true;
      ++r->checked;
    } else if (known && units_ok && t && (!(t & 1) || policy.allow_literals) &&
               (!(t & 2) || policy.allow_declared))
      ++r->permitted;
    else
      ++r->blocked;
  }
  /* V6 frame failures are carried by real boundaries/expressions and cannot
   * be waived by literal or interface permission. Full receipt export is separate. */
  r->accepted = r->controls_valid && r->complete && r->required > 0 &&
                r->checked > 0 && !r->blocked;
  return true;
}
