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
    {"fabs_destination",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double z [[fisics::dim(time)]] =fabs(x);",4103},
    {"fabsf_destination",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;float z [[fisics::dim(time)]] =fabsf(x);",4103},
    {"fabsl_destination",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;long double z [[fisics::dim(time)]] =fabsl(x);",4103},
    {"fabs_valid",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double z [[fisics::dim(length)]]  [[fisics::unit(meter)]] =fabs(x);",0},
    {"fabs_generic_destination",TGMATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double z [[fisics::dim(time)]] =fabs(x);",4103},
    {"fabs_generic_valid",TGMATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double z [[fisics::dim(length)]]  [[fisics::unit(meter)]] =fabs(x);",0},
    {"fabs_unit_retention",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double z [[fisics::dim(length)]]  [[fisics::unit(foot)]] =fabs(x);",4107},
    {"fabs_argument",MATH,"void take(double [[fisics::dim(time)]] );","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;take(fabs(x));",4112},
    {"fabs_return",MATH,"double [[fisics::dim(time)]]  f(double x [[fisics::dim(length)]] ){return fabs(x);}","",4113},
    {"fabs_literal_dimensionless",MATH,"","double z [[fisics::dim(length)]] =fabs(3);",4103},
    {"fabs_unknown",MATH,"","double x;double z [[fisics::dim(time)]] =fabs(x);",0},
    {"fabs_custom_prototype","","double fabs(double);","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double z [[fisics::dim(time)]] =fabs(x);",0},
    {"fabs_pointer_shadow",MATH,"double custom(double a){return a;}","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double (*fabs)(double)=custom;double z [[fisics::dim(time)]] =fabs(x);",0},
    {"fabs_indirect",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double (*fn)(double)=fabs;double z [[fisics::dim(time)]] =fn(x);",0},
    {"fabs_definition",MATH,"double fabs(double a [[fisics::dim(time)]] ){return a;}","double a [[fisics::dim(time)]] =3;double z [[fisics::dim(time)]] =fabs(a);",0},
    {"hypot_destination",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(time)]] =hypot(x,y);",4103},
    {"hypotf_destination",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;float z [[fisics::dim(time)]] =hypotf(x,y);",4103},
    {"hypotl_destination",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;long double z [[fisics::dim(time)]] =hypotl(x,y);",4103},
    {"hypot_valid",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(length)]]  [[fisics::unit(meter)]] =hypot(x,y);",0},
    {"hypot_generic_destination",TGMATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(time)]] =hypot(x,y);",4103},
    {"hypot_generic_valid",TGMATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(length)]]  [[fisics::unit(meter)]] =hypot(x,y);",0},
    {"hypot_unit_retention",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(length)]]  [[fisics::unit(foot)]] =hypot(x,y);",4107},
    {"hypot_argument",MATH,"void take(double [[fisics::dim(time)]] );","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;take(hypot(x,y));",4112},
    {"hypot_return",MATH,"double [[fisics::dim(time)]]  f(double x [[fisics::dim(length)]] ,double y [[fisics::dim(length)]] ){return hypot(x,y);}","",4113},
    {"hypot_literal_dimensionless",MATH,"","double z [[fisics::dim(length)]] =hypot(3,4);",4103},
    {"hypot_unknown",MATH,"","double x;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(time)]] =hypot(x,y);",0},
    {"hypot_custom_prototype","","double hypot(double,double);","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(time)]] =hypot(x,y);",0},
    {"hypot_pointer_shadow",MATH,"double custom(double a,double b){return a;}","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double (*hypot)(double,double)=custom;double z [[fisics::dim(time)]] =hypot(x,y);",0},
    {"hypot_indirect",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double (*fn)(double,double)=hypot;double z [[fisics::dim(time)]] =fn(x,y);",0},
    {"hypot_definition",MATH,"double hypot(double a [[fisics::dim(time)]] ,double b [[fisics::dim(time)]] ){return a;}","double a [[fisics::dim(time)]] =3;double z [[fisics::dim(time)]] =hypot(a,a);",0},
    {"hypot_dimensions",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double t [[fisics::dim(time)]] =4;(void)hypot(x,t);",4116},
    {"hypot_generic_dimensions",TGMATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double t [[fisics::dim(time)]] =4;(void)hypot(x,t);",4116},
    {"hypot_units",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(foot)]] =4;(void)hypot(x,y);",4107},
    {"hypot_bare_zero",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;(void)hypot(x,0);",4116},
    {"hypot_reversed_zero",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;(void)hypot(0,x);",4116},
    {"hypot_typed_zero",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double zero [[fisics::dim(length)]]  [[fisics::unit(meter)]] =0;double z [[fisics::dim(length)]]  [[fisics::unit(meter)]] =hypot(x,zero);",0},
    {"hypot_dimension_only",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]] =4;double z [[fisics::dim(length)]] =hypot(x,y);",0},
    {"hypot_no_invented_unit",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]] =4;(void)fisics_convert_unit(hypot(x,y),\"meter\");",4110},
    {"hypot_no_invented_unit_reverse",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]] =4;(void)fisics_convert_unit(hypot(y,x),\"meter\");",4110},
    {"hypot_unknown_rhs",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y;double z [[fisics::dim(time)]] =hypot(x,y);",0},
    {"hypot_conversion",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(foot)]] =4;double z [[fisics::dim(length)]]  [[fisics::unit(meter)]] =hypot(x,fisics_convert_unit(y,\"meter\"));",0},
    {"fmax_destination",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(time)]] =fmax(x,y);",4103},
    {"fmaxf_destination",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;float z [[fisics::dim(time)]] =fmaxf(x,y);",4103},
    {"fmaxl_destination",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;long double z [[fisics::dim(time)]] =fmaxl(x,y);",4103},
    {"fmax_valid",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(length)]]  [[fisics::unit(meter)]] =fmax(x,y);",0},
    {"fmax_generic_destination",TGMATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(time)]] =fmax(x,y);",4103},
    {"fmax_generic_valid",TGMATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(length)]]  [[fisics::unit(meter)]] =fmax(x,y);",0},
    {"fmax_unit_retention",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(length)]]  [[fisics::unit(foot)]] =fmax(x,y);",4107},
    {"fmax_argument",MATH,"void take(double [[fisics::dim(time)]] );","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;take(fmax(x,y));",4112},
    {"fmax_return",MATH,"double [[fisics::dim(time)]]  f(double x [[fisics::dim(length)]] ,double y [[fisics::dim(length)]] ){return fmax(x,y);}","",4113},
    {"fmax_literal_dimensionless",MATH,"","double z [[fisics::dim(length)]] =fmax(3,4);",4103},
    {"fmax_unknown",MATH,"","double x;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(time)]] =fmax(x,y);",0},
    {"fmax_custom_prototype","","double fmax(double,double);","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(time)]] =fmax(x,y);",0},
    {"fmax_pointer_shadow",MATH,"double custom(double a,double b){return a;}","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double (*fmax)(double,double)=custom;double z [[fisics::dim(time)]] =fmax(x,y);",0},
    {"fmax_indirect",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double (*fn)(double,double)=fmax;double z [[fisics::dim(time)]] =fn(x,y);",0},
    {"fmax_definition",MATH,"double fmax(double a [[fisics::dim(time)]] ,double b [[fisics::dim(time)]] ){return a;}","double a [[fisics::dim(time)]] =3;double z [[fisics::dim(time)]] =fmax(a,a);",0},
    {"fmax_dimensions",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double t [[fisics::dim(time)]] =4;(void)fmax(x,t);",4116},
    {"fmax_generic_dimensions",TGMATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double t [[fisics::dim(time)]] =4;(void)fmax(x,t);",4116},
    {"fmax_units",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(foot)]] =4;(void)fmax(x,y);",4107},
    {"fmax_bare_zero",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;(void)fmax(x,0);",4116},
    {"fmax_reversed_zero",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;(void)fmax(0,x);",4116},
    {"fmax_typed_zero",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double zero [[fisics::dim(length)]]  [[fisics::unit(meter)]] =0;double z [[fisics::dim(length)]]  [[fisics::unit(meter)]] =fmax(x,zero);",0},
    {"fmax_dimension_only",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]] =4;double z [[fisics::dim(length)]] =fmax(x,y);",0},
    {"fmax_no_invented_unit",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]] =4;(void)fisics_convert_unit(fmax(x,y),\"meter\");",4110},
    {"fmax_no_invented_unit_reverse",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]] =4;(void)fisics_convert_unit(fmax(y,x),\"meter\");",4110},
    {"fmax_unknown_rhs",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y;double z [[fisics::dim(time)]] =fmax(x,y);",0},
    {"fmax_conversion",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(foot)]] =4;double z [[fisics::dim(length)]]  [[fisics::unit(meter)]] =fmax(x,fisics_convert_unit(y,\"meter\"));",0},
    {"__builtin_fabs",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double z [[fisics::dim(time)]] =__builtin_fabs(x);",4103},
    {"__builtin_fabsf",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double z [[fisics::dim(time)]] =__builtin_fabsf(x);",4103},
    {"__builtin_fabsl",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double z [[fisics::dim(time)]] =__builtin_fabsl(x);",4103},
    {"nested",MATH,"","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double y [[fisics::dim(length)]]  [[fisics::unit(meter)]] =4;double z [[fisics::dim(length)]]  [[fisics::unit(meter)]] =fmax(fabs(-x),hypot(x,y));",0},
    {"generic_name_user_definition",MATH,"double __fisics_tg_fmax(double a,double b){return a;}","double x [[fisics::dim(length)]]  [[fisics::unit(meter)]] =3;double z [[fisics::dim(time)]] =__fisics_tg_fmax(x,x);",0},
};
int main(void) {
    int failures=0;
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        char source[8192];
        snprintf(source,sizeof(source),"%s%s int main(void){%s return 0;}\n",cases[i].headers,cases[i].definitions,cases[i].body);
        FisicsFrontendOptions opts={0};opts.overlay_features=FISICS_OVERLAY_PHYSICS_UNITS;
        FisicsAnalysisResult result={0};
        int ok=fisics_analyze_buffer("units_math_helpers.c",source,strlen(source),&opts,&result);
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
    printf("Physical math-helper contracts: %zu cases, %d failures\n",sizeof(cases)/sizeof(cases[0]),failures);
    return failures?1:0;
}
