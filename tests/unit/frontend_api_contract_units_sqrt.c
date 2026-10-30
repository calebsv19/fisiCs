#include <stdio.h>
#include <string.h>
#include "fisics_frontend.h"
#define D(x) " [[fisics::dim(" x ")]] "
#define U(x) " [[fisics::unit(" x ")]] "
#define MATH "#include <math.h>\n"
#define TGMATH "#include <tgmath.h>\n"
#define SPEED "double v" D("speed") "=3;"
typedef struct {const char *name,*headers,*definitions,*body;int code;} Case;
static const Case cases[]={
    {"wrong_time",MATH,"",SPEED "double t" D("time") "=sqrt(v*v);",4103},
    {"matching_speed",MATH,"",SPEED "double y" D("speed") "=sqrt(v*v);",0},
    {"acceleration_norm",MATH,"","double a" D("acceleration") "=3;double y" D("acceleration") "=sqrt(a*a);",0},
    {"negative_exponents",MATH,"","double q" D("s^-2") "=4;double y" D("s^-1") "=sqrt(q);",0},
    {"odd_positive",MATH,"","double x" D("length") "=4;(void)sqrt(x);",4115},
    {"odd_negative",MATH,"","double x" D("s^-1") "=4;(void)sqrt(x);",4115},
    {"odd_nonlength_axis",MATH,"","double x" D("kg") "=4;(void)sqrt(x);",4115},
    {"all_axes",MATH,"","double x" D("m^2*kg^2*s^-2*A^2*K^2*mol^2*cd^2*X^2") "=4;double y" D("m*kg*s^-1*A*K*mol*cd*X") "=sqrt(x);",0},
    {"exponent_min",MATH,"","double x" D("m^-128") "=4;double y" D("m^-64") "=sqrt(x);",0},
    {"odd_max",MATH,"","double x" D("m^127") "=4;(void)sqrt(x);",4115},
    {"dimensionless",MATH,"","double x" D("scalar") "=4;double y" D("scalar") "=sqrt(x);",0},
    {"literal_not_contextual",MATH,"","double y" D("length") "=sqrt(4.0);",4103},
    {"scalar_wrong_destination",MATH,"","double x" D("scalar") "=4;double y" D("length") "=sqrt(x);",4103},
    {"sqrtf_wrong",MATH,"","float x" D("area") "=4;float y" D("time") "=sqrtf(x);",4103},
    {"sqrtl_wrong",MATH,"","long double x" D("area") "=4;long double y" D("time") "=sqrtl(x);",4103},
    {"parenthesized_name",MATH,"",SPEED "double y" D("time") "=(sqrt)(v*v);",4103},
    {"typegeneric",TGMATH,"",SPEED "double y" D("time") "=sqrt(v*v);",4103},
    {"assignment",MATH,"",SPEED "double t" D("time") "=0;t=sqrt(v*v);",4103},
    {"argument",MATH,"void take(double t" D("time") ");",SPEED "take(sqrt(v*v));",4112},
    {"return",MATH,"double" D("time") " norm(double v" D("speed") "){return sqrt(v*v);}","",4113},
    {"nested_sqrt",MATH,"","double x" D("m^4") "=16;double y" D("length") "=sqrt(sqrt(x));",0},
    {"unit_meter",MATH,"","double x" D("area") U("square_meter") "=4;double y" D("length") U("meter") "=sqrt(x);",0},
    {"unit_centimeter",MATH,"","double x" D("area") U("square_centimeter") "=4;double y" D("length") U("centimeter") "=sqrt(x);",0},
    {"unit_mismatch",MATH,"","double x" D("area") U("square_centimeter") "=4;double y" D("length") U("meter") "=sqrt(x);",4107},
    {"unit_fourth",MATH,"","double x" D("m^4") U("millimeter_to_fourth") "=16;double y" D("area") U("square_millimeter") "=sqrt(x);",0},
    {"unknown_external",MATH,"double external(void);","double t" D("time") "=sqrt(external());",0},
    {"unknown_variable",MATH,"","double x;double t" D("time") "=sqrt(x);",0},
    {"local_pointer_shadow",MATH,"double custom(double x){return x;}","double (*sqrt)(double)=custom;double x" D("length") "=4;double t" D("time") "=sqrt(x);",0},
    {"user_prototype","","double sqrt(double x);","double x" D("length") "=4;double t" D("time") "=sqrt(x);",0},
    {"user_definition",MATH,"double sqrt(double x" D("time") "){return x;}","double t" D("time") "=4;double y" D("time") "=sqrt(t);",0},
    {"indirect_library_pointer",MATH,"","double (*fn)(double)=sqrt;double x" D("length") "=4;double t" D("time") "=fn(x);",0},
};
int main(void) {
    int failures=0;
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        char source[8192];
        snprintf(source,sizeof(source),"%s%s int main(void){%s return 0;}\n",cases[i].headers,cases[i].definitions,cases[i].body);
        FisicsFrontendOptions opts={0};opts.overlay_features=FISICS_OVERLAY_PHYSICS_UNITS;
        FisicsAnalysisResult result={0};
        int ok=fisics_analyze_buffer("units_sqrt.c",source,strlen(source),&opts,&result);
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
    printf("Square-root contracts: %zu cases, %d failures\n",sizeof(cases)/sizeof(cases[0]),failures);
    return failures?1:0;
}
