// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <string.h>
#include "fisics_frontend.h"
#define D(x) " [[fisics::dim(" x ")]] "
#define U(x) " [[fisics::unit(" x ")]] "
#define F "double force" D("force") "=120,moment" D("force*length") "=200;"
#define L "double meters" D("length") U("meter") "=2,feet" D("length") U("foot") "=3;"
#define MAIN(x) "int main(void){" x "return 0;}"
typedef struct { const char *name, *body; int code, count; } Case;
static const Case cases[] = {
 {"duplicate_second",MAIN("double a" D("length") "=1,b" D("time") D("time") "=2;"),4003,1},
 {"duplicate_unit_second",MAIN("double a" D("length") "=1,b" D("time") U("second") U("second") "=2;"),4005,1},
 {"unit_without_dim_neighbor",MAIN("double a" D("length") "=1,b" U("meter") "=2;"),4006,1},
 {"incompatible_unit_second",MAIN("double a" D("length") "=1,b" D("time") U("meter") "=2;"),4007,1},
 {"original",MAIN(F "double result" D("force*length") "=moment;"),0,0},
 {"reverse_order",MAIN("double m" D("force*length") "=200,f" D("force") "=120;double q" D("force") "=f;"),0,0},
 {"read_first",MAIN(F "double q" D("force") "=force;"),0,0},
 {"reject_second_read",MAIN(F "double q" D("force") "=moment;"),4103,1},
 {"reject_first_read",MAIN(F "double q" D("force*length") "=force;"),4103,1},
 {"dimensionless_factor_reject",MAIN("double f" D("force") "=2,m" D("force*length") "=f*3;"),4103,1},
 {"initializer_reject",MAIN("double f" D("force") "=2,m" D("force*length") "=f;"),4103,1},
 {"initializer_previous",MAIN("double f" D("force") "=2,g" D("force") "=f;"),0,0},
 {"three_quantities",MAIN("double f" D("force") "=2,m" D("force*length") "=3,t" D("time") "=4;double q" D("time") "=t;"),0,0},
 {"second_only",MAIN("double raw=2,t" D("time") "=3;double q" D("length") "=t;"),4103,1},
 {"unannotated_neighbor",MAIN("double t" D("time") "=2,raw=3;double q" D("length") "=raw;"),0,0},
 {"no_backflow",MAIN("double raw=2,t" D("time") "=3;double q" D("length") "=raw;"),0,0},
 {"unit_isolation",MAIN(L "double q" D("length") U("foot") "=feet;"),0,0},
 {"unit_reject",MAIN(L "double q" D("length") U("meter") "=feet;"),4107,1},
 {"unit_initializer",MAIN("double m" D("length") U("meter") "=2,f" D("length") U("foot") "=m;"),4107,1},
 {"unit_explicit",MAIN(L "double q" D("length") U("meter") "=fisics_convert_unit(feet,\"meter\");"),0,0},
 {"assignment_reject",MAIN(F "force=moment;"),4103,1},
 {"assignment_valid",MAIN(F "moment=moment;"),0,0},
 {"sum_reject",MAIN(F "(void)(force+moment);"),4101,1},
 {"array_valid",MAIN("double f[2]" D("force") "={1,2},m[2]" D("force*length") "={3,4};double q" D("force*length") "=m[1];"),0,0},
 {"array_reject",MAIN("double f[2]" D("force") "={1,2},m[2]" D("force*length") "={3,4};f[1]=m[0];"),4103,1},
 {"array_initializer_reject",MAIN(F "double f[2]" D("force") "={1,2},m[2]" D("force*length") "={0,force};"),4103,1},
 {"pointer_valid",MAIN("double *f" D("force") ",*m" D("force*length") ";double q" D("force*length") "=*m;"),0,0},
 {"pointer_reject",MAIN("double *f" D("force") ",*m" D("force*length") ";double q" D("force") "=*m;"),4103,1},
 {"field_valid","struct S{double f" D("force") ",m" D("force*length") ";};" MAIN("struct S s;double q" D("force*length") "=s.m;"),0,0},
 {"field_reject","struct S{double f" D("force") ",m" D("force*length") ";};" MAIN("struct S s;s.f=s.m;"),4103,1},
 {"global_valid",F MAIN("double q" D("force*length") "=moment;"),0,0},
 {"global_reject",F MAIN("force=moment;"),4103,1},
 {"for_valid",MAIN("for(double f" D("force") "=2,m" D("force*length") "=3;0;){double q" D("force*length") "=m;}"),0,0},
 {"for_reject",MAIN("for(double f" D("force") "=2,m" D("force*length") "=3;0;){f=m;}"),4103,1},
 {"typedef_valid","typedef double Real;" MAIN("Real f" D("force") "=2,m" D("force*length") "=3;Real q" D("force*length") "=m;"),0,0},
 {"shared_specifier",MAIN("double " D("length") "a=2,b=3;double q" D("time") "=b;"),4103,1},
 {"shared_specifier_valid",MAIN("double " D("length") "a=2,b=3;double q" D("length") "=b;"),0,0},
 {"shadowing",MAIN(F "{double f" D("time") "=1,moment" D("length") "=2;double q" D("length") "=moment;}double q" D("force*length") "=moment;"),0,0},
 {"call_second","void take(double x" D("force") ");" MAIN(F "take(moment);"),4112,1},
 {"return_second","double take(void)" D("force") "{" F "return moment;}",4113,1},
 {"same_unit_distinct","double a" D("length") U("meter") ",b" D("length") U("meter") ";",0,0},
 {"plain_c",MAIN("double a=2,b=3,c=a+b;int d=1,*p=&d;(void)c;(void)p;"),0,0},
};
int main(void) {
    int failures=0;
    for (size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        char source[4096];
        snprintf(source,sizeof(source),"%s\n",cases[i].body);
        FisicsFrontendOptions opts={0};
        opts.overlay_features=FISICS_OVERLAY_PHYSICS_UNITS;
        FisicsAnalysisResult res={0};
        int ok=fisics_analyze_buffer("units_declarators.c",source,strlen(source),&opts,&res);
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
    printf("Declarator contracts: %zu cases, %d failures\n",sizeof(cases)/sizeof(cases[0]),failures);
    return failures?1:0;
}
