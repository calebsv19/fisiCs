// SPDX-License-Identifier: Apache-2.0
/* Finite immutable local snapshots. No mutable heap, pointer, dynamic-index,
 * external-buffer or DOF-name inference. Structural facts are not quantities. */
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "Extensions/Units/units_coverage_report.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/extension_units_expr_table.h"
#include <stdlib.h>
#include <string.h>
#define SNAPSHOT 3u
#define SNAPSHOT_MAX_ELEMENTS 64u
static bool literal_index(const ASTNode *n, size_t *v) {
  if (!n || n->type != AST_NUMBER_LITERAL || !n->valueNode.value) return false;
  const char *s=n->valueNode.value;
  if (!*s) return false;
  /* C leading-zero integer tokens are octal, not decimal slot identities. */
  if(s[0]=='0' && s[1]) return false;
  size_t x=0;
  for (;*s;s++) {
    if (*s<'0'||*s>'9'||x>SNAPSHOT_MAX_ELEMENTS) return false;
    x=x*10+(size_t)(*s-'0');
  }
  if (x>=SNAPSHOT_MAX_ELEMENTS) return false;
  *v=x; return true;
}
static unsigned shape(const ParsedType *t, size_t dims[2]) {
  if (!t || !t->isConst || t->isVolatile || t->isStatic || t->isExtern ||
      t->pointerDepth || t->kind!=TYPE_PRIMITIVE || t->primitiveType!=TOKEN_DOUBLE ||
      !t->derivationCount || t->derivationCount>2) return 0;
  size_t total=1;
  for (size_t i=0;i<t->derivationCount;i++) {
    if (t->derivations[i].kind!=TYPE_DERIVATION_ARRAY) return 0;
    ParsedArrayInfo a=t->derivations[i].as.array;
    if (a.isVLA || !a.hasConstantSize || a.constantSize<=0 ||
        a.constantSize>SNAPSHOT_MAX_ELEMENTS) return 0;
    dims[i]=(size_t)a.constantSize;
    if (total>SNAPSHOT_MAX_ELEMENTS/dims[i]) return 0;
    total*=dims[i];
  }
  return (unsigned)t->derivationCount;
}
static bool initializer(ASTNode *n, const size_t *dims, unsigned rank) {
  if (!n || n->type!=AST_COMPOUND_LITERAL || !rank ||
      n->compoundLiteral.entryCount!=dims[0]) return false;
  for (size_t i=0;i<n->compoundLiteral.entryCount;i++) {
    DesignatedInit *e=n->compoundLiteral.entries[i];
    if (!e || e->fieldName || e->indexExpr || !e->expression) return false;
    if (rank>1 && !initializer(e->expression,dims+1,rank-1)) return false;
    if (rank==1 && e->expression->type==AST_COMPOUND_LITERAL) return false;
  }
  return true;
}
static ASTNode *parent(const FisicsCoverageReport *r, const ASTNode *child) {
  for (size_t i=0;i<r->expressionCount;i++) {
    const FisicsCoverageExpression *e=&r->expressions[i];
    for (size_t j=0;j<e->dependencyCount;j++)
      if (e->dependencies[j]==child) return (ASTNode*)e->node;
  }
  return NULL;
}
static void mark_compounds(CompilerContext *c,ASTNode *n) {
  physical_put(c,n,SNAPSHOT,0,(PhysicalFact){.state=PHYSICAL_CONTROL,.origin=n,
      .reason="eligible immutable local snapshot initializer",.storage_component=true,.storage_structure=true});
  for(size_t i=0;i<n->compoundLiteral.entryCount;i++) {
    ASTNode *e=n->compoundLiteral.entries[i]->expression;
    if(e->type==AST_COMPOUND_LITERAL)mark_compounds(c,e);
  }
}
/* Avoid a second coverage walk for ordinary scalar functions. A label/goto
 * may bypass initialization; this finite contract deliberately refuses it. */
