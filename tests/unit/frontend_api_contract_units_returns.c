#include <stdio.h>
#include <string.h>
#include "fisics_frontend.h"
#define D(x) " [[fisics::dim(" x ")]] "
#define U(x) " [[fisics::unit(" x ")]] "
#define TIME "double dt" D("time") "=2;"
#define LENGTH "double x" D("length") "=3;"
typedef struct { const char *name,*definitions,*body; int code,count; } Case;
static const Case cases[]={
    {"definition_dimension","double" D("length") " f(double t" D("time") "){return t;}","",4113,1},
    {"definition_branch","double" D("length") " f(int b,double t" D("time") ",double x" D("length") "){if(b)return x; else return t;}","",4113,1},
    {"definition_two_returns","double" D("length") " f(int b,double t" D("time") "){if(b)return t; return t;}","",4113,2},
    {"definition_unit","double" D("length") U("meter") " f(double x" D("length") U("foot") "){return x;}","",4107,1},
    {"prototype_consumer","double" D("length") " f(void);",TIME "double result" D("time") "=f();",4103,1},
    {"prototype_assignment","double" D("length") " f(void);",TIME "dt=f();",4103,1},
    {"prototype_argument","double" D("length") " f(void); void take(double t" D("time") ");","take(f());",4112,1},
    {"prototype_unit","double" D("length") U("meter") " f(void);","double ft" D("length") U("foot") "=f();",4107,1},
    {"header_inherited_definition","double" D("length") " f(double t" D("time") "); double f(double t" D("time") "){return t;}","",4113,1},
    {"declaration_conflict","double" D("length") " f(void); double" D("time") " f(void);","",4114,1},
    {"unit_contract_conflict","double" D("length") U("meter") " f(void); double" D("length") U("foot") " f(void);","",4107,1},
    {"trailing_contract","double f(double t" D("time") ")" D("length") " {return t;}","",4113,1},
    {"matching_definition","double" D("length") " f(double x" D("length") "){return x;}",LENGTH "double result" D("length") "=f(x);",0,0},
    {"literal_returns","double" D("length") " f(int b){if(b)return -3;return (double)(1.0/60);}","",0,0},
    {"unknown_definition","double external(void); double" D("length") " f(void){return external();}","",0,0},
    {"unknown_external","double f(void);","double result" D("time") "=f();",0,0},
    {"shadow_pointer","double" D("length") " f(void); double plain(void);","double (*f)(void)=plain; double result" D("time") "=f();",0,0},
    {"block_prototype","double" D("length") " f(void);","double" D("time") " f(void); double result" D("time") "=f();",0,0},
    {"forward_definition","double" D("length") " f(void); double" D("length") " g(void){return f();} double" D("length") " f(void){return 3;}","",0,0},
    {"callback_return","typedef double" D("length") " Fn(void); void g(Fn *fn){double result" D("time") "=fn();}","",4103,1},
    {"shadow_global_definition", "double f(void){double x" D("length") "=3;return x;} double plain(void);", "double (*f)(void)=plain;double result" D("time") "=f();",0,0},
    {"block_unannotated_shadow", "double f(void){double x" D("length") "=3;return x;}", "double f(void);double result" D("time") "=f();",0,0},
    {"later_contract", "double f(void); double" D("length") " f(void);", "double result" D("time") "=f();",4103,1},
    {"contract_not_erased", "double" D("length") " f(void); double f(void);", "double result" D("time") "=f();",4103,1},
    {"converted_return", "double" D("length") U("meter") " f(double x" D("length") U("foot") "){return fisics_convert_unit(x,\"meter\");}", "",0,0},
    {"recursive_contract", "double" D("length") " f(double t" D("time") "){if(t)return f(t);return t;}", "",4113,1},
    {"duplicate_result_dim", "double" D("length") D("length") " f(void);", "",4003,1},
    {"invalid_result_dim", "double" D("nonsense") " f(void);", "",4002,1},
    {"legacy_body_inference", "double f(void){double x" D("length") "=3;return x;}", "double result" D("time") "=f();",4103,1},
};
int main(void) {
    int failures=0;
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        char source[8192];
        snprintf(source,sizeof(source),"%s int main(void){%s return 0;}\n",cases[i].definitions,cases[i].body);
        FisicsFrontendOptions opts={0}; opts.overlay_features=FISICS_OVERLAY_PHYSICS_UNITS;
        FisicsAnalysisResult res={0};
        int ok=fisics_analyze_buffer("units_returns.c",source,strlen(source),&opts,&res);
        size_t errors=0,matching=0,dimensions=0,units=0;
        for(size_t j=0;j<res.diag_count;++j) {
            const FisicsDiagnostic *d=&res.diagnostics[j];
            if(d->severity_id!=FISICS_DIAG_SEVERITY_ERROR)continue;
            ++errors;
            if(d->code_id==4113)++dimensions;
            if(d->code_id==4107)++units;
            if(d->category_id==FISICS_DIAG_CATEGORY_EXTENSION && d->line>0 && d->column>0 &&
               (d->code_id==cases[i].code || (cases[i].code==-1 && (d->code_id==4113||d->code_id==4107))))++matching;
            else fprintf(stderr,"%s unexpected %d: %s\n",cases[i].name,d->code_id,d->message);
        }
        if(!ok||errors!=(size_t)cases[i].count||matching!=errors||
           (cases[i].code==-1 && (dimensions!=1||units!=1))) {
            fprintf(stderr,"FAIL %s: analyzed=%d errors=%zu matching=%zu expected=%d\n",cases[i].name,ok,errors,matching,cases[i].count);++failures;
        } else printf("PASS %s\n",cases[i].name);
        fisics_free_analysis_result(&res);
    }
    printf("Return contracts: %zu cases, %d failures\n",sizeof(cases)/sizeof(cases[0]),failures);
    return failures?1:0;
}
