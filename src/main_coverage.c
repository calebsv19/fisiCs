// SPDX-License-Identifier: Apache-2.0
#include "main_internal.h"
#include "Compiler/units_coverage_output.h"
#include "Compiler/pipeline_artifacts_semantic_metadata.h"
#include <stdlib.h>

int main_run_coverage_mode(const MainDriverConfig* config,const char* function,
    const char* jsonPath,const char* textPath,FisicsCoveragePolicy policy) {
    const char* input=config->inputCFiles->items[0];
    CompileOptions options=main_make_compile_options(config,input,NULL);
    options.enableCodegen=false;options.captureSourceContract=true;
    CompileResult result={0};int status=compile_translation_unit(&options,&result);
    FisicsCoverageDecision decision={0};FisicsCoverageReport report={0};char* source=NULL;bool ok=false;
    if(status || !result.ast || !result.semanticModel)goto done;
    if(!fisics_units_collect_coverage(result.compilerCtx,result.ast,function,&report)) {
        fprintf(stderr,"Error: coverage requires one source-defined function '%s' and a collectable AST\n",function);goto done;
    }
    decision=fisics_units_coverage_decide(&report,policy);
    source=pipeline_normalize_semantic_metadata_source_path(input);
    if(!source)goto done;
    if(jsonPath && !fisics_units_coverage_destination(jsonPath,textPath,source,&result))goto done;
    if(textPath && !fisics_units_coverage_destination(textPath,jsonPath,source,&result))goto done;
    if(jsonPath && !fisics_units_coverage_write(jsonPath,textPath,source,&result,&report,&options,&decision,true))goto done;
    if(textPath && !fisics_units_coverage_write(textPath,jsonPath,source,&result,&report,&options,&decision,false))goto done;
    ok=true;
 done:
    if(!ok)fprintf(stderr,"Error: units coverage report was not published completely\n");
    if(ok && policy.enabled)fprintf(stderr,"Units strict policy selected_function_v1: %s; required %zu; checked %zu; controls %zu; permitted trust %zu; blocked %zu; enumeration %s\n",
        decision.accepted && !result.semanticErrors?"accepted":"rejected",decision.required,decision.checked,decision.controls,decision.acceptedTrust,decision.blocked,report.enumerationComplete?"complete":"incomplete");
    int exitCode=ok && !result.semanticErrors && (!policy.enabled || decision.accepted)?0:1;
    free(source);fisics_units_coverage_destroy(&report);compile_result_destroy(&result);
    return exitCode;
}
