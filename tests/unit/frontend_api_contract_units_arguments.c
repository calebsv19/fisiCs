#include <stdio.h>
#include <string.h>
#include "fisics_frontend.h"

#define D(x) " [[fisics::dim(" x ")]] "
#define U(x) " [[fisics::unit(" x ")]] "
#define TIME "double dt" D("time") U("second") "=2;"
#define LENGTH "double x" D("length") U("meter") "=3;"
#define DIRECT "double take(double value" D("length") "){return value;}"
#define PROTO "double take(double value" D("length") ");"
#define UNIT_FN "double take(double value" D("length") U("meter") "){return value;}"
#define PTR "typedef double Fn(double value" D("length") ");" DIRECT
#define UNIT_PTR "typedef double Fn(double value" D("length") U("meter") ");" UNIT_FN

typedef struct { const char *name, *definitions, *body; int code, count; } Case;
static const Case cases[] = {
    {"direct_dim", DIRECT, TIME "(void)take(dt);", 4112, 1},
    {"direct_unit", UNIT_FN, TIME "(void)take(dt);", 4112, 1},
    {"prototype_only", PROTO, TIME "(void)take(dt);", 4112, 1},
    {"function_pointer", PTR, TIME "Fn *fn=take; (void)fn(dt);", 4112, 1},
    {"dereferenced_pointer", PTR, TIME "Fn *fn=take; (void)(*fn)(dt);", 4112, 1},
    {"cast_pointer", PTR, TIME "Fn *fn=take; (void)((Fn*)fn)(dt);", 4112, 1},
    {"conditional_pointer", PTR, TIME "Fn *fn=take; (void)(1?fn:fn)(dt);", 4112, 1},
    {"member_pointer", PTR "struct S { Fn *fn; };", TIME "struct S s={take}; (void)s.fn(dt);", 4112, 1},
    {"array_pointer", PTR, TIME "Fn *slots[1]={take}; (void)slots[0](dt);", 4112, 1},
    {"returned_pointer", PTR "Fn *get(void){return take;}", TIME "(void)get()(dt);", 4112, 1},
    {"callback_parameter", PTR "double relay(Fn *fn,double value" D("time") "){return fn(value);}", "", 4112, 1},
    {"two_bad_arguments", "void take(double x" D("length") ",double m" D("mass") ");", TIME "take(dt,dt);", 4112, 2},
    {"second_argument", "void take(double x" D("length") ",double m" D("mass") ");", TIME LENGTH "take(x,dt);", 4112, 1},
    {"dimensionless_variable", DIRECT, "double ratio" D("scalar") "=1; (void)take(ratio);", 4112, 1},
    {"physical_ratio", DIRECT, TIME "(void)take(dt/dt);", 4112, 1},
    {"cast_physical", DIRECT, TIME "(void)take((double)dt);", 4112, 1},
    {"arithmetic_physical", DIRECT, TIME "(void)take(dt*dt);", 4112, 1},
    {"variadic_fixed", "void take(double x" D("length") ",...);", TIME "take(dt,dt);", 4112, 1},
    {"concrete_unit_mismatch", UNIT_FN, "double ft" D("length") U("foot") "=3; (void)take(ft);", 4107, 1},
    {"pointer_unit_mismatch", UNIT_PTR, "double ft" D("length") U("foot") "=3; Fn *fn=take; (void)fn(ft);", 4107, 1},
    {"dimension_and_unit_independent", "void take(double x" D("length") U("meter") ",double y" D("length") U("meter") ");", TIME "double ft" D("length") U("foot") "=3; take(dt,ft);", -1, 2},
    {"matching_direct", UNIT_FN, LENGTH "(void)take(x);", 0, 0},
    {"matching_pointer", PTR, LENGTH "Fn *fn=take; (void)fn(x);", 0, 0},
    {"dimension_only_accepts_unit", DIRECT, "double ft" D("length") U("foot") "=3; (void)take(ft);", 0, 0},
    {"numeric_literals", UNIT_FN, "(void)take(3); (void)take(-3); (void)take((double)3); (void)take(1.0/60); (void)take(1?2:3); (void)take(0);", 0, 0},
    {"explicit_conversion", UNIT_FN, "double ft" D("length") U("foot") "=3; (void)take(fisics_convert_unit(ft,\"meter\"));", 0, 0},
    {"variadic_extra", "void take(double x" D("length") ",...);", TIME LENGTH "take(x,dt);", 0, 0},
    {"unannotated_parameter", "void take(double value);", TIME "take(dt);", 0, 0},
    {"shadowed_unannotated_pointer", DIRECT "double plain(double value){return value;}", TIME "double (*take)(double)=plain; (void)take(dt);", 0, 0},
    {"block_scope_prototype", DIRECT, TIME "double take(double value" D("time") "); (void)take(dt);", 0, 0},
    {"unknown_argument", DIRECT "double external(void);", "(void)take(external());", 0, 0},
};

int main(void) {
    int failures=0;
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        char source[8192];
        snprintf(source,sizeof(source),"%s int main(void){%s return 0;}\n",cases[i].definitions,cases[i].body);
        FisicsFrontendOptions opts={0}; opts.overlay_features=FISICS_OVERLAY_PHYSICS_UNITS;
        FisicsAnalysisResult res={0};
        int ok=fisics_analyze_buffer("units_arguments.c",source,strlen(source),&opts,&res);
        size_t errors=0,matching=0,dimensions=0,units=0;
        for(size_t j=0;j<res.diag_count;++j) {
            const FisicsDiagnostic *d=&res.diagnostics[j];
            if(d->severity_id!=FISICS_DIAG_SEVERITY_ERROR)continue;
            ++errors;
            if(d->code_id==4112)++dimensions;
            if(d->code_id==4107)++units;
            if(d->category_id==FISICS_DIAG_CATEGORY_EXTENSION && d->line>0 && d->column>0 &&
               (d->code_id==cases[i].code || (cases[i].code==-1 && (d->code_id==4112||d->code_id==4107))))++matching;
            else fprintf(stderr,"%s unexpected %d: %s\n",cases[i].name,d->code_id,d->message);
        }
        if(!ok||errors!=(size_t)cases[i].count||matching!=errors||
           (cases[i].code==-1 && (dimensions!=1||units!=1))) {
            fprintf(stderr,"FAIL %s: analyzed=%d errors=%zu matching=%zu expected=%d\n",cases[i].name,ok,errors,matching,cases[i].count);++failures;
        } else printf("PASS %s\n",cases[i].name);
        fisics_free_analysis_result(&res);
    }
    printf("Argument contracts: %zu cases, %d failures\n",sizeof(cases)/sizeof(cases[0]),failures);
    return failures?1:0;
}
