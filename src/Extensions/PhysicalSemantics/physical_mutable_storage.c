// SPDX-License-Identifier: Apache-2.0
/* Finite straight-line write/read provenance. No pointer/field aliases, joins,
 * loop induction, runtime metadata, or restoration from destination types. */
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "Extensions/Units/units_coverage_report.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/extension_units_expr_table.h"
#include <stdlib.h>
#include <string.h>
#define MUTABLE 4u
#define MAX_CELLS 729u
#define MAX_ROOTS 32u
#define MAX_EXPRESSIONS 4096u
#define INDEX_LIMIT 4096u

typedef struct {
  ASTNode *root;
  size_t dims[2];
  unsigned rank;
  bool valid;
  ASTNode *previous[MAX_CELLS];
} Storage;
typedef struct {
  Storage roots[MAX_ROOTS];
  size_t count;
  ASTNode *statements[MAX_EXPRESSIONS];
  size_t statement_count;
} Preparation;
static PhysicalFact structure(ASTNode *n) {
  return (PhysicalFact){.state=PHYSICAL_CONTROL,.origin=n,
    .reason="finite local mutable storage relationship",.storage_component=true,
    .storage_structure=true};
}
static unsigned shape(const ParsedType *t, size_t dims[2]) {
  if (!t || t->isConst || t->isVolatile || t->isStatic || t->isExtern ||
      t->pointerDepth || t->kind!=TYPE_PRIMITIVE || t->primitiveType!=TOKEN_DOUBLE ||
      !t->derivationCount || t->derivationCount>2) return 0;
  size_t total=1;
  for (size_t i=0;i<t->derivationCount;i++) {
    if(t->derivations[i].kind!=TYPE_DERIVATION_ARRAY) return 0;
    ParsedArrayInfo a=t->derivations[i].as.array;
    if(a.isVLA || !a.hasConstantSize || a.constantSize<=0 ||
       a.constantSize>MAX_CELLS || total>MAX_CELLS/(size_t)a.constantSize) return 0;
    dims[i]=(size_t)a.constantSize;total*=dims[i];
  }
  return (unsigned)t->derivationCount;
}
/* Entire functions with joins or unsupported control remain outside this rule.
 * Preflight finishes before installing any facts. No partial traversal proof. */
static bool statements(ASTNode *n, Preparation *p, unsigned depth) {
  if(!n) return true;
  if(depth>128 || p->statement_count==MAX_EXPRESSIONS) return false;
  switch(n->type) {
  case AST_BLOCK:
    for(size_t i=0;i<n->block.statementCount;i++)
      if(!statements(n->block.statements[i],p,depth+1)) return false;
    return true;
  case AST_VARIABLE_DECLARATION:
    for(size_t i=0;i<n->varDecl.varCount;i++) {
      if(n->varDecl.initializers && n->varDecl.initializers[i]) continue;
      size_t dims[2]={0}; unsigned rank=shape(astVarDeclTypeAt(n,i),dims);
      if(!rank) continue;
      if(p->count==MAX_ROOTS) return false;
      Storage *s=&p->roots[p->count++];s->root=n->varDecl.varNames[i];
      memcpy(s->dims,dims,sizeof(dims));s->rank=rank;s->valid=true;
    }
    break;
  case AST_ASSIGNMENT: case AST_RETURN: case AST_FUNCTION_CALL: break;
  default: return false;
  }
  p->statements[p->statement_count++]=n; return true;
}
static bool candidate(ASTNode *n,unsigned depth) {
  if(!n || depth>128) return false;
  if(n->type==AST_BLOCK) {
    for(size_t i=0;i<n->block.statementCount;i++)
      if(candidate(n->block.statements[i],depth+1)) return true;
  } else if(n->type==AST_VARIABLE_DECLARATION) {
    for(size_t i=0;i<n->varDecl.varCount;i++) {
      size_t dims[2]={0};
      if((!n->varDecl.initializers || !n->varDecl.initializers[i]) &&
         shape(astVarDeclTypeAt(n,i),dims)) return true;
    }
  }
  return false;
}
static bool statement(const Preparation *p, ASTNode *n) {
  for(size_t i=0;i<p->statement_count;i++) if(p->statements[i]==n) return true;
  return false;
}
static ASTNode *parent(const FisicsCoverageReport *r, const ASTNode *n) {
  for(size_t i=0;i<r->expressionCount;i++)
    for(size_t j=0;j<r->expressions[i].dependencyCount;j++)
      if(r->expressions[i].dependencies[j]==n) return (ASTNode*)r->expressions[i].node;
  return NULL;
}
static bool decimal(ASTNode *n,size_t *v) {
  if(!n || n->type!=AST_NUMBER_LITERAL || !n->valueNode.value) return false;
  const char *s=n->valueNode.value;size_t x=0;
  if(!*s) return false;
  /* C leading-zero integer tokens are octal, not decimal slot identities. */
  if(s[0]=='0' && s[1]) return false;
  for(;*s;s++) {
    if(*s<'0'||*s>'9' || x>INDEX_LIMIT/10) return false;
    x=x*10+(size_t)(*s-'0');if(x>INDEX_LIMIT) return false;
  }
  *v=x;return true;
}
/* Integer maps are immutable, local, complete positional unsigned-int arrays.
 * Their complete use set is checked below; no pointer can mutate a map. */
