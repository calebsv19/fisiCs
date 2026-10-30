#include <stdio.h>
#include <string.h>
#include "fisics_frontend.h"
#define D(x) " [[fisics::dim(" x ")]] "
#define U(x) " [[fisics::unit(" x ")]] "
#define TIME "double dt" D("time") U("second") "=2;"
#define BODY "typedef struct {double a[2]" D("acceleration") ";double x" D("length") ";double grid[2][2]" D("length") ";} Body;"
#define UNIT_BODY "typedef struct{double x[2]" D("length") U("meter") ";} UnitBody;"
typedef struct {const char *name,*definitions,*body;int code;} Case;
static const Case cases[]={
    {"dot_write",BODY,"Body b;" TIME "b.a[0]=dt;",4103},
    {"arrow_write",BODY,"Body b;Body *p=&b;" TIME "p->a[0]=dt;",4103},
    {"deref_dot",BODY,"Body b;Body *p=&b;" TIME "(*p).a[0]=dt;",4103},
    {"nested_dot",BODY "struct Outer {Body child;};","struct Outer o;" TIME "o.child.a[0]=dt;",4103},
    {"nested_pointer",BODY "struct Outer {Body *child;};","struct Outer *o;" TIME "o->child->a[0]=dt;",4103},
    {"record_array",BODY,"Body b[2];" TIME "b[1].a[0]=dt;",4103},
    {"pointer_subscript",BODY,"Body *p;" TIME "p[1].a[0]=dt;",4103},
    {"multi_index",BODY,"Body b;" TIME "b.grid[1][0]=dt;",4103},
    {"dot_scalar",BODY,"Body b;" TIME "b.x=dt;",4103},
    {"arrow_scalar",BODY,"Body *b;" TIME "b->x=dt;",4103},
    {"array_read",BODY,"Body b;double q" D("time") "=b.a[0];",4103},
    {"scalar_read",BODY,"Body b;double q" D("time") "=b.x;",4103},
    {"grid_read",BODY,"Body b;double q" D("time") "=b.grid[1][1];",4103},
    {"read_argument",BODY "void take(double t" D("time") ");","Body b;take(b.a[0]);",4112},
    {"read_return",BODY "double" D("time") " f(Body *b){return b->a[0];}","",4113},
    {"field_add",BODY,"Body b;" TIME "(void)(b.a[0]+dt);",4101},
    {"compound_add",BODY,"Body b;" TIME "b.a[0]+=dt;",4101},
    {"compound_sub",BODY,"Body b;" TIME "b.a[0]-=dt;",4102},
    {"conditional_record",BODY,"Body *a;Body *b;" TIME "(1?a:b)->a[0]=dt;",4103},
    {"cast_record",BODY,"void *raw;" TIME "((Body*)raw)->a[0]=dt;",4103},
    {"returned_record",BODY "Body *get(void);",TIME "get()->a[0]=dt;",4103},
    {"local_tag","","struct Local{double a[2]" D("acceleration") ";};struct Local b;" TIME "b.a[0]=dt;",4103},
    {"unit_read",UNIT_BODY,"UnitBody b;double ft" D("length") U("foot") "=b.x[0];",4107},
    {"unit_write",UNIT_BODY,"UnitBody b;double ft" D("length") U("foot") "=3;b.x[0]=ft;",4107},
    {"unit_arrow",UNIT_BODY,"UnitBody *b;double ft" D("length") U("foot") "=3;b->x[0]=ft;",4107},
    {"direct_array","","double a[2]" D("length") ";" TIME "a[0]=dt;",4103},
    {"direct_array_read","","double a[2]" D("length") ";double q" D("time") "=a[0];",4103},
    {"typed_pointer","","double" D("length") " *p;" TIME "*p=dt;",4103},
    {"typed_pointer_read","","double" D("length") " *p;double q" D("time") "=*p;",4103},
    {"matching_write",BODY,"Body b;double acc" D("acceleration") "=3;b.a[0]=acc;",0},
    {"matching_read",BODY,"Body b;double acc" D("acceleration") "=b.a[0];",0},
    {"matching_update",BODY,"Body b;double acc" D("acceleration") "=3;b.a[0]+=acc;",0},
    {"explicit_unit",UNIT_BODY,"UnitBody b;double ft" D("length") U("foot") "=3;b.x[0]=fisics_convert_unit(ft,\"meter\");",0},
    {"different_records","struct L{double x" D("length") ";};struct T{double x" D("time") ";};","struct L l;struct T t;double x" D("length") "=l.x;double dt" D("time") "=t.x;",0},
    {"unannotated_same_name","struct L{double x" D("length") U("meter") ";};struct Raw{double x;};","struct Raw b;" TIME "b.x=dt;",0},
    {"unannotated_same_array",UNIT_BODY "struct Raw{double x[2];};","struct Raw *b;" TIME "b->x[0]=dt;",0},
    {"raw_array","","double a[2];" TIME "a[0]=dt;",0},
    {"unknown_rhs",BODY "double external(void);","Body b;b.a[0]=external();",0},
    {"shadow_record_tag","struct S{double x" D("length") U("meter") ";};","struct S{double x" D("time") ";};struct S b;" TIME "b.x=dt;",0},
    {"unannotated_pointer","","double *p;" TIME "*p=dt;",0},
    {"record_pointer_alias", BODY, "Body *p;Body *alias=p;" TIME "alias->a[0]=dt;",4103},
    {"pointer_to_record_pointer", BODY, "Body **p;" TIME "(*p)->a[0]=dt;",4103},
    {"address_element_deref", BODY, "Body b;" TIME "*(&b.a[0])=dt;",4103},
    {"raw_element_alias_unknown", BODY, "Body b;double *p=b.a;double q" D("time") "=*p;",0},
    {"record_parameter", BODY "void update(Body *b,double dt" D("time") "){b->a[0]=dt;}", "",4103},
    {"mixed_checksum", "struct S{double e" D("energy") ";double x" D("length") ";};", "struct S b;(void)(b.e+b.x);",4101},
};
int main(void) {
    int failures=0;
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        char source[8192];
        snprintf(source,sizeof(source),"%s int main(void){%s return 0;}\n",cases[i].definitions,cases[i].body);
        FisicsFrontendOptions opts={0};opts.overlay_features=FISICS_OVERLAY_PHYSICS_UNITS;
        FisicsAnalysisResult result={0};
        int ok=fisics_analyze_buffer("units_access.c",source,strlen(source),&opts,&result);
        size_t errors=0,matching=0;
        for(size_t j=0;j<result.diag_count;++j) {
            const FisicsDiagnostic *d=&result.diagnostics[j];
            if(d->severity_id!=FISICS_DIAG_SEVERITY_ERROR)continue;
            ++errors;
            if(d->code_id==cases[i].code && d->category_id==FISICS_DIAG_CATEGORY_EXTENSION && d->line>0 && d->column>0)++matching;
            else fprintf(stderr,"%s unexpected %d: %s\n",cases[i].name,d->code_id,d->message);
        }
        size_t expected=cases[i].code?1:0;
        if(!ok||errors!=expected||matching!=errors) {
            fprintf(stderr,"FAIL %s: analyzed=%d errors=%zu matching=%zu expected=%zu\n",cases[i].name,ok,errors,matching,expected);++failures;
        }else printf("PASS %s\n",cases[i].name);
        fisics_free_analysis_result(&result);
    }
    printf("Access contracts: %zu cases, %d failures\n",sizeof(cases)/sizeof(cases[0]),failures);
    return failures?1:0;
}
