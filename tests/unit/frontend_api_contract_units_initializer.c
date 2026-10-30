#include <stdio.h>
#include <string.h>
#include "fisics_frontend.h"

#define D(x) " [[fisics::dim(" x ")]] "
#define U(x) " [[fisics::unit(" x ")]] "
#define TIME "double dt" D("time") U("second") "=2;"
#define LENGTH "double x" D("length") U("meter") "=3;"
#define FIELD "struct S { double value" D("length") "; };"
#define FIELD_UNIT "struct S { double value" D("length") U("meter") "; };"
#define ARRAY_FIELD "struct S { double value[2]" D("length") "; };"

typedef struct { const char *name, *body; int code, count; } Case;
static const Case cases[] = {
    {"scalar_dim", TIME "double value" D("length") "=dt;", 4103, 1},
    {"scalar_unit", TIME "double value" D("length") U("meter") "=dt;", 4103, 1},
    {"energy_factor", "double m" D("mass") "=2; double v" D("speed") "=3; double e" D("energy") "=0.5*m*v;", 4103, 1},
    {"dimensionless_result", TIME "double value" D("length") "=dt/dt;", 4103, 1},
    {"cast_preserves_source", TIME "double value" D("length") "=(double)dt;", 4103, 1},
    {"field_positional", FIELD TIME "struct S s={dt};", 4103, 1},
    {"field_designated_unit", FIELD_UNIT TIME "struct S s={.value=dt};", 4103, 1},
    {"field_compound", FIELD TIME "struct S s=(struct S){.value=dt};", 4103, 1},
    {"nested_field", FIELD "struct Outer { struct S s; };" TIME "struct Outer o={{dt}};", 4103, 1},
    {"union_field", "union S { double value" D("length") "; int unused; };" TIME "union S s={.value=dt};", 4103, 1},
    {"array_positional", TIME "double a[2]" D("length") "={0,dt};", 4103, 1},
    {"array_designated_unit", TIME "double a[2]" D("length") U("meter") "={[1]=dt};", 4103, 1},
    {"array_nested", TIME "double a[2][2]" D("length") "={{0,dt},{0,0}};", 4103, 1},
    {"array_field", ARRAY_FIELD TIME "struct S s={{0,dt}};", 4103, 1},
    {"multiple_entries", TIME "double a[2]" D("length") "={dt,dt};", 4103, 2},
    {"unit_mismatch", "double ft" D("length") U("foot") "=3; double m" D("length") U("meter") "=ft;", 4107, 1},
    {"constants", "double a" D("length") "=0; double b" D("time") "=2; double c" D("length") "=-3; double d" D("time") "=(double)4; double e" D("time") "=1.0/60; double f" D("length") "='A'; double g" D("length") "=(1?2:3);", 0, 0},
    {"constant_arrays", "double a[2][2]" D("length") U("meter") "={{0,-1},{(double)2,3.0/4}};", 0, 0},
    {"constant_fields", FIELD "struct S s={-2};", 0, 0},
    {"constant_array_field", ARRAY_FIELD "struct S s={{0,-2}};", 0, 0},
    {"same_dimension", LENGTH "double value" D("length") "=x;", 0, 0},
    {"same_dimension_array", LENGTH "double a[2]" D("length") "={x,x};", 0, 0},
    {"same_dimension_field", FIELD LENGTH "struct S s={x};", 0, 0},
    {"explicit_conversion", "double ft" D("length") U("foot") "=3; double m" D("length") U("meter") "=fisics_convert_unit(ft,\"meter\");", 0, 0},
    {"unknown_stays_unknown", "extern double external(void); double value" D("length") "=external();", 0, 0},
};

int main(void) {
    int failures=0;
    for (size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        char source[4096];
        snprintf(source,sizeof(source),"int main(void){%s return 0;}\n",cases[i].body);
        FisicsFrontendOptions opts={0};
        opts.overlay_features=FISICS_OVERLAY_PHYSICS_UNITS;
        FisicsAnalysisResult res={0};
        int ok=fisics_analyze_buffer("units_initializer.c",source,strlen(source),&opts,&res);
        size_t errors=0,matching=0;
        for (size_t j=0;j<res.diag_count;++j) {
            const FisicsDiagnostic *d=&res.diagnostics[j];
            if (d->severity_id!=FISICS_DIAG_SEVERITY_ERROR) continue;
            ++errors;
            if (d->category_id==FISICS_DIAG_CATEGORY_EXTENSION &&
                d->code_id==cases[i].code && d->line>0 && d->column>0) ++matching;
            else fprintf(stderr,"%s: unexpected diagnostic %d: %s\n",cases[i].name,d->code_id,d->message);
        }
        if (!ok || errors!=(size_t)cases[i].count || matching!=errors) {
            fprintf(stderr,"FAIL %s: analyzed=%d errors=%zu matching=%zu expected=%d\n",
                    cases[i].name,ok,errors,matching,cases[i].count);
            ++failures;
        } else printf("PASS %s\n",cases[i].name);
        fisics_free_analysis_result(&res);
    }
    printf("Initializer contracts: %zu cases, %d failures\n",sizeof(cases)/sizeof(cases[0]),failures);
    return failures?1:0;
}
