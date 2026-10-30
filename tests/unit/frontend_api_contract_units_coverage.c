// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <string.h>
#include "Compiler/pipeline.h"
#include "AST/ast_node.h"
#include "Extensions/Units/units_coverage_model.h"
#include "Extensions/extension_hooks.h"

static int failures, checks;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); } } while (0)
static ASTNode* declaration(ASTNode* node, const char* name, size_t* slot) {
    if (!node) return NULL;
    if (node->type == AST_VARIABLE_DECLARATION) {
        for (size_t i=0;i<node->varDecl.varCount;++i) {
            ASTNode* n=node->varDecl.varNames[i];
            if (n && n->valueNode.value && !strcmp(n->valueNode.value,name)) {
                *slot=i; return node;
            }
        }
    }
    if (node->type == AST_FUNCTION_DEFINITION)
        return declaration(node->functionDef.body,name,slot);
    if (node->type == AST_PROGRAM || node->type == AST_BLOCK)
        for (size_t i=0;i<node->block.statementCount;++i) {
            ASTNode* found=declaration(node->block.statements[i],name,slot);
            if (found) return found;
        }
    return NULL;
}
static FisicsCoverageFact named(CompileResult* result,const char* name,bool initializer) {
    size_t index=0; ASTNode* decl=declaration(result->ast,name,&index);
    CHECK(decl!=NULL);
    if (!decl) return (FisicsCoverageFact){0};
    if (!initializer) return fisics_coverage_declaration(result->compilerCtx,decl,index);
    DesignatedInit* init=decl->varDecl.initializers[index]; CHECK(init && init->expression);
    return fisics_coverage_expression(result->compilerCtx,init?init->expression:NULL);
}
static FisicsCoverageAssessment compare(FisicsCoverageFact expected,FisicsCoverageFact actual,bool units) {
    FisicsCoverageObligation obligation={0};
    obligation.site=actual.site; obligation.boundary=FISICS_COVERAGE_INITIALIZER;
    obligation.expected=expected; obligation.actual=actual; obligation.requireConcreteUnit=units;
    return fisics_coverage_assess(obligation);
}
static void compile_expected(const char* path, const char* macro, CompileResult* result,size_t errors) {
    CompileOptions options={0}; options.inputPath=path;
    options.overlayFeatures=FISICS_OVERLAY_PHYSICS_UNITS;
    const char* includes[]={"include"};options.includePaths=includes;options.includePathCount=1;
    if (macro) { options.macroDefines=&macro;options.macroDefineCount=1; }
    CHECK(compile_translation_unit(&options,result)==0);
    CHECK(result->semanticErrors==errors);
}
static void compile_case(const char* path,const char* macro,CompileResult* result) {
    compile_expected(path,macro,result,0);
}
int main(void) {
    CompileResult result={0};
    compile_case("tests/fixtures/solver_physics/coverage_contract.c",NULL,&result);
    if (!result.ast || !result.compilerCtx) return 1;
    FisicsCoverageFact force=named(&result,"force",false),moment=named(&result,"moment",false);
    CHECK(force.hasDimension && moment.hasDimension);
    CHECK(force.origin==FISICS_COVERAGE_DECLARED && force.provenanceComplete);
    CHECK(!fisics_dim_equal(force.dimension,moment.dimension));
    CHECK(force.originNode!=moment.originNode && force.originLocation.start.column!=moment.originLocation.start.column);
    FisicsCoverageFact read=named(&result,"read_force",true);
    CHECK(read.originNode==force.originNode && read.origin==FISICS_COVERAGE_DECLARED);
    FisicsCoverageAssessment good=compare(force,read,false);
    CHECK(good.state==FISICS_COVERAGE_CHECKED && good.dimensionCompared && !good.unitCompared);
    FisicsCoverageAssessment bad=compare(force,named(&result,"read_moment",true),false);
    CHECK(bad.state==FISICS_COVERAGE_CONTRADICTORY && bad.reason==FISICS_COVERAGE_DIMENSION_MISMATCH);
    FisicsCoverageFact raw=named(&result,"raw",false),zero=named(&result,"scalar",false);
    CHECK(!raw.hasDimension && zero.hasDimension && fisics_dim_equal(zero.dimension,fisics_dim_zero()));
    FisicsCoverageAssessment missing=compare(force,named(&result,"read_raw",true),false);
    CHECK(missing.state==FISICS_COVERAGE_UNCHECKED && missing.lossNode==missing.obligation.actual.site);
    CHECK(missing.reason==FISICS_COVERAGE_MISSING_ANNOTATION);
    FisicsCoverageFact inferred=named(&result,"sum",true);
    CHECK(inferred.hasDimension && inferred.origin==FISICS_COVERAGE_INFERRED && !inferred.provenanceComplete);
    FisicsCoverageAssessment partial=compare(force,inferred,false);
    CHECK(partial.state==FISICS_COVERAGE_UNCHECKED && partial.dimensionCompared);
    CHECK(partial.reason==FISICS_COVERAGE_UNTRACKED_PROVENANCE);
    FisicsCoverageFact meters=named(&result,"meters",false),feet=named(&result,"read_feet",true);
    FisicsCoverageAssessment unit=compare(meters,feet,true);
    CHECK(unit.state==FISICS_COVERAGE_CONTRADICTORY && unit.reason==FISICS_COVERAGE_UNIT_MISMATCH);
    FisicsCoverageAssessment noUnit=compare(meters,named(&result,"distance",false),true);
    CHECK(noUnit.state==FISICS_COVERAGE_UNCHECKED && noUnit.reason==FISICS_COVERAGE_MISSING_UNIT);
    CHECK(compare(meters,named(&result,"distance",false),false).state==FISICS_COVERAGE_CHECKED);
    FisicsCoverageFact call=named(&result,"external_value",true);
    CHECK(call.hasDimension && call.trusted && call.site && call.location.start.line>0);
    FisicsCoverageAssessment trusted=compare(force,call,false);
    CHECK(trusted.state==FISICS_COVERAGE_TRUSTED_BOUNDARY);
    CHECK(compare(moment,call,false).state==FISICS_COVERAGE_CONTRADICTORY);
    FisicsCoverageFact unknownCall=named(&result,"unknown_value",true);
    CHECK(!unknownCall.hasDimension && unknownCall.reason==FISICS_COVERAGE_UNCONTRACTED_CALL);
    FisicsCoverageFact field=named(&result,"raw_field",true),slot=named(&result,"raw_slot",true);
    CHECK(!field.hasDimension && field.reason==FISICS_COVERAGE_UNANNOTATED_STORAGE);
    CHECK(!slot.hasDimension && slot.reason==FISICS_COVERAGE_UNANNOTATED_STORAGE);
    FisicsCoverageFact typed=named(&result,"typed_field",true);
    CHECK(typed.hasDimension && !typed.provenanceComplete && typed.originNode==NULL);
    CHECK(typed.reason==FISICS_COVERAGE_UNTRACKED_PROVENANCE);
    FisicsCoverageAssessment rows[]={good,bad,missing,trusted,partial,unit,noUnit};
    FisicsCoverageSummary summary={0};
    CHECK(fisics_coverage_summarize(rows,7,&summary));
    CHECK(summary.total==7 && summary.checked==1 && summary.trusted==1 && summary.unchecked==3 && summary.contradictory==2);
    rows[0].state=FISICS_COVERAGE_CONTRADICTORY; /* Verdict forgery cannot alter counts. */
    CHECK(fisics_coverage_summarize(rows,7,&summary) && summary.checked==1);
    FisicsCoverageAssessment duplicates[]={good,good};
    CHECK(!fisics_coverage_summarize(duplicates,2,&summary) && summary.total==0);
    duplicates[1].obligation.slot=1;
    CHECK(fisics_coverage_summarize(duplicates,2,&summary) && summary.total==2);
    duplicates[1].obligation.site=NULL;
    CHECK(!fisics_coverage_summarize(duplicates,2,&summary) && summary.total==0);
    CHECK(fisics_coverage_summarize(NULL,0,&summary) && summary.total==0);
    CHECK(!fisics_coverage_summarize(NULL,1,&summary));
    CHECK(!fisics_coverage_summarize(rows,7,NULL));
    CHECK(!fisics_coverage_expression(NULL,read.site).hasDimension);
    CHECK(fisics_coverage_expression(NULL,read.site).reason==FISICS_COVERAGE_OVERLAY_DISABLED);
    FisicsCoverageFact assumed=read;
    assumed.trusted=true; assumed.reason=FISICS_COVERAGE_ASSUMED_REPRESENTATION;
    FisicsCoverageAssessment bridge=compare(force,assumed,false);
    CHECK(bridge.state==FISICS_COVERAGE_TRUSTED_BOUNDARY && bridge.reason==FISICS_COVERAGE_ASSUMED_REPRESENTATION);
    CHECK(compare(moment,assumed,false).state==FISICS_COVERAGE_CONTRADICTORY);
    compile_result_destroy(&result);
    compile_expected("tests/fixtures/solver_physics/coverage_contract.c","BAD_COVERAGE_ANNOTATION=1",&result,1);
    FisicsCoverageFact malformed=named(&result,"malformed",false);
    CHECK(malformed.hasDimension && !malformed.provenanceComplete);
    CHECK(malformed.reason==FISICS_COVERAGE_UNRESOLVED_ANNOTATION);
    FisicsCoverageFact malformedRead=named(&result,"observed_malformed",true);
    CHECK(!malformedRead.provenanceComplete && malformedRead.reason==FISICS_COVERAGE_UNRESOLVED_ANNOTATION);
    CHECK(compare(malformed,malformedRead,false).state==FISICS_COVERAGE_UNCHECKED);
    compile_result_destroy(&result);
    /* Source-derived qualification controls remain explicit gaps. */
    compile_case("tests/fixtures/solver_physics/translation.c",NULL,&result);
    FisicsCoverageFact acceleration=named(&result,"acceleration",true);
    CHECK(!acceleration.hasDimension && acceleration.reason==FISICS_COVERAGE_UNANNOTATED_STORAGE);
    compile_result_destroy(&result);
    compile_case("tests/fixtures/solver_physics/translation.c","TYPED_STATE=1",&result);
    acceleration=named(&result,"acceleration",true);
    CHECK(acceleration.hasDimension && !acceleration.provenanceComplete);
    compile_result_destroy(&result);
    compile_case("tests/fixtures/solver_physics/structure.c",NULL,&result);
    FisicsCoverageFact matrixForce=named(&result,"force",true);
    CHECK(!matrixForce.hasDimension); /* annotation on destination is not source proof */
    compile_result_destroy(&result);
    printf("Coverage contract: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