static bool local_candidates(ASTNode *n,unsigned depth,bool *unsafe) {
  if(!n)return false;
  if(depth>128){*unsafe=true;return false;}
  bool any=false;
  switch(n->type) {
  case AST_BLOCK:
    for(size_t i=0;i<n->block.statementCount;i++)
      any=local_candidates(n->block.statements[i],depth+1,unsafe)||any;
    break;
  case AST_VARIABLE_DECLARATION:
    for(size_t i=0;i<n->varDecl.varCount;i++) {
      size_t dims[2];if(shape(astVarDeclTypeAt(n,i),dims))any=true;
    }
    break;
  case AST_IF_STATEMENT:
    any=local_candidates(n->ifStmt.thenBranch,depth+1,unsafe);
    any=local_candidates(n->ifStmt.elseBranch,depth+1,unsafe)||any;break;
  case AST_FOR_LOOP:
    any=local_candidates(n->forLoop.initializer,depth+1,unsafe);
    any=local_candidates(n->forLoop.body,depth+1,unsafe)||any;break;
  case AST_WHILE_LOOP:any=local_candidates(n->whileLoop.body,depth+1,unsafe);break;
  case AST_GOTO_STATEMENT:case AST_LABEL_DECLARATION:case AST_SWITCH:
  case AST_ASM:case AST_STATEMENT_EXPRESSION:*unsafe=true;break;
  default:break;
  }
  return any;
}
void physical_snapshot_prepare(CompilerContext *c,ASTNode *fn) {
  if (physical_state(c)->policy.schema_version<3) return;
  bool unsafe=false;
  bool any=local_candidates(fn->functionDef.body,0,&unsafe);
  if(!any || unsafe)return;
  FisicsCoverageReport r;
  const char *name=fn->functionDef.funcName->valueNode.value;
  ASTNode program={.type=AST_PROGRAM};
  program.block.statements=&fn;program.block.statementCount=1;
  if(!fisics_units_collect_coverage_structural_controls(c,&program,name,&r)) {
    fisics_units_coverage_destroy(&r); return;
  }
  /* This is a finite proof; oversized/incomplete traversal proves nothing. */
  if(r.failed || r.expressionCount>4096) {fisics_units_coverage_destroy(&r);return;}
  /* Mixed storage does not yet carry concrete unit scales. Never allow the
   * physical-kind path to launder a known unit mismatch into checked status.
   * Refuse this entire finite proof if any concrete-unit expression occurs. */
  for(size_t i=0;i<r.expressionCount;i++) {
    const FisicsUnitsExprResult *u=fisics_extension_lookup_units_expr_result(c,r.expressions[i].node);
    if(u && u->unitResolved) {fisics_units_coverage_destroy(&r);return;}
  }
  for(size_t i=0;i<r.rowCount;i++)
    if(r.rows[i].operation && !strcmp(r.rows[i].operation,"unsupported") &&
       (!r.rows[i].assessment.obligation.site ||
        r.rows[i].assessment.obligation.site->type!=AST_COMPOUND_LITERAL)) {
      fisics_units_coverage_destroy(&r);return;
    }
  for(size_t i=0;i<r.expressionCount;i++) {
    ASTNode *id=(ASTNode*)r.expressions[i].node;
    if(id->type!=AST_IDENTIFIER)continue;
    const ASTNode *root=fisics_units_lookup_origin(c,id);
    const ParsedType *t=fisics_extension_lookup_units_symbol_type_binding(c,id);
    size_t dims[2];unsigned rank=shape(t,dims);
    if(!root || !rank || physical_type(c,t,root,false,false).type.kind)continue;
    DesignatedInit *init=fisics_extension_lookup_units_symbol_initializer_binding(c,id);
    bool local=false;
    for(size_t j=0;j<r.rowCount;j++)
      if(r.rows[j].assessment.obligation.boundary==FISICS_COVERAGE_INITIALIZER &&
         r.rows[j].assessment.obligation.site==root)local=true;
    if(!local || !init || !initializer(init->expression,dims,rank))continue;
    PhysicalFact old=physical_get(c,root,SNAPSHOT,0);
    if(old.state==PHYSICAL_UNKNOWN && old.storage_component)continue;
    ASTNode *at=id;
    bool valid=true;
    for(unsigned k=0;k<rank;k++) {
      ASTNode *p=parent(&r,at);size_t index;
      if(!p || p->type!=AST_ARRAY_ACCESS || p->arrayAccess.array!=at ||
         !literal_index(p->arrayAccess.index,&index) || index>=dims[k]) {valid=false;break;}
      at=p;
    }
    ASTNode *p=parent(&r,at);
    if(p && ((p->type==AST_UNARY_EXPRESSION && p->expr.op &&
                (!strcmp(p->expr.op,"&")||!strcmp(p->expr.op,"++")||!strcmp(p->expr.op,"--"))) ||
             (p->type==AST_ASSIGNMENT && p->assignment.target==at)))valid=false;
    PhysicalFact f={.state=valid?PHYSICAL_CONTROL:PHYSICAL_UNKNOWN,.origin=root,
       .loss=valid?NULL:id,.storage_component=true,
       .reason=valid?"immutable local snapshot; exact access-only uses":"snapshot escapes or has unsupported index/write use"};
    physical_put(c,root,SNAPSHOT,0,f);
  }
  /* Compound markers are installed only after every use has been inspected. */
  for(size_t i=0;i<r.expressionCount;i++) {
    ASTNode *id=(ASTNode*)r.expressions[i].node;
    if(id->type!=AST_IDENTIFIER)continue;
    const ASTNode *root=fisics_units_lookup_origin(c,id);
    PhysicalFact f=physical_get(c,root,SNAPSHOT,0);
    if(f.state==PHYSICAL_CONTROL && f.storage_component) {
      DesignatedInit *init=fisics_extension_lookup_units_symbol_initializer_binding(c,id);
      if(init)mark_compounds(c,init->expression);
    }
  }
  fisics_units_coverage_destroy(&r);
}
/* Aggregate storage is checked structurally, never assigned one physical kind.
 * Every leaf must have its own complete physical provenance. */
