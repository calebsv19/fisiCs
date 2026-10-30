// SPDX-License-Identifier: Apache-2.0
#include "Extensions/Units/units_coverage_report.h"
#include "Extensions/Units/units_coverage_policy.h"
#include "Extensions/Units/units_coverage_c_boundaries.h"
#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/extension_hooks.h"
#include "Extensions/extension_units_call_contracts.h"
#include "Extensions/Units/units_literal_semantics.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DEPTH 192
static size_t expression(FisicsCoverageReport*,const ASTNode*,unsigned);
static void walk(FisicsCoverageReport*,const ASTNode*,unsigned);
static bool grow(void** data,size_t* capacity,size_t count,size_t size) {
    if (count<*capacity) return true;
    if (*capacity >= 16384) return false;
    size_t next=*capacity?*capacity*2:32;
    void* p=realloc(*data,next*size); if (!p) return false;
    *data=p;*capacity=next;return true;
}
static FisicsCoverageFact absent(const ASTNode* node,FisicsCoverageReason reason) {
    FisicsCoverageFact f={0};f.site=node;f.reason=reason;
    if(node)f.location=node->location;
    return f;
}
static void add(FisicsCoverageReport* r,const ASTNode* site,FisicsCoverageBoundary boundary,
    size_t slot,const char* operation,FisicsCoverageFact expected,FisicsCoverageFact actual,size_t result) {
    if (!grow((void**)&r->rows,&r->rowCapacity,r->rowCount,sizeof(*r->rows))) {r->failed=true;return;}
    if(site && site->type==AST_VARIABLE_DECLARATION && site->varDecl.varNames && slot<site->varDecl.varCount) {
        site=site->varDecl.varNames[slot];expected.site=site;if(site)expected.location=site->location;
    }
    FisicsCoverageObligation o={0};o.site=site;o.boundary=boundary;o.slot=slot;
    o.expected=expected;o.actual=actual;
    FisicsCoverageRow* row=&r->rows[r->rowCount++];
    memset(row,0,sizeof(*row));row->assessment=fisics_coverage_assess(o);row->operation=operation;row->expression=result;
    /* Preserve an observed upstream loss rather than blaming only the consumer. */
    if(row->assessment.state==FISICS_COVERAGE_UNCHECKED && result<r->expressionCount &&
        r->expressions[result].loss) row->assessment.lossNode=r->expressions[result].loss;
}
static FisicsCoverageFact contract(const FisicsUnitsCallArgContract* c,const ASTNode* site) {
    if(site && site->type==AST_FUNCTION_DEFINITION)site=site->functionDef.funcName;
    else if(site && site->type==AST_FUNCTION_DECLARATION)site=site->functionDecl.funcName;
    else if(site && site->type==AST_VARIABLE_DECLARATION && site->varDecl.varNames)site=site->varDecl.varNames[0];
    FisicsCoverageFact f=absent(site,FISICS_COVERAGE_MISSING_ANNOTATION);
    if(c && c->resolved) { f.hasDimension=true;f.dimension=c->dim;f.hasUnit=c->unitResolved;
        f.unit=c->unitDef;f.origin=FISICS_COVERAGE_DECLARED;f.provenanceComplete=site!=NULL;
        f.originNode=site;if(site)f.originLocation=site->location;f.provenanceComplete=site!=NULL;
        f.reason=site?FISICS_COVERAGE_NONE:FISICS_COVERAGE_UNTRACKED_PROVENANCE; }
    return f;
}
static bool same(const char* a,const char* b){return a && !strcmp(a,b);}
static void unsupported(FisicsCoverageReport* r,const ASTNode* n) {
    r->enumerationComplete=false;++r->unsupportedNodes;
    FisicsCoverageFact f=absent(n,FISICS_COVERAGE_NO_INFERENCE);
    add(r,n,FISICS_COVERAGE_OPERATION,SIZE_MAX,"unsupported",f,f,SIZE_MAX);
}
static size_t expression(FisicsCoverageReport* r,const ASTNode* n,unsigned depth) {
    if(!n || r->failed)return SIZE_MAX;
    for(size_t i=0;i<r->expressionCount;++i)if(r->expressions[i].node==n)return i;
    if(depth>MAX_DEPTH){r->failed=true;return SIZE_MAX;}
    const ASTNode** children=NULL;size_t count=0;
    const ASTNode* pair[3]={0};const char* op=NULL;bool operation=false;
    switch(n->type) {
        case AST_BINARY_EXPRESSION:pair[0]=n->expr.left;pair[1]=n->expr.right;count=2;op=n->expr.op;operation=true;break;
        case AST_UNARY_EXPRESSION:pair[0]=n->expr.left;count=1;op=n->expr.op;operation=true;break;
        case AST_CAST_EXPRESSION:pair[0]=n->castExpr.expression;count=1;op="cast";operation=true;break;
        case AST_TERNARY_EXPRESSION:pair[0]=n->ternaryExpr.condition;pair[1]=n->ternaryExpr.trueExpr;pair[2]=n->ternaryExpr.falseExpr;count=3;op="ternary";operation=true;break;
        case AST_ARRAY_ACCESS:pair[0]=n->arrayAccess.array;pair[1]=n->arrayAccess.index;count=2;break;
        case AST_DOT_ACCESS:case AST_POINTER_ACCESS:pair[0]=n->memberAccess.base;count=1;break;
        case AST_POINTER_DEREFERENCE:pair[0]=n->pointerDeref.pointer;count=1;break;
        case AST_ASSIGNMENT:pair[0]=n->assignment.target;pair[1]=n->assignment.value;count=2;break;
        case AST_FUNCTION_CALL:children=(const ASTNode**)n->functionCall.arguments;count=n->functionCall.argumentCount;break;
        case AST_COMMA_EXPRESSION:children=(const ASTNode**)n->commaExpr.expressions;count=n->commaExpr.exprCount;break;
        case AST_IDENTIFIER:case AST_NUMBER_LITERAL:case AST_CHAR_LITERAL:case AST_STRING_LITERAL:break;
        case AST_COMPOUND_LITERAL:
            for(size_t i=0;i<n->compoundLiteral.entryCount;++i)if(n->compoundLiteral.entries[i])
                (void)expression(r,n->compoundLiteral.entries[i]->expression,depth+1);
            unsupported(r,n);break;
        case AST_STATEMENT_EXPRESSION:walk(r,n->statementExpr.block,depth+1);unsupported(r,n);break;
        default:unsupported(r,n);break;
    }
    if(!children)children=pair;
    size_t* ids=count?calloc(count,sizeof(*ids)):NULL;
    if(count>16384){free(ids);r->failed=true;return SIZE_MAX;}
    if(count && !ids){r->failed=true;return SIZE_MAX;}
    for(size_t i=0;i<count;++i) ids[i]=expression(r,children[i],depth+1);
    FisicsCoverageFact f=fisics_coverage_expression(r->ctx,n);
    const ASTNode* origin=fisics_units_lookup_origin(r->ctx,n);
    if(origin) {f.originNode=origin;f.originLocation=origin->location;
        if(f.hasDimension && f.origin==FISICS_COVERAGE_DECLARED &&
            f.reason==FISICS_COVERAGE_UNTRACKED_PROVENANCE){f.provenanceComplete=true;f.reason=FISICS_COVERAGE_NONE;}}
    if((n->type==AST_NUMBER_LITERAL || n->type==AST_CHAR_LITERAL) && f.hasDimension &&
        fisics_dim_is_dimensionless(f.dimension) && !f.hasUnit) {
        f.originNode=n;f.originLocation=n->location;f.provenanceComplete=true;f.reason=FISICS_COVERAGE_NONE;
    }
    const ASTNode* loss=f.hasDimension?NULL:n;
    FisicsCoverageFact left=count && ids[0]!=SIZE_MAX?r->expressions[ids[0]].fact:absent(n,FISICS_COVERAGE_NO_INFERENCE);
    FisicsCoverageFact right=count>1 && ids[1]!=SIZE_MAX?r->expressions[ids[1]].fact:absent(n,FISICS_COVERAGE_NO_INFERENCE);
    FisicsCoverageFact expected=f;bool compatible=true,rule=false;
    if(operation) {
        loss=NULL;
        if(n->type==AST_BINARY_EXPRESSION) {
            if(same(op,"*")||same(op,"/")) {
                rule=left.hasDimension&&right.hasDimension;
                if(rule)rule=same(op,"*")?fisics_dim_add(left.dimension,right.dimension,&expected.dimension):
                    fisics_dim_sub(left.dimension,right.dimension,&expected.dimension);
            } else if(same(op,"+")||same(op,"-")||same(op,"<")||same(op,">")||same(op,"<=")||same(op,">=")||same(op,"==")||same(op,"!=")) {
                rule=left.hasDimension&&right.hasDimension;
                compatible=rule&&fisics_dim_equal(left.dimension,right.dimension);
                expected.dimension=(same(op,"+")||same(op,"-"))?left.dimension:fisics_dim_zero();
            }
        } else if(n->type==AST_TERNARY_EXPRESSION && count==3 && ids[2]!=SIZE_MAX) {
            left=right;right=r->expressions[ids[2]].fact;rule=left.hasDimension&&right.hasDimension;
            compatible=rule&&fisics_dim_equal(left.dimension,right.dimension);expected.dimension=left.dimension;
        } else if(same(op,"cast")||same(op,"+")||same(op,"-")) {rule=left.hasDimension;expected.dimension=left.dimension;}
        expected.hasDimension=rule;expected.hasUnit=false;expected.unit=NULL;
        expected.origin=FISICS_COVERAGE_INFERRED;expected.site=n;
        expected.originNode=n;expected.originLocation=n->location;
        expected.provenanceComplete=rule;expected.reason=FISICS_COVERAGE_NONE;
        bool complete=rule,trust=false;
        for(size_t i=0;i<count;++i)if(ids[i]!=SIZE_MAX) {
            const FisicsCoverageExpression* child=&r->expressions[ids[i]];
            if(n->type==AST_TERNARY_EXPRESSION && i==0)continue;
            complete=complete&&child->fact.hasDimension&&child->fact.provenanceComplete;
            trust=trust||child->fact.trusted;
            if(!child->fact.hasDimension || !child->fact.provenanceComplete) {
                if(!loss)loss=child->loss?child->loss:child->node;
                if(f.reason==FISICS_COVERAGE_UNTRACKED_PROVENANCE || !f.hasDimension)f.reason=child->fact.reason;
            }
        }
        if(rule && compatible && f.hasDimension && fisics_dim_equal(expected.dimension,f.dimension)) {
            f.provenanceComplete=complete;f.originNode=n;f.originLocation=n->location;
            f.trusted=trust;f.reason=trust?FISICS_COVERAGE_DECLARED_INTERFACE:
                complete?FISICS_COVERAGE_NONE:FISICS_COVERAGE_UNTRACKED_PROVENANCE;
        } else if(!rule) {f.hasDimension=false;f.provenanceComplete=false;
            if(!loss)loss=n;if(f.reason==FISICS_COVERAGE_NONE)f.reason=FISICS_COVERAGE_NO_INFERENCE;}
    }
    if(!grow((void**)&r->expressions,&r->expressionCapacity,r->expressionCount,sizeof(*r->expressions))) {free(ids);r->failed=true;return SIZE_MAX;}
    size_t id=r->expressionCount++;
    FisicsCoverageExpression* item=&r->expressions[id];memset(item,0,sizeof(*item));item->node=n;item->fact=f;item->loss=loss;
    if(count) {item->dependencies=malloc(count*sizeof(*item->dependencies));if(!item->dependencies){r->failed=true;free(ids);return SIZE_MAX;}
        memcpy(item->dependencies,children,count*sizeof(*children));item->dependencyCount=count;}
    if(operation) {
        if(rule && (!compatible || ((same(op,"+")||same(op,"-")) && left.hasUnit && right.hasUnit && left.unit!=right.unit))) add(r,n,FISICS_COVERAGE_OPERATION,0,op,left,right,id);
        else add(r,n,FISICS_COVERAGE_OPERATION,0,op,expected,f,id);
    }
    if(n->type==AST_ASSIGNMENT && count==2) {
        FisicsCoverageFact value=right;
        if(same(n->assignment.op,"*=") || same(n->assignment.op,"/=")) {
            value.hasDimension=left.hasDimension && right.hasDimension;
            if(value.hasDimension)value.hasDimension=same(n->assignment.op,"*=")?
                fisics_dim_add(left.dimension,right.dimension,&value.dimension):
                fisics_dim_sub(left.dimension,right.dimension,&value.dimension);
            value.hasUnit=false;value.unit=NULL;
            value.provenanceComplete=left.provenanceComplete&&right.provenanceComplete;
        } else if(!same(n->assignment.op,"=") && !same(n->assignment.op,"+=") && !same(n->assignment.op,"-=")) {
            value.hasDimension=false;value.provenanceComplete=false;value.reason=FISICS_COVERAGE_NO_INFERENCE;
        }
        if(same(n->assignment.op,"=") && left.hasDimension && fisics_units_is_literal_value(n->assignment.value)) {
            value=left;value.site=n->assignment.value;value.location=n->assignment.value->location;
            value.trusted=true;value.reason=FISICS_COVERAGE_ASSUMED_REPRESENTATION;
        }
        add(r,n,FISICS_COVERAGE_ASSIGNMENT,0,n->assignment.op,left,value,ids[1]);
    }
    if(n->type==AST_FUNCTION_CALL) {
        FisicsUnitsMathOperation math=FISICS_UNITS_MATH_NONE;
        if(r->ctx->extensionState)for(size_t i=0;i<r->ctx->extensionState->unitsMathCallCount;++i)
            if(r->ctx->extensionState->unitsMathCalls[i].node==n)math=r->ctx->extensionState->unitsMathCalls[i].operation;
        size_t callRows=r->rowCount;
        const ASTNode* callee=n->functionCall.callee;
        bool conversion=callee && callee->type==AST_IDENTIFIER &&
            (same(callee->valueNode.value,"fisics_convert_unit") ||
             same(callee->valueNode.value,"__builtin_fisics_convert_unit")) &&
            count==2 && ids[0]!=SIZE_MAX && ids[1]!=SIZE_MAX &&
            children[1] && children[1]->type==AST_STRING_LITERAL &&
            left.hasDimension && left.hasUnit && f.hasDimension && f.hasUnit &&
            fisics_dim_equal(left.dimension,f.dimension);
        if(conversion) {
            /* Semantic analysis already validates the seeded target/unit pair.
             * Rebuild only its source proof edge; a resolved result alone is not
             * enough and does not bless raw values or arbitrary calls. */
            f.provenanceComplete=left.provenanceComplete;f.trusted=left.trusted;
            f.reason=left.trusted?left.reason:left.provenanceComplete?
                FISICS_COVERAGE_NONE:FISICS_COVERAGE_UNTRACKED_PROVENANCE;
            f.originNode=n;f.originLocation=n->location;
            r->expressions[id].fact=f;
            r->expressions[id].loss=left.provenanceComplete?NULL:children[0];
            add(r,n,FISICS_COVERAGE_OPERATION,0,"explicit_unit_conversion",f,f,id);
            add(r,n,FISICS_COVERAGE_ARGUMENT,0,"conversion_source",left,left,ids[0]);
            add(r,n,FISICS_COVERAGE_ARGUMENT,1,"conversion_target",right,right,ids[1]);
            if(!r->failed && r->rowCount>callRows) {
                r->rows[r->rowCount-1].scalarControl=true;
                r->rows[r->rowCount-1].controlReason="explicit_unit_target_literal";
            }
        } else if(math && count && ids[0]!=SIZE_MAX) {
            FisicsCoverageFact wanted=left;bool legal=left.hasDimension;
            if(math==FISICS_UNITS_MATH_SQRT)for(int k=0;k<8;++k) {
                if(left.dimension.e[k]%2)legal=false;
                wanted.dimension.e[k]=left.dimension.e[k]/2;
            }
            if((math==FISICS_UNITS_MATH_HYPOT || math==FISICS_UNITS_MATH_FMAX || math==FISICS_UNITS_MATH_FMIN || math==FISICS_UNITS_MATH_ATAN2) && count>1)
                legal=legal&&right.hasDimension&&fisics_dim_equal(left.dimension,right.dimension)&&
                    !(left.hasUnit&&right.hasUnit&&left.unit!=right.unit);
            if(math==FISICS_UNITS_MATH_ATAN2) wanted.dimension=fisics_dim_zero();
            if(math==FISICS_UNITS_MATH_EXPM1 || math==FISICS_UNITS_MATH_LOG) {
                FisicsDim8 zero=fisics_dim_zero();
                legal=legal&&fisics_dim_equal(left.dimension,zero);
                wanted.dimension=zero;
            }
            if(legal && f.hasDimension && fisics_dim_equal(wanted.dimension,f.dimension)) {
                f.provenanceComplete=left.provenanceComplete&&(count==1||right.provenanceComplete);
                f.trusted=left.trusted||(count>1&&right.trusted);f.originNode=n;f.originLocation=n->location;
                f.reason=f.trusted?FISICS_COVERAGE_DECLARED_INTERFACE:f.provenanceComplete?FISICS_COVERAGE_NONE:FISICS_COVERAGE_UNTRACKED_PROVENANCE;
            }
            r->expressions[id].fact=f;
            wanted.hasDimension=legal;wanted.hasUnit=false;wanted.unit=NULL;
            if(count>1 && left.hasDimension && right.hasDimension &&
                (!fisics_dim_equal(left.dimension,right.dimension) ||
                 (left.hasUnit&&right.hasUnit&&left.unit!=right.unit)))
                add(r,n,FISICS_COVERAGE_OPERATION,0,"math_rule",left,right,id);
            else add(r,n,FISICS_COVERAGE_OPERATION,0,"math_rule",wanted,f,id);
        } else add(r,n,FISICS_COVERAGE_OPERATION,0,"call_result",f,f,id);
        const ASTNode* definition=fisics_extension_lookup_units_function_binding(r->ctx,n->functionCall.callee);
        for(size_t i=0;!math && !conversion && i<count;++i)if(ids[i]!=SIZE_MAX) {
            const ASTNode* parameter=NULL;
            if(definition && definition->type==AST_FUNCTION_DEFINITION && i<definition->functionDef.paramCount)
                parameter=definition->functionDef.parameters[i];
            if(definition && definition->type==AST_FUNCTION_DECLARATION && i<definition->functionDecl.paramCount)
                parameter=definition->functionDecl.parameters[i];
            add(r,n,FISICS_COVERAGE_ARGUMENT,i,"argument",contract(fisics_extension_lookup_units_call_arg_contract(r->ctx,n,i),parameter),r->expressions[ids[i]].fact,ids[i]);
        }
        fisics_units_coverage_mark_c_interface(r,n,callRows);
    }
    free(ids);return id;
}
static void walk(FisicsCoverageReport* r,const ASTNode* n,unsigned depth) {
    if(!n || r->failed)return;if(depth>MAX_DEPTH){r->failed=true;return;}
    switch(n->type) {
        case AST_SEQUENCE:walk(r,n->expr.left,depth+1);walk(r,n->expr.right,depth+1);break;
        case AST_BLOCK:case AST_PROGRAM:
            for(size_t i=0;i<n->block.statementCount;++i)walk(r,n->block.statements[i],depth+1);break;
        case AST_VARIABLE_DECLARATION:
            for(size_t i=0;i<n->varDecl.varCount;++i) {
                DesignatedInit* init=n->varDecl.initializers?n->varDecl.initializers[i]:NULL;
                if(!init || !init->expression)continue;
                size_t id=expression(r,init->expression,depth+1);if(id==SIZE_MAX)continue;
                FisicsCoverageFact expected=fisics_coverage_declaration(r->ctx,n,i),actual=r->expressions[id].fact;
                if(expected.hasDimension && fisics_units_is_literal_value(init->expression)) {
                    actual=expected;actual.site=init->expression;actual.location=init->expression->location;
                    actual.trusted=true;actual.reason=FISICS_COVERAGE_ASSUMED_REPRESENTATION;
                }
                add(r,n,FISICS_COVERAGE_INITIALIZER,i,"initializer",expected,actual,id);
            }break;
        case AST_RETURN:if(n->returnStmt.returnValue) {
            size_t id=expression(r,n->returnStmt.returnValue,depth+1);if(id!=SIZE_MAX)
                add(r,n,FISICS_COVERAGE_RETURN,0,"return",contract(fisics_extension_lookup_units_call_arg_contract(r->ctx,r->function,FISICS_UNITS_RETURN_CONTRACT_INDEX),r->function),r->expressions[id].fact,id);
            fisics_units_coverage_mark_status_return(r);
            }break;
        case AST_IF_STATEMENT: {
            size_t first=r->rowCount;expression(r,n->ifStmt.condition,depth+1);
            fisics_units_coverage_mark_predicate(r,n->ifStmt.condition,first);
            walk(r,n->ifStmt.thenBranch,depth+1);walk(r,n->ifStmt.elseBranch,depth+1);break;
        }
        case AST_FOR_LOOP: {
            size_t first=r->rowCount;
            walk(r,n->forLoop.initializer,depth+1);expression(r,n->forLoop.condition,depth+1);expression(r,n->forLoop.increment,depth+1);
            fisics_units_coverage_mark_loop_controls(r,n->forLoop.initializer,first,r->rowCount);
            walk(r,n->forLoop.body,depth+1);break;
        }
        case AST_WHILE_LOOP:expression(r,n->whileLoop.condition,depth+1);walk(r,n->whileLoop.body,depth+1);break;
        case AST_SWITCH:expression(r,n->switchStmt.condition,depth+1);for(size_t i=0;i<n->switchStmt.caseListSize;++i)walk(r,n->switchStmt.caseList[i],depth+1);break;
        case AST_CASE:expression(r,n->caseStmt.caseValue,depth+1);for(size_t i=0;i<n->caseStmt.caseBodySize;++i)walk(r,n->caseStmt.caseBody[i],depth+1);break;
        case AST_LABEL_DECLARATION:walk(r,n->label.statement,depth+1);break;
        case AST_BREAK:case AST_CONTINUE:case AST_GOTO_STATEMENT:case AST_TYPEDEF:case AST_STATIC_ASSERT:case AST_STRUCT_DEFINITION:case AST_UNION_DEFINITION:case AST_ENUM_DEFINITION:break;
        case AST_ASM:unsupported(r,n);break;
        default:expression(r,n,depth+1);break;
    }
}
static bool collect_coverage(const CompilerContext* ctx,const ASTNode* root,const char* name,FisicsCoverageReport* r,bool structuralControls) {
    if(!r)return false;memset(r,0,sizeof(*r));r->ctx=ctx;r->root=root;r->functionName=name;r->enumerationComplete=true;r->structuralControls=structuralControls;
    if(!ctx || !root || root->type!=AST_PROGRAM || !name)return false;
    size_t matches=0;
    for(size_t i=0;i<root->block.statementCount;++i) {
        ASTNode* n=root->block.statements[i];
        if(n && n->type==AST_FUNCTION_DEFINITION && n->functionDef.funcName &&
            same(n->functionDef.funcName->valueNode.value,name)){r->function=n;++matches;}
    }
    if(matches!=1)return false;
    walk(r,r->function->functionDef.body,0);
    if(r->failed)return false;
    for(size_t i=0;i<r->rowCount;++i) {
        FisicsCoverageState state=r->rows[i].assessment.state;++r->summary.total;
        switch(state){case FISICS_COVERAGE_CHECKED:++r->summary.checked;break;
            case FISICS_COVERAGE_TRUSTED_BOUNDARY:++r->summary.trusted;break;
            case FISICS_COVERAGE_UNCHECKED:++r->summary.unchecked;break;
            case FISICS_COVERAGE_CONTRADICTORY:++r->summary.contradictory;break;}
    }
    return true;
}
void fisics_units_coverage_destroy(FisicsCoverageReport* r) {
    if(!r)return;for(size_t i=0;i<r->expressionCount;++i)free(r->expressions[i].dependencies);
    free(r->expressions);free(r->rows);memset(r,0,sizeof(*r));
}

bool fisics_units_collect_coverage(const CompilerContext* c,const ASTNode* n,const char* name,FisicsCoverageReport* r){return collect_coverage(c,n,name,r,false);}
bool fisics_units_collect_coverage_structural_controls(const CompilerContext* c,const ASTNode* n,const char* name,FisicsCoverageReport* r){return collect_coverage(c,n,name,r,true);}
