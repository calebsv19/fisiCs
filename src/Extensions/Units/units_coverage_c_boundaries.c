// SPDX-License-Identifier: Apache-2.0
#include "Extensions/Units/units_coverage_c_boundaries.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Compiler/compiler_context.h"
#include "AST/ast_node.h"
#include <string.h>
#include <stdlib.h>

static bool op(const ASTNode* n,const char* value) {return n && n->expr.op && !strcmp(n->expr.op,value);}
static const FisicsCoverageFact* observed(const FisicsCoverageReport* r,const ASTNode* n) {
    for(size_t i=0;i<r->expressionCount;++i)if(r->expressions[i].node==n)return &r->expressions[i].fact;
    return NULL;
}
static bool predicate(const FisicsCoverageReport* r,const ASTNode* n,unsigned depth) {
    if(!n || depth>192)return false;
    const FisicsCoverageFact* f=observed(r,n);
    if(n->type==AST_IDENTIFIER) {
        const ParsedType* t=fisics_extension_lookup_units_symbol_type_binding(r->ctx,n);
        return t && (r->structuralControls || (!t->attributeCount && f && !f->hasDimension)) &&
            (t->pointerDepth>0 || (t->kind==TYPE_PRIMITIVE && t->primitiveType==TOKEN_BOOL));
    }
    if(n->type==AST_UNARY_EXPRESSION && op(n,"!"))return predicate(r,n->expr.left,depth+1);
    if(n->type==AST_BINARY_EXPRESSION) {
        if(op(n,"&&")||op(n,"||"))return predicate(r,n->expr.left,depth+1)&&predicate(r,n->expr.right,depth+1);
        if(op(n,"<")||op(n,"<=")||op(n,">")||op(n,">=")||op(n,"==")||op(n,"!="))
            return r->structuralControls || (f && f->hasDimension && f->provenanceComplete && fisics_dim_is_dimensionless(f->dimension));
    }
    return false;
}
void fisics_units_coverage_mark_predicate(FisicsCoverageReport* r,const ASTNode* condition,size_t first) {
    if(!predicate(r,condition,0))return;
    for(size_t i=first;i<r->rowCount;++i) {
        FisicsCoverageRow* row=&r->rows[i];const ASTNode* n=row->assessment.obligation.site;
        if(row->assessment.state!=FISICS_COVERAGE_CONTRADICTORY && n &&
           ((n->type==AST_UNARY_EXPRESSION && op(n,"!")) ||
            (n->type==AST_BINARY_EXPRESSION && (op(n,"&&")||op(n,"||")))) && predicate(r,n,0)) {
            row->scalarControl=true;row->controlReason="c_predicate";
        }
    }
}
static bool boolean_type(const FisicsCoverageReport* r,const ParsedType* t,unsigned depth) {
    if(!t || depth>16 || t->pointerDepth || (t->attributeCount && !r->structuralControls))return false;
    if(t->kind==TYPE_PRIMITIVE)return t->primitiveType==TOKEN_BOOL;
    if(t->kind==TYPE_NAMED && t->userTypeName && r->root)for(size_t i=0;i<r->root->block.statementCount;++i) {
        const ASTNode* n=r->root->block.statements[i];
        if(n && n->type==AST_TYPEDEF && n->typedefStmt.alias && n->typedefStmt.alias->valueNode.value &&
            !strcmp(n->typedefStmt.alias->valueNode.value,t->userTypeName))return boolean_type(r,&n->typedefStmt.baseType,depth+1);
    }
    return false;
}
void fisics_units_coverage_mark_status_return(FisicsCoverageReport* r) {
    if(!r->rowCount || !r->function)return;
    const ParsedType* t=&r->function->functionDef.returnType;
    FisicsCoverageRow* row=&r->rows[r->rowCount-1];const ASTNode* site=row->assessment.obligation.site;
    if(!boolean_type(r,t,0) ||
       (!r->structuralControls && row->assessment.obligation.expected.hasDimension) || !site || site->type!=AST_RETURN)return;
    const ASTNode* n=site->returnStmt.returnValue;
    if(!n || n->type!=AST_NUMBER_LITERAL || !n->valueNode.value)return;
    if(strcmp(n->valueNode.value,"0") && strcmp(n->valueNode.value,"1"))return;
    const FisicsCoverageFact* f=observed(r,n);
    if(f && f->hasDimension && fisics_dim_is_dimensionless(f->dimension)) {
        row->scalarControl=true;row->controlReason="c_boolean_status";
    }
}
static const ASTNode* record(const FisicsCoverageReport* r,const ParsedType* t,unsigned depth) {
    if(!t || depth>16)return NULL;
    if(t->inlineStructOrUnionDef && t->inlineStructOrUnionDef->type==AST_STRUCT_DEFINITION)return t->inlineStructOrUnionDef;
    if(t->kind==TYPE_STRUCT && t->userTypeName)return cc_tag_definition(r->ctx,CC_TAG_STRUCT,t->userTypeName);
    if(t->kind==TYPE_NAMED && t->userTypeName && r->root)for(size_t i=0;i<r->root->block.statementCount;++i) {
        const ASTNode* n=r->root->block.statements[i];
        if(n && n->type==AST_TYPEDEF && n->typedefStmt.alias && n->typedefStmt.alias->valueNode.value &&
           !strcmp(n->typedefStmt.alias->valueNode.value,t->userTypeName))return record(r,&n->typedefStmt.baseType,depth+1);
    }
    return NULL;
}
static bool physical_record(const FisicsCoverageReport* r,const ParsedType* t) {
    const ASTNode* n=record(r,t,0);if(!n || n->type!=AST_STRUCT_DEFINITION)return false;
    for(size_t i=0;i<n->structDef.fieldCount;++i) {
        const ASTNode* field=n->structDef.fields[i];
        if(field && field->type==AST_VARIABLE_DECLARATION)for(size_t j=0;j<field->varDecl.varCount;++j)
            if(fisics_coverage_declaration(r->ctx,field,j).hasDimension)return true;
    }
    return false;
}
void fisics_units_coverage_mark_c_interface(FisicsCoverageReport* r,const ASTNode* call,size_t first) {
    const ASTNode* def=fisics_extension_lookup_units_function_binding(r->ctx,call->functionCall.callee);
    if(!def || def->type!=AST_FUNCTION_DEFINITION || !def->functionDef.body || def->functionDef.isVariadic ||
        def->functionDef.paramCount!=call->functionCall.argumentCount || !def->functionDef.paramCount)return;
    const ParsedType* ret=&def->functionDef.returnType;
    if(ret->kind!=TYPE_PRIMITIVE || ret->primitiveType!=TOKEN_VOID || ret->pointerDepth || ret->attributeCount)return;
    for(size_t i=0;i<def->functionDef.paramCount;++i) {
        const ASTNode* p=def->functionDef.parameters[i];
        const ASTNode* arg=call->functionCall.arguments[i];
        if(!p || p->type!=AST_VARIABLE_DECLARATION || p->varDecl.varCount!=1)return;
        const ParsedType* expected=p->varDecl.declaredTypes?&p->varDecl.declaredTypes[0]:&p->varDecl.declaredType;
        const ParsedType* actual=fisics_extension_lookup_units_symbol_type_binding(r->ctx,arg);
        const FisicsCoverageFact* f=observed(r,arg);
        if(!actual || !f || f->hasDimension || expected->pointerDepth!=1 || actual->pointerDepth!=1 ||
            expected->attributeCount || actual->attributeCount || !physical_record(r,expected) ||
            record(r,expected,0)!=record(r,actual,0))return;
    }
    /* A source-defined void/record-pointer signature is a C interface trust
     * boundary, never a claim that this caller checked the callee's body. */
    for(size_t i=first;i<r->rowCount;++i)if(r->rows[i].assessment.obligation.site==call &&
        r->rows[i].assessment.state==FISICS_COVERAGE_UNCHECKED)r->rows[i].cInterface=true;
}