bool physical_snapshot_compound(CompilerContext *c,ASTNode *n,PhysicalFact *out) {
  PhysicalFact marker=physical_get(c,n,SNAPSHOT,0);
  if(!marker.storage_component)return false;
  PhysicalFact result=marker;
  for(size_t i=0;i<n->compoundLiteral.entryCount;i++) {
    PhysicalFact f=physical_get(c,n->compoundLiteral.entries[i]->expression,PHYSICAL_EXPR,0);
    if(!physical_known(f) && !(f.storage_structure && (f.state==PHYSICAL_CONTROL || f.state==PHYSICAL_LITERAL_TRUST || f.state==PHYSICAL_DECLARED_TRUST))) {
      *out=physical_unknown(n,"snapshot initializer element lacks physical provenance");
      out->loss=f.loss?f.loss:n->compoundLiteral.entries[i]->expression;return true;
    }
    result=physical_merge_trust(result,f);
  }
  *out=result;return true;
}
bool physical_snapshot_access(CompilerContext *c,ASTNode *n,PhysicalFact *out) {
  ASTNode *root=n;ASTNode *indices[2];unsigned count=0;
  while(root && root->type==AST_ARRAY_ACCESS) {
    if(count==2)return false;
    indices[count++]=root->arrayAccess.index;root=root->arrayAccess.array;
  }
  if(!root || root->type!=AST_IDENTIFIER)return false;
  const ASTNode *origin=fisics_units_lookup_origin(c,root);
  PhysicalFact marker=physical_get(c,origin,SNAPSHOT,0);
  if(!marker.storage_component)return false;
  if(marker.state!=PHYSICAL_CONTROL){*out=marker;return true;}
  if(!count){*out=marker;return true;}
  const ParsedType *t=fisics_extension_lookup_units_symbol_type_binding(c,root);
  size_t dims[2];unsigned rank=shape(t,dims);
  DesignatedInit *init=fisics_extension_lookup_units_symbol_initializer_binding(c,root);
  if(!rank || count>rank || !init){*out=physical_unknown(n,"snapshot shape unresolved");return true;}
  ASTNode *value=init->expression;
  for(unsigned k=0;k<count;k++) {
    size_t ix;
    if(!literal_index(indices[count-1-k],&ix) || ix>=dims[k]) {
      *out=physical_unknown(n,"snapshot index is not an exact in-range literal");return true;
    }
    value=value->compoundLiteral.entries[ix]->expression;
  }
  if(count<rank){*out=marker;return true;}
  *out=physical_get(c,value,PHYSICAL_EXPR,0);
  if(physical_known(*out)) {
    out->storage_component=true;out->reason="exact immutable snapshot element; initializer provenance retained";
  }
  return true;
}
