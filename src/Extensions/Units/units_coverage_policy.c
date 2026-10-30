// SPDX-License-Identifier: Apache-2.0
#include "Extensions/Units/units_coverage_policy.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "AST/ast_node.h"
#include <stdlib.h>
#include <string.h>

static bool ordinary(const FisicsCoverageReport* r,const ASTNode* n,const ASTNode* counter,unsigned depth) {
    if(!n || depth>192)return false;
    if(n==counter)return true;
    if(n->type==AST_IDENTIFIER)return fisics_units_lookup_origin(r->ctx,n)==counter;
    if(n->type==AST_NUMBER_LITERAL) {
        const char* value=n->valueNode.value;char* end=NULL;
        if(!value || !value[0])return false;
        (void)strtoull(value,&end,0);
        if(end==value)return false;
        while(*end=='u'||*end=='U'||*end=='l'||*end=='L')++end;
        return !*end;
    }
    if(n->type==AST_BINARY_EXPRESSION) {
        const char* op=n->expr.op;
        if(!op || (strcmp(op,"<")&&strcmp(op,"<=")&&strcmp(op,">")&&strcmp(op,">=")&&
            strcmp(op,"==")&&strcmp(op,"!=")&&strcmp(op,"+")&&strcmp(op,"-")))return false;
        return ordinary(r,n->expr.left,counter,depth+1)&&ordinary(r,n->expr.right,counter,depth+1);
    }
    if(n->type==AST_UNARY_EXPRESSION && n->expr.op &&
        (!strcmp(n->expr.op,"++")||!strcmp(n->expr.op,"--")))
        return n->expr.left && n->expr.left->type==AST_IDENTIFIER &&
            fisics_units_lookup_origin(r->ctx,n->expr.left)==counter;
    return false;
}
void fisics_units_coverage_mark_loop_controls(FisicsCoverageReport* r,const ASTNode* init,size_t first,size_t end) {
    if(!init || init->type!=AST_VARIABLE_DECLARATION || init->varDecl.varCount!=1 ||
        !init->varDecl.varNames || !init->varDecl.initializers || !init->varDecl.initializers[0])return;
    const ParsedType* t=init->varDecl.declaredTypes?&init->varDecl.declaredTypes[0]:&init->varDecl.declaredType;
    if(t->kind!=TYPE_PRIMITIVE || t->primitiveType!=TOKEN_INT || t->pointerDepth ||
        t->derivationCount || (t->attributeCount && !r->structuralControls) || t->isVolatile || t->isStatic || t->isExtern)return;
    FisicsCoverageFact declared=fisics_coverage_declaration(r->ctx,init,0);
    if(!r->structuralControls && (declared.hasDimension || declared.hasUnit || declared.reason!=FISICS_COVERAGE_MISSING_ANNOTATION))return;
    const ASTNode* counter=init->varDecl.varNames[0];
    if(!ordinary(r,init->varDecl.initializers[0]->expression,counter,0))return;
    for(size_t i=first;i<end && i<r->rowCount;++i) {
        FisicsCoverageRow* row=&r->rows[i];
        if(row->assessment.state!=FISICS_COVERAGE_CONTRADICTORY &&
            ordinary(r,row->assessment.obligation.site,counter,0))row->scalarControl=true;
    }
}
FisicsCoverageDecision fisics_units_coverage_decide(const FisicsCoverageReport* r,FisicsCoveragePolicy p) {
    FisicsCoverageDecision d={0};d.policy=p;
    if(!p.enabled)return d;
    if(!r || r->failed)return d;
    for(size_t i=0;i<r->rowCount;++i) {
        const FisicsCoverageRow* row=&r->rows[i];
        FisicsCoverageAssessment a=fisics_coverage_assess(row->assessment.obligation);
        if(row->scalarControl && a.state!=FISICS_COVERAGE_CONTRADICTORY){++d.controls;continue;}
        ++d.required;
        if(a.state==FISICS_COVERAGE_UNCHECKED && row->cInterface && p.allowDeclaredBoundaries)++d.acceptedTrust;
        else if(a.state==FISICS_COVERAGE_CHECKED)++d.checked;
        else if(a.state==FISICS_COVERAGE_TRUSTED_BOUNDARY &&
            a.obligation.expected.provenanceComplete && a.obligation.actual.provenanceComplete &&
            ((a.reason==FISICS_COVERAGE_DECLARED_INTERFACE && p.allowDeclaredBoundaries)||
             (a.reason==FISICS_COVERAGE_ASSUMED_REPRESENTATION && p.allowLiteralAssumptions)))++d.acceptedTrust;
        else ++d.blocked;
    }
    d.accepted=r->enumerationComplete && d.required>0 && d.checked>0 && !d.blocked;
    return d;
}
