/* SPDX-License-Identifier: Apache-2.0
 * Internal frontend adapter for optional, bounded source-effect tooling.
 * It exports resolved C declaration identities; it performs no physical proof. */
#include "Compiler/pipeline.h"
#include "AST/ast_node.h"
#include "Extensions/extension_units_expr_bindings.h"
#include "Extensions/extension_units_expr_table.h"
#include "Extensions/extension_units_view.h"
#include <float.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static FILE *out;static CompilerContext *ctx;static int failed;static const char *unit;
static void str(const char *s){fputc('"',out);for(;s&&*s;s++){unsigned char c=(unsigned char)*s;if(c=='"'||c=='\\')fprintf(out,"\\%c",c);else if(c<32)fprintf(out,"\\u%04x",c);else fputc(c,out);}fputc('"',out);}
static void key(const ASTNode *n){size_t cap=strlen(unit)+40;char *s=malloc(cap);if(!s){failed=1;str("allocation-failed");return;}snprintf(s,cap,"%s#%llx",unit,(unsigned long long)(uintptr_t)n);str(s);free(s);}
static void type(const ParsedType *t){
 fputs("{\"name\":",out);str(t?t->userTypeName:NULL);fprintf(out,",\"kind\":%d,\"primitive\":",t?(int)t->kind:-1);str(t?getTokenTypeName(t->primitiveType):NULL);
 fprintf(out,",\"narrow\":%s,\"unsigned\":%s,\"pointer\":%d,\"const\":%s,\"volatile\":%s,\"static\":%s,\"extern\":%s,\"dims\":[",t&&t->isShort?"true":"false",t&&t->isUnsigned?"true":"false",t?t->pointerDepth:0,t&&t->isConst?"true":"false",t&&t->isVolatile?"true":"false",t&&t->isStatic?"true":"false",t&&t->isExtern?"true":"false");
 if(t)for(size_t i=0;i<t->derivationCount;i++){if(i)fputc(',',out);if(t->derivations[i].kind==TYPE_DERIVATION_ARRAY && t->derivations[i].as.array.hasConstantSize && !t->derivations[i].as.array.isVLA)fprintf(out,"%lld",(long long)t->derivations[i].as.array.constantSize);else fputs("null",out);}
 fprintf(out,"],\"long_double\":%s}",t&&t->primitiveType==TOKEN_DOUBLE&&t->isLong?"true":"false");
}
static const FisicsUnitsAnnotation *decl_annotation(const ASTNode *n,size_t slot){
 for(size_t i=0;i<fisics_extension_units_annotation_count(ctx);i++){
  const FisicsUnitsAnnotation *a=fisics_extension_units_annotation_at(ctx,i);
  if(a&&a->node==n&&a->declaratorIndex==slot)return a;
 }
 return NULL;
}
static void annotation(const FisicsUnitsAnnotation *a){
 if(!a||!a->resolved){fputs("null",out);return;}
 fprintf(out,"[%d,%d,%d]",a->dim.e[1],a->dim.e[0],a->dim.e[2]);
 for(int i=3;i<8;i++)if(a->dim.e[i])failed=1;
}
static void node(ASTNode *n,unsigned depth);
static void item(const char *name,ASTNode *n,unsigned d){fputc(',',out);str(name);fputc(':',out);node(n,d+1);}
static void list(ASTNode **nodes,size_t count,unsigned d){fputc('[',out);for(size_t i=0;i<count;i++){if(i)fputc(',',out);node(nodes[i],d+1);}fputc(']',out);}
static void node(ASTNode *n,unsigned d){
 if(!n){fputs("null",out);return;}if(d>256){failed=1;fputs("null",out);return;}
 fputs("{\"id\":",out);key(n);fputs(",\"origin\":",out);key(fisics_units_lookup_origin(ctx,n));fputs(",\"file\":",out);str(n->location.start.file);fprintf(out,",\"line\":%d,\"column\":%d,\"dtype\":",n->location.start.line,n->location.start.column);type(fisics_extension_lookup_units_symbol_type_binding(ctx,n));const FisicsUnitsExprResult *u=fisics_extension_lookup_units_expr_result(ctx,n);fprintf(out,",\"concrete_unit\":%s",u&&u->unitResolved?"true":"false");fputs(",\"tag\":",out);
#define TAG(s) str(s)
 switch(n->type){
 case AST_BLOCK:case AST_PROGRAM:TAG("block");fputs(",\"body\":",out);list(n->block.statements,n->block.statementCount,d);break;
 case AST_FUNCTION_DEFINITION:TAG("function");fputs(",\"return_type\":",out);type(&n->functionDef.returnType);fputs(",\"return_dim\":",out);annotation(decl_annotation(n,0));fputs(",\"name\":",out);str(n->functionDef.funcName->valueNode.value);fputs(",\"parameters\":",out);list(n->functionDef.parameters,n->functionDef.paramCount,d);item("body",n->functionDef.body,d);break;
 case AST_VARIABLE_DECLARATION:TAG("decl");fputs(",\"variables\":[",out);for(size_t i=0;i<n->varDecl.varCount;i++){if(i)fputc(',',out);ASTNode *v=n->varDecl.varNames[i];fputs("{\"key\":",out);key(v);fputs(",\"name\":",out);str(v?v->valueNode.value:NULL);fputs(",\"dtype\":",out);type(astVarDeclTypeAt(n,i));fputs(",\"dimension\":",out);annotation(decl_annotation(n,i));fputs(",\"concrete_unit\":",out);const FisicsUnitsAnnotation *an=decl_annotation(n,i);fputs(an&&an->unitResolved?"true":"false",out);fputs(",\"init\":",out);node(n->varDecl.initializers&&n->varDecl.initializers[i]?n->varDecl.initializers[i]->expression:NULL,d+1);fputc('}',out);}fputc(']',out);break;
 case AST_IDENTIFIER:TAG("id");fprintf(out,",\"direct_function\":%s",fisics_extension_lookup_units_function_binding(ctx,n)?"true":"false");fputs(",\"name\":",out);str(n->valueNode.value);break;
 case AST_NUMBER_LITERAL:TAG("number");fputs(",\"value\":",out);str(n->valueNode.value);break;
 case AST_CHAR_LITERAL:case AST_STRING_LITERAL:TAG("literal");fputs(",\"value\":",out);str(n->valueNode.value);break;
 case AST_ARRAY_ACCESS:TAG("index");item("base",n->arrayAccess.array,d);item("index",n->arrayAccess.index,d);break;
 case AST_POINTER_ACCESS:case AST_DOT_ACCESS:TAG("field");item("base",n->memberAccess.base,d);fputs(",\"field\":",out);str(n->memberAccess.field);break;
 case AST_ASSIGNMENT:TAG("assign");fputs(",\"op\":",out);str(n->assignment.op);item("target",n->assignment.target,d);item("value",n->assignment.value,d);break;
 case AST_BINARY_EXPRESSION:case AST_UNARY_EXPRESSION:case AST_SEQUENCE:TAG(n->type==AST_BINARY_EXPRESSION?"binary":n->type==AST_SEQUENCE?"sequence":"unary");fputs(",\"op\":",out);str(n->expr.op);fprintf(out,",\"postfix\":%s",n->expr.isPostfix?"true":"false");item("left",n->expr.left,d);item("right",n->expr.right,d);break;
 case AST_TERNARY_EXPRESSION:TAG("ternary");item("condition",n->ternaryExpr.condition,d);item("yes",n->ternaryExpr.trueExpr,d);item("no",n->ternaryExpr.falseExpr,d);break;
 case AST_CAST_EXPRESSION:TAG("cast");fputs(",\"cast_type\":",out);type(&n->castExpr.castType);item("value",n->castExpr.expression,d);break;
 case AST_IF_STATEMENT:TAG("if");item("condition",n->ifStmt.condition,d);item("yes",n->ifStmt.thenBranch,d);item("no",n->ifStmt.elseBranch,d);break;
 case AST_FOR_LOOP:TAG("for");item("init",n->forLoop.initializer,d);item("condition",n->forLoop.condition,d);item("increment",n->forLoop.increment,d);item("body",n->forLoop.body,d);break;
 case AST_WHILE_LOOP:TAG("while");item("condition",n->whileLoop.condition,d);item("body",n->whileLoop.body,d);break;
 case AST_RETURN:TAG("return");item("value",n->returnStmt.returnValue,d);break;
 case AST_FUNCTION_CALL:TAG("call");item("callee",n->functionCall.callee,d);fputs(",\"args\":",out);list(n->functionCall.arguments,n->functionCall.argumentCount,d);break;
 case AST_COMPOUND_LITERAL:TAG("aggregate");fputs(",\"entries\":[",out);for(size_t i=0;i<n->compoundLiteral.entryCount;i++){if(i)fputc(',',out);DesignatedInit *e=n->compoundLiteral.entries[i];fputs("{\"field\":",out);str(e?e->fieldName:NULL);item("index",e?e->indexExpr:NULL,d);item("value",e?e->expression:NULL,d);fputc('}',out);}fputc(']',out);break;
 case AST_BASIC_TYPE:case AST_PARSED_TYPE:TAG("type");fputs(",\"parsed\":",out);type(&n->parsedTypeNode.parsed);for(size_t i=0;i<n->parsedTypeNode.parsed.derivationCount;i++)if(n->parsedTypeNode.parsed.derivations[i].kind==TYPE_DERIVATION_ARRAY && n->parsedTypeNode.parsed.derivations[i].as.array.isVLA)failed=1;break;
 case AST_SIZEOF:TAG("sizeof");item("value",n->expr.left,d);break;
 case AST_GOTO_STATEMENT:TAG("goto");fputs(",\"label\":",out);str(n->gotoStmt.label);break;
 case AST_LABEL_DECLARATION:TAG("label");item("body",n->label.statement,d);break;
 case AST_BREAK:TAG("break");break;case AST_CONTINUE:TAG("continue");break;
 default:TAG("unsupported");fprintf(out,",\"enum\":%d",(int)n->type);break;
 }
 fputc('}',out);
#undef TAG
}
int main(int argc,char **argv){
 if(sizeof(size_t)<4 || sizeof(double)!=8 || DBL_MANT_DIG!=53 || DBL_MAX_EXP!=1024)return 2;
 if(argc<4){fprintf(stderr,"usage: ast-export SOURCE OUTPUT INCLUDE...\n");return 2;}
 unit=argv[1];char *deps=malloc(strlen(argv[2])+16);if(!deps)return 2;sprintf(deps,"%s.deps.json",argv[2]);
 CompileOptions options={.depsJsonPath=deps,.inputPath=argv[1],.includePaths=(const char*const*)(argv+3),.includePathCount=(size_t)argc-3,.dialect=CC_DIALECT_C11,.preprocessMode=PREPROCESS_INTERNAL,.overlayFeatures=FISICS_OVERLAY_PHYSICS_UNITS,.enableCodegen=false,.captureSourceContract=true};
 CompileResult r={0};int rc=compile_translation_unit(&options,&r);if(rc || r.semanticErrors || !r.ast){compile_result_destroy(&r);return 1;}
 out=fopen(argv[2],"wx");if(!out){compile_result_destroy(&r);return 2;}ctx=r.compilerCtx;fputs("{\"schema\":\"fisiCs.internal.resolved_ast.v1\",\"functions\":[",out);size_t count=0;const char *owned=getenv("FISICS_AST_SOURCE_ROOT");size_t owned_len=owned?strlen(owned):0;
 for(size_t i=0;i<r.ast->block.statementCount;i++){ASTNode *n=r.ast->block.statements[i];if(n && n->type==AST_FUNCTION_DEFINITION){const char *file=n->location.start.file;if(owned && (!file || ((strncmp(file,owned,owned_len)!=0 || file[owned_len]!='/') && strcmp(file,argv[1])!=0)))continue;if(count++)fputc(',',out);node(n,0);}}
 fputs("]}\n",out);if(fclose(out))failed=1;compile_result_destroy(&r);free(deps);return failed?1:0;
}