static bool map_info(CompilerContext *c,ASTNode *id,const FisicsCoverageReport *r,
                     ASTNode **value,size_t *count) {
  if(!id || id->type!=AST_IDENTIFIER) return false;
  const ParsedType *t=fisics_extension_lookup_units_symbol_type_binding(c,id);
  const ASTNode *root=fisics_units_lookup_origin(c,id);
  if(!t || !root || !t->isConst || t->isStatic || t->isExtern || t->isVolatile ||
     t->pointerDepth || t->kind!=TYPE_PRIMITIVE || t->primitiveType!=TOKEN_INT ||
     !t->isUnsigned || t->derivationCount!=1 ||
     t->derivations[0].kind!=TYPE_DERIVATION_ARRAY) return false;
  ParsedArrayInfo a=t->derivations[0].as.array;
  if(a.isVLA || !a.hasConstantSize || a.constantSize<=0 || a.constantSize>64) return false;
  bool local=false;
  for(size_t i=0;i<r->rowCount;i++)
    if(r->rows[i].assessment.obligation.boundary==FISICS_COVERAGE_INITIALIZER &&
       r->rows[i].assessment.obligation.site==root) local=true;
  DesignatedInit *init=fisics_extension_lookup_units_symbol_initializer_binding(c,id);
  if(!local || !init || !init->expression || init->expression->type!=AST_COMPOUND_LITERAL ||
     init->expression->compoundLiteral.entryCount!=(size_t)a.constantSize) return false;
  for(size_t i=0;i<(size_t)a.constantSize;i++) {
    DesignatedInit *e=init->expression->compoundLiteral.entries[i];size_t x;
    if(!e || e->fieldName || e->indexExpr || !decimal(e->expression,&x)) return false;
  }
  *value=init->expression;*count=(size_t)a.constantSize;return true;
}
static bool index_value(CompilerContext *c,ASTNode *n,const FisicsCoverageReport *r,
                        size_t *v,unsigned depth) {
  if(depth>32 || !n) return false;
  if(decimal(n,v)) return true;
  if(n->type==AST_BINARY_EXPRESSION && n->expr.op) {
    size_t a,b;
    if(!index_value(c,n->expr.left,r,&a,depth+1) ||
       !index_value(c,n->expr.right,r,&b,depth+1)) return false;
    if(!strcmp(n->expr.op,"+") && a<=INDEX_LIMIT-b) {*v=a+b;return true;}
    if(!strcmp(n->expr.op,"-") && a>=b) {*v=a-b;return true;}
    if(!strcmp(n->expr.op,"*") && (!b || a<=INDEX_LIMIT/b)) {*v=a*b;return true;}
  }
  if(n->type==AST_ARRAY_ACCESS) {
    ASTNode *init;size_t count,i;
    if(map_info(c,n->arrayAccess.array,r,&init,&count) &&
       decimal(n->arrayAccess.index,&i) && i<count)
      return decimal(init->compoundLiteral.entries[i]->expression,v);
  }
  return false;
}
static Storage *storage(CompilerContext *c,Preparation *p,ASTNode *n) {
  const ASTNode *origin=n && n->type==AST_IDENTIFIER?fisics_units_lookup_origin(c,n):NULL;
  for(size_t i=0;i<p->count;i++) if(p->roots[i].root==origin) return &p->roots[i];
  return NULL;
}
static ASTNode *root(ASTNode *n) {
  while(n && n->type==AST_ARRAY_ACCESS) n=n->arrayAccess.array;
  return n;
}
static bool cell(CompilerContext *c,ASTNode *n,Storage *s,const FisicsCoverageReport *r,
                 size_t *slot) {
  ASTNode *ix[2];unsigned rank=0;
  while(n && n->type==AST_ARRAY_ACCESS) {
    if(rank==2) return false;
    ix[rank++]=n->arrayAccess.index;n=n->arrayAccess.array;
  }
  if(rank!=s->rank) return false;
  size_t offset=0;
  for(unsigned i=0;i<rank;i++) {
    size_t x;
    if(!index_value(c,ix[rank-1-i],r,&x,0) || x>=s->dims[i]) return false;
    offset=offset*s->dims[i]+x;
  }
  *slot=offset;return true;
}
static void mark_index(CompilerContext *c,ASTNode *n,const FisicsCoverageReport *r) {
  physical_put(c,n,MUTABLE,0,structure(n));
  if(n->type==AST_BINARY_EXPRESSION) {
    mark_index(c,n->expr.left,r);mark_index(c,n->expr.right,r);
  } else if(n->type==AST_ARRAY_ACCESS) {
    ASTNode *init;size_t count;
    if(map_info(c,n->arrayAccess.array,r,&init,&count)) {
      physical_put(c,n->arrayAccess.array,MUTABLE,0,structure(n->arrayAccess.array));
      mark_index(c,n->arrayAccess.index,r);
      physical_put(c,init,MUTABLE,0,structure(init));
      for(size_t i=0;i<count;i++) mark_index(c,init->compoundLiteral.entries[i]->expression,r);
    }
  }
}
void physical_mutable_prepare(CompilerContext *c,ASTNode *fn) {
  if(physical_state(c)->policy.schema_version!=7 || !candidate(fn->functionDef.body,0)) return;
  Preparation *p=calloc(1,sizeof(*p));
  if(!p) {physical_state(c)->failed=true;physical_error(c,fn,4305,"mutable storage preparation allocation failed");return;}
  if(!statements(fn->functionDef.body,p,0) || !p->count) {free(p);return;}
  FisicsCoverageReport r;ASTNode program={.type=AST_PROGRAM};
  program.block.statements=&fn;program.block.statementCount=1;
  bool ok=fisics_units_collect_coverage_structural_controls(c,&program,fn->functionDef.funcName->valueNode.value,&r);
  if(!ok || r.failed || r.expressionCount>MAX_EXPRESSIONS) goto done;
  for(size_t i=0;i<r.expressionCount;i++) {
    ASTNode *n=(ASTNode*)r.expressions[i].node;
    const FisicsUnitsExprResult *u=fisics_extension_lookup_units_expr_result(c,n);
    if(u && u->unitResolved) goto done;
    /* Nested assignments or conditional evaluation are not straight-line. */
    if((n->type==AST_ASSIGNMENT && !statement(p,n)) || n->type==AST_TERNARY_EXPRESSION ||
       n->type==AST_COMMA_EXPRESSION || n->type==AST_STATEMENT_EXPRESSION) goto done;
    if(n->type!=AST_IDENTIFIER) continue;
    Storage *s=storage(c,p,n);
    if(s) {
      const ParsedType *bound=fisics_extension_lookup_units_symbol_type_binding(c,n);
      if(physical_type(c,bound,s->root,false,false).type.kind) s->valid=false;
      ASTNode *at=n;
      for(unsigned j=0;j<s->rank;j++) {
        ASTNode *next=parent(&r,at);
        if(!next || next->type!=AST_ARRAY_ACCESS || next->arrayAccess.array!=at) {s->valid=false;break;}
        at=next;
      }
      size_t slot;
      if(!s->valid || !cell(c,at,s,&r,&slot)) {s->valid=false;continue;}
      ASTNode *par=parent(&r,at);
      if(par && ((par->type==AST_UNARY_EXPRESSION && par->expr.op &&
          (!strcmp(par->expr.op,"&") || !strcmp(par->expr.op,"++") || !strcmp(par->expr.op,"--"))) ||
          par->type==AST_POINTER_DEREFERENCE)) s->valid=false;
    }
    ASTNode *init;size_t count;
    if(map_info(c,n,&r,&init,&count)) {
      ASTNode *access=parent(&r,n);size_t x;
      if(!access || access->type!=AST_ARRAY_ACCESS || access->arrayAccess.array!=n ||
         !index_value(c,access,&r,&x,0)) goto done;
      ASTNode *at=access,*par=parent(&r,at);
      while(par && par->type==AST_BINARY_EXPRESSION) {at=par;par=parent(&r,at);}
      if(!par || par->type!=AST_ARRAY_ACCESS || par->arrayAccess.index!=at ||
         !storage(c,p,root(par))) goto done;
    }
  }
  /* The expression graph is postorder, matching the ordinary semantic walk.
   * Each occurrence binds the preceding write, never the final slot state. */
  for(size_t i=0;i<r.expressionCount;i++) {
    ASTNode *n=(ASTNode*)r.expressions[i].node;
    Storage *s=storage(c,p,root(n));
    if(s && s->valid) {
      physical_put(c,n,MUTABLE,0,structure(n));
      if(n->type==AST_ARRAY_ACCESS) {
        mark_index(c,n->arrayAccess.index,&r);size_t slot;
        if(cell(c,n,s,&r,&slot)) {
          PhysicalFact f=structure(n);f.origin=s->previous[slot];f.storage_structure=false;
          if(!f.origin) {f.state=PHYSICAL_UNKNOWN;f.loss=n;f.reason="mutable element has no preceding checked write";}
          physical_put(c,n,MUTABLE,0,f);
        }
      }
    }
    if(n->type==AST_ASSIGNMENT) {
      ASTNode *target=n->assignment.target;Storage *a=storage(c,p,root(target));size_t slot;
      if(a && a->valid && cell(c,target,a,&r,&slot)) {
        physical_put(c,n,MUTABLE,2,structure(n));
        if(!a->previous[slot] && n->assignment.op && !strcmp(n->assignment.op,"="))
          physical_put(c,n,MUTABLE,1,structure(n));
        a->previous[slot]=n;
      }
    }
  }
done:
  fisics_units_coverage_destroy(&r);free(p);
}
bool physical_mutable_value(CompilerContext *c,ASTNode *n,PhysicalFact *out) {
  PhysicalFact f=physical_get(c,n,MUTABLE,0);
  if(!f.storage_component) return false;
  if(f.storage_structure || f.state==PHYSICAL_UNKNOWN) {*out=f;return true;}
  *out=physical_get(c,f.origin,PHYSICAL_EXPR,0);
  out->storage_component=true;out->reason="mutable element retains preceding write provenance";
  return true;
}
bool physical_mutable_first_write(CompilerContext *c,ASTNode *n,PhysicalFact *out) {
  PhysicalFact marker=physical_get(c,n,MUTABLE,1);
  if(!marker.storage_component) return false;
  PhysicalFact value=physical_get(c,n->assignment.value,PHYSICAL_EXPR,0);
  *out=value;
  if(!physical_known(value)) {
    *out=physical_unknown(n,"first mutable write lacks physical provenance");
    out->loss=value.loss?value.loss:n->assignment.value;
    if(value.state==PHYSICAL_ERROR) out->state=PHYSICAL_ERROR;
  }
  physical_event(c,n,"assignment",value,value,*out);return true;
}

bool physical_mutable_store(CompilerContext *c,ASTNode *n) {
  return physical_get(c,n,MUTABLE,2).storage_component;
}
